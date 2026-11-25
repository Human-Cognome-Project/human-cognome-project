#pragma once

#include "signal_guard.h"

#include <fcntl.h>
#include <poll.h>

#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace viewport {
struct LiveOutputError : std::runtime_error {
  LiveOutputError() : std::runtime_error("live study output closed") {}
};

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
      restore_input();
      throw;
    }
  }
  LiveCommandInput(const LiveCommandInput &) = delete;
  LiveCommandInput &operator=(const LiveCommandInput &) = delete;
  ~LiveCommandInput() { restore_input(); }

  bool stopped() const { return wake_ >= 0 && stop_requested(); }
  bool read_line(std::string &line) {
    while (!stopped()) {
      const auto newline = pending_.find('\n');
      if (newline != std::string::npos) {
        line.assign(pending_, 0, newline);
        pending_.erase(0, newline + 1);
        return true;
      }
      if (wake_ >= 0) {
        // A pipe writer reports a lost reader even when no write is pending.
        struct pollfd ready[]{{fd_, POLLIN, 0}, {wake_, POLLIN, 0}, {STDOUT_FILENO, 0, 0}};
        if (poll(ready, 3, -1) < 0) {
          if (errno == EINTR) continue;
          throw std::runtime_error("command input wait failed");
        }
        if (ready[2].revents & (POLLERR | POLLHUP | POLLNVAL)) throw LiveOutputError();
        if (ready[1].revents || stopped()) return false;
      }
      char bytes[4096];
      const auto count = read(fd_, bytes, sizeof(bytes));
      if (count < 0) {
        if (errno == EINTR || (wake_ >= 0 && (errno == EAGAIN || errno == EWOULDBLOCK))) continue;
        throw std::runtime_error("command input failed");
      }
      if (count == 0) {
        if (pending_.empty() || stopped()) return false;
        line = std::move(pending_);
        pending_.clear();
        return true;
      }
      pending_.append(bytes, static_cast<std::size_t>(count));
    }
    return false;
  }

 private:
  void restore_input() {
    if (owned_) close(fd_);
    else if (previous_flags_ >= 0) fcntl(fd_, F_SETFL, previous_flags_);
  }
  int fd_;
  bool owned_;
  int previous_flags_{-1};
  int wake_{-1};
  std::string pending_;
};
}  // namespace viewport
