#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

#include "live_study_io.h"
#include "signal_guard.h"

namespace {
int checks = 0;
void check(bool ok, const char *label) {
  ++checks;
  if (!ok) throw std::runtime_error(label);
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
    // Control: without the handlers a signal after the lease commit ends the process at once,
    // running no destructor and so releasing nothing.
    const int unprotected = in_child([] {
      struct Release { ~Release() { _exit(5); } } would_release;
      raise(SIGINT);  // The lease was committed just before this point.
      return 0;
    });
    check(WIFSIGNALED(unprotected) && WTERMSIG(unprotected) == SIGINT,
          "control: an unhandled signal after the commit terminates the process without releasing");
    // With the handlers installed first, the same interleaving only sets the flag.
    for (const int signo : {SIGINT, SIGTERM}) {
      const int status = in_child([signo] {
        install_stop_handlers();
        int released = 0;
        {
          auto guarded = acquire_guarded([&] { raise(signo); return 7; }, [&] { ++released; });
          if (guarded.first != 7) return 10;
          if (!stop_requested()) return 11;  // Noted, not fatal.
          if (released) return 12;           // Held until the run finishes.
        }                                    // Guard destroyed: release runs.
        return released == 1 ? 0 : 13;
      });
      check(status == 0, signo == SIGINT ? "SIGINT after the committed lease: survives, flag set, release once"
                                         : "SIGTERM after the committed lease: survives, flag set, release once");
    }
    // Explicit release runs once and the destructor does not repeat it.
    check(in_child([] {
      install_stop_handlers();
      int released = 0;
      {
        auto guarded = acquire_guarded([&] { raise(SIGTERM); return 0; }, [&] { ++released; });
        guarded.second.release();
        if (released != 1) return 10;
      }
      return released == 1 && stop_requested() ? 0 : 11;
    }) == 0, "explicit release runs once");
    // A second signal of the same kind takes the default action (SA_RESETHAND).
    const int second = in_child([] {
      install_stop_handlers();
      raise(SIGINT);
      if (!stop_requested()) return 10;
      raise(SIGINT);
      return 11;
    });
    check(WIFSIGNALED(second) && WTERMSIG(second) == SIGINT, "a second signal ends the process");
    check(in_child([] {
      int commands[2];
      if (pipe(commands) < 0 || dup2(commands[0], STDIN_FILENO) < 0) return 10;
      close(commands[0]);
      const char input[] = "observe\npartial";
      if (write(commands[1], input, sizeof(input)-1) != ssize_t(sizeof(input)-1)) return 11;
      const auto flags = fcntl(STDIN_FILENO, F_GETFL);
      {
        LiveCommandInput reader("", true);
        std::string line;
        if (!reader.read_line(line) || line != "observe") return 12;
        std::thread worker([] {
          std::this_thread::sleep_for(std::chrono::milliseconds(20));
          raise(SIGTERM);
        });
        const bool read_more = reader.read_line(line);
        worker.join();
        if (read_more || !reader.stopped()) return 13;
      }
      close(commands[1]);
      return fcntl(STDIN_FILENO, F_GETFL) == flags ? 0 : 14;
    }) == 0, "worker signal wakes a partial command wait on an open pipe and restores input flags");
    check(in_child([] {
      int output[2];
      if (pipe(output) < 0 || dup2(output[1], STDOUT_FILENO) < 0) return 10;
      close(output[0]);
      close(output[1]);
      ignore_live_sigpipe();
      try {
        write_live_output(std::cout,"completed observation");
      } catch (const LiveOutputError &) {
        return 0;
      }
      return 11;
    }) == 0, "a failed flush on a broken output pipe reaches the CLI error path");
    // A failing acquire guards nothing.
    check(in_child([] {
      install_stop_handlers();
      int released = 0;
      try {
        acquire_guarded([]() -> int { throw std::runtime_error("no lease"); }, [&] { ++released; });
        return 10;
      } catch (const std::runtime_error &) {}
      return released == 0 ? 0 : 11;
    }) == 0, "failing acquire: nothing released");
    // The fork throws bare strings: a catch-all unwinds, so the guard runs and the pin is released.
    check(in_child([] {
      int released = 0;
      try {
        auto guarded = acquire_guarded([] { return 1; }, [&] { ++released; });
        throw std::string("bare string from the engine");
      } catch (...) {
        return released == 1 ? 0 : 10;  // Unwound before the handler body.
      }
    }) == 0, "a bare-string throw caught by catch(...) unwinds and releases the pin");
    std::cout << checks << " viewport signal guard checks passed\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "viewport signal guard test: " << e.what() << '\n';
    return 1;
  }
}
