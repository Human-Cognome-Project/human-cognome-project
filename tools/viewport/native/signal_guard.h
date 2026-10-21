#pragma once

#include <signal.h>

#include <atomic>
#include <csignal>
#include <functional>
#include <utility>

// Stop-signal handling of viewport_letter_capture, kept free of the engine and database so the
// committed-lease boundary can be unit tested deterministically.
namespace viewport {
// Signals only set this flag (async-signal-safe); the capture loop acts on it between ticks.
// A lock-free atomic: a handler may run on any thread the engine started, and relaxed stores and
// loads are async-signal-safe for a lock-free type.
inline std::atomic<int> stop_flag{0};
static_assert(std::atomic<int>::is_always_lock_free, "stop flag must be lock-free");
inline void on_signal(int) { stop_flag.store(1, std::memory_order_relaxed); }
inline bool stop_requested() { return stop_flag.load(std::memory_order_relaxed) != 0; }
// Installed before the engine starts or anything is acquired, with no masking, so a stuck
// start-up or a stalled acquire stays interruptible: the first signal is noted, and
// SA_RESETHAND makes a second signal of the same kind take the default action. SA_RESTART keeps
// ordinary blocking I/O from failing with EINTR on that first, noted signal.
inline void install_stop_handlers() {
  struct sigaction action {};
  action.sa_handler = on_signal;
  action.sa_flags = SA_RESETHAND | SA_RESTART;
  sigemptyset(&action.sa_mask);
  sigaction(SIGINT, &action, nullptr);
  sigaction(SIGTERM, &action, nullptr);
}
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
// acquire() may commit a persistent lease before it returns. With the stop handlers already
// installed, a signal anywhere in that window only sets the flag, so the guard is constructed
// right after acquire returns and the run ends gracefully at its first stop check with the pin
// released. Returns the acquired value and the guard. If acquire() throws, nothing is guarded.
template <class Acquire>
auto acquire_guarded(Acquire acquire, std::function<void()> release) {
  auto value = acquire();
  Pin pin(std::move(release));
  return std::make_pair(std::move(value), std::move(pin));
}
}  // namespace viewport
