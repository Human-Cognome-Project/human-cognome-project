#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <iostream>
#include <string_view>

int main(int argc, char **argv) {
  // The PostgreSQL probe lends the pipe descriptor on stdin. Inspect it
  // without consuming any bytes from the live study.
  if (argc > 2 || (argc == 2 && std::string_view(argv[1]) != "shrink")) return 2;
  if (argc == 2 && fcntl(STDIN_FILENO, F_SETPIPE_SZ, 4096) < 0) return 1;
  const int capacity = fcntl(STDIN_FILENO, F_GETPIPE_SZ);
  int queued = 0;
  if (capacity < 0 || ioctl(STDIN_FILENO, FIONREAD, &queued) < 0) return 1;
  std::cout << capacity << ' ' << queued << '\n';
}
