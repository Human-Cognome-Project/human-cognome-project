#include <sys/wait.h>
#include <unistd.h>

#include <atomic>
#include <iostream>
#include <stdexcept>
#include <thread>

#include "signal_guard.h"

namespace {
int checks = 0;
void check(bool ok, const char *label) {
  ++checks;
  if (!ok) throw std::runtime_error(label);
}
bool blocked_here(int signo) {
  sigset_t mask;
  pthread_sigmask(SIG_BLOCK, nullptr, &mask);
  return sigismember(&mask, signo) == 1;
}
// Runs body in a child process; returns the raw wait status.
template <class F> int in_child(F body) {
  const pid_t pid = fork();
  if (pid == 0) {  // A launcher may have left the signals ignored: start from the default action.
    signal(SIGINT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
    _exit(body());
  }
  int status = 0;
  waitpid(pid, &status, 0);
  return status;
}
}  // namespace
int main() {
  try {
    using namespace viewport;
    // The boundary itself: acquire commits, then a signal arrives before the guard and the handlers
    // exist. Without the block the default action ends the process and nothing is released.
    const int unprotected = in_child([] {
      struct Release { ~Release() { _exit(5); } } would_release;  // Never runs on the default action.
      raise(SIGINT);  // The lease was committed just before this point.
      return 0;
    });
    check(WIFSIGNALED(unprotected) && WTERMSIG(unprotected) == SIGINT,
          "control: an unprotected signal after the commit terminates the process without releasing");
    // With acquire_guarded the same interleaving is deferred, then handled gracefully.
    check(in_child([] {
      bool committed = false, released = false;
      {
        auto guarded = acquire_guarded(
          [&] { committed = true; raise(SIGINT); return 7; },
          [&] { released = true; });
        if (guarded.first != 7 || !committed) return 10;
        if (!stop_requested()) return 11;  // Delivered to the handler once unblocked.
        if (released) return 12;           // Still held: the loop owns it until it finishes.
      }                                    // Guard destroyed: release runs.
      return released && !blocked_here(SIGINT) && !blocked_here(SIGTERM) ? 0 : 13;
    }) == 0, "signal after the committed lease: survives, flag set, pin released, mask restored");
    check(in_child([] {
      bool released = false;
      {
        auto guarded = acquire_guarded([&] { raise(SIGTERM); return 0; }, [&] { released = true; });
        guarded.second.release();
        if (!released) return 10;
      }
      return stop_requested() ? 0 : 11;
    }) == 0, "SIGTERM likewise; explicit release runs once");
    // A second signal of the same kind takes the default action (SA_RESETHAND).
    check(WIFSIGNALED(in_child([] {
      auto guarded = acquire_guarded([] { raise(SIGINT); return 0; }, [] {});
      if (!stop_requested()) return 10;
      raise(SIGINT);
      return 11;
    })), "a second signal ends the process");
    // A failing acquire guards nothing and leaves the mask as it was.
    check(in_child([] {
      bool released = false;
      try {
        acquire_guarded([]() -> int { throw std::runtime_error("no lease"); }, [&] { released = true; });
        return 10;
      } catch (const std::runtime_error &) {}
      return !released && !blocked_here(SIGINT) && !stop_requested() ? 0 : 11;
    }) == 0, "failing acquire: nothing released, mask restored");
    // Threads created while blocked inherit the block; the creator's mask is restored.
    {
      std::atomic<int> thread_blocks_int{-1};
      {
        SignalBlock block;
        std::thread t([&] { thread_blocks_int = blocked_here(SIGINT) && blocked_here(SIGTERM); });
        t.join();
        check(blocked_here(SIGINT), "blocked inside the scope");
      }
      check(thread_blocks_int == 1, "engine threads started under the block inherit it");
      check(!blocked_here(SIGINT) && !blocked_here(SIGTERM), "creator's mask restored");
    }
    std::cout << checks << " viewport signal guard checks passed\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "viewport signal guard test: " << e.what() << '\n';
    return 1;
  }
}
