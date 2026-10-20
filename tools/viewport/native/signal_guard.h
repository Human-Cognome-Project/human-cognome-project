#pragma once

#include <pthread.h>
#include <signal.h>

#include <csignal>
#include <functional>
#include <utility>

// Stop-signal handling of viewport_letter_capture, kept free of the engine and database so the
// committed-lease boundary can be unit tested deterministically.
namespace viewport {
// Signals only set this flag (async-signal-safe); the capture loop acts on it between ticks.
inline volatile std::sig_atomic_t stop_flag = 0;
inline void on_signal(int) { stop_flag = 1; }
inline bool stop_requested() { return stop_flag != 0; }
// SA_RESETHAND: after the first signal a second one takes the default action, so a stuck run
// stays killable.
inline void install_stop_handlers() {
  struct sigaction action {};
  action.sa_handler = on_signal;
  action.sa_flags = SA_RESETHAND;
  sigemptyset(&action.sa_mask);
  sigaction(SIGINT, &action, nullptr);
  sigaction(SIGTERM, &action, nullptr);
}
// Blocks SIGINT/SIGTERM in the calling thread for its lifetime. Threads created meanwhile inherit
// the mask. On exit the previous mask returns and a signal that arrived meanwhile is delivered
// then, under whatever disposition is current.
class SignalBlock {
 public:
  SignalBlock() {
    sigset_t blocked;
    sigemptyset(&blocked);
    sigaddset(&blocked, SIGINT);
    sigaddset(&blocked, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &blocked, &previous_);
  }
  SignalBlock(const SignalBlock &) = delete;
  SignalBlock &operator=(const SignalBlock &) = delete;
  ~SignalBlock() { pthread_sigmask(SIG_SETMASK, &previous_, nullptr); }

 private:
  sigset_t previous_;
};
// Releases the owned warm pin when still held, ignoring errors; release() is the checked success path.
struct Pin {
  std::function<void()> release_fn;
  explicit Pin(std::function<void()> f) : release_fn(std::move(f)) {}
  Pin(Pin &&other) noexcept : release_fn(std::move(other.release_fn)) { other.release_fn = nullptr; }
  Pin(const Pin &) = delete;
  Pin &operator=(const Pin &) = delete;
  ~Pin() { try { if (release_fn) release_fn(); } catch (...) {} }
  void release() { auto f = std::move(release_fn); release_fn = nullptr; if (f) f(); }
};
// acquire() may commit a persistent lease before it returns. SIGINT/SIGTERM stay blocked from
// before it runs until the release guard exists and the handlers are installed; a signal that
// arrived in between is then delivered to the graceful handler, never the default action.
// Returns the acquired value and the guard. If acquire() throws, nothing is guarded.
template <class Acquire>
auto acquire_guarded(Acquire acquire, std::function<void()> release) {
  SignalBlock block;
  auto value = acquire();
  Pin pin(std::move(release));
  install_stop_handlers();
  return std::make_pair(std::move(value), std::move(pin));
}
}  // namespace viewport
