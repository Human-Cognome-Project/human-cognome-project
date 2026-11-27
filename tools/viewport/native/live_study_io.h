#pragma once

#include "signal_guard.h"

#include <fcntl.h>
#include <poll.h>

#include <chrono>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace viewport {
struct LiveOutputError : std::runtime_error {
  LiveOutputError() : std::runtime_error("live study output closed") {}
};
struct LiveOutputStopped {};

inline void ignore_live_sigpipe() {
  struct sigaction action {};
  action.sa_handler = SIG_IGN;
  sigemptyset(&action.sa_mask);
  if (sigaction(SIGPIPE, &action, nullptr) < 0)
    throw std::runtime_error("cannot configure live study output");
}

inline void write_live_output(std::ostream &output, std::string_view line) {
  if (!(output << line << '\n' << std::flush))
    throw LiveOutputError();
}

enum class LiveRead { Line, Deadline, Closed };
using LiveClock = std::chrono::steady_clock;

class LiveCommandInput {
 public:
  LiveCommandInput(const std::string &path, bool watch_stop)
      : fd_(path.empty() ? STDIN_FILENO : open(path.c_str(), O_RDONLY | O_CLOEXEC)),
        owned_(!path.empty()) {
    if (fd_ < 0)
      throw std::invalid_argument("cannot open command file");
    try {
      if (watch_stop) {
        const int flags = fcntl(fd_, F_GETFL);
        if (flags < 0 || fcntl(fd_, F_SETFL, flags | O_NONBLOCK) < 0)
          throw std::runtime_error("cannot configure live study input");
        previous_flags_ = flags;
        const int output_flags = fcntl(STDOUT_FILENO, F_GETFL);
        if (output_flags < 0 || fcntl(STDOUT_FILENO, F_SETFL, output_flags | O_NONBLOCK) < 0)
          throw std::runtime_error("cannot configure live study output");
        previous_output_flags_ = output_flags;
        int pipe_fds[2];
        if (pipe(pipe_fds) < 0)
          throw std::runtime_error("cannot create the live study stop pipe");
        if (fcntl(pipe_fds[0], F_SETFD, FD_CLOEXEC) < 0 ||
            fcntl(pipe_fds[1], F_SETFD, FD_CLOEXEC) < 0 ||
            fcntl(pipe_fds[1], F_SETFL, O_NONBLOCK) < 0) {
          close(pipe_fds[0]);
          close(pipe_fds[1]);
          throw std::runtime_error("cannot configure the live study stop pipe");
        }
        // The handlers are process-wide and may run on any native worker.
        // Keep their descriptors valid until process exit.
        wake_ = pipe_fds[0];
        stop_wake_fd.store(pipe_fds[1], std::memory_order_relaxed);
        install_stop_handlers();
      }
    } catch (...) {
      restore_descriptors();
      throw;
    }
  }
  LiveCommandInput(const LiveCommandInput &) = delete;
  LiveCommandInput &operator=(const LiveCommandInput &) = delete;
  ~LiveCommandInput() { restore_descriptors(); }

  bool interruptible() const { return wake_ >= 0; }
  bool stopped() const { return interruptible() && stop_requested(); }
  void check_consumer() const {
    if (!interruptible()) return;
    struct pollfd output{STDOUT_FILENO, 0, 0};
    while (poll(&output, 1, 0) < 0) {
      if (errno == EINTR) continue;
      throw std::runtime_error("live study output poll failed");
    }
    if (output.revents & output_closed) throw LiveOutputError();
  }
  void write_output(std::string line) const {
    line.push_back('\n');
    std::string_view remaining(line);
    while (!remaining.empty()) {
      const auto count = ::write(STDOUT_FILENO, remaining.data(), remaining.size());
      if (count > 0) {
        remaining.remove_prefix(static_cast<std::size_t>(count));
        continue;
      }
      if (count < 0 && errno == EINTR) continue;
      if (count == 0 || (errno != EAGAIN && errno != EWOULDBLOCK)) throw LiveOutputError();
      // A final record can still be delivered if it fits. Shutdown never waits
      // for the reader to make room once a write has encountered backpressure.
      if (stopped()) throw LiveOutputStopped();
      struct pollfd ready[]{{STDOUT_FILENO, POLLOUT, 0}, {wake_, POLLIN, 0}};
      if (poll(ready, 2, -1) < 0) {
        if (errno == EINTR) continue;
        throw std::runtime_error("live study output wait failed");
      }
      if (ready[0].revents & output_closed) throw LiveOutputError();
      if (ready[1].revents || stopped()) throw LiveOutputStopped();
    }
  }
  bool read_line(std::string &line) {
    return read_until(line, std::nullopt)==LiveRead::Line;
  }
  LiveRead read_until(std::string &line, std::optional<LiveClock::time_point> deadline) {
    while (!stopped()) {
      const auto newline = pending_.find('\n');
      if (newline != std::string::npos) {
        line.assign(pending_, 0, newline);
        pending_.erase(0, newline + 1);
        return LiveRead::Line;
      }
      if (wake_ >= 0 || deadline) {
        // A pipe writer reports a lost reader even when no write is pending.
        struct pollfd ready[]{{fd_, POLLIN, 0}, {wake_, POLLIN, 0}, {STDOUT_FILENO, 0, 0}};
        int timeout=-1;
        if (deadline) {
          const auto left=*deadline-LiveClock::now();
          timeout=left<=LiveClock::duration::zero() ? 0 :
              static_cast<int>(std::chrono::ceil<std::chrono::milliseconds>(left).count());
        }
        const auto count=poll(ready, 3, timeout);
        if (count==0) return LiveRead::Deadline;
        if (count<0) {
          if (errno == EINTR) continue;
          throw std::runtime_error("command input wait failed");
        }
        if (ready[2].revents & output_closed) throw LiveOutputError();
        if (ready[1].revents || stopped()) return LiveRead::Closed;
      }
      char bytes[4096];
      const auto count = ::read(fd_, bytes, sizeof(bytes));
      if (count < 0) {
        if (errno == EINTR || (wake_ >= 0 && (errno == EAGAIN || errno == EWOULDBLOCK))) continue;
        throw std::runtime_error("command input failed");
      }
      if (count == 0) {
        if (pending_.empty() || stopped()) return LiveRead::Closed;
        line = std::move(pending_);
        pending_.clear();
        return LiveRead::Line;
      }
      pending_.append(bytes, static_cast<std::size_t>(count));
    }
    return LiveRead::Closed;
  }

 private:
  static constexpr short output_closed = POLLERR | POLLHUP | POLLNVAL;
  void restore_descriptors() {
    if (previous_output_flags_ >= 0) fcntl(STDOUT_FILENO, F_SETFL, previous_output_flags_);
    if (owned_) close(fd_);
    else if (previous_flags_ >= 0) fcntl(fd_, F_SETFL, previous_flags_);
  }
  int fd_;
  bool owned_;
  int previous_flags_{-1};
  int previous_output_flags_{-1};
  int wake_{-1};
  std::string pending_;
};
}  // namespace viewport
