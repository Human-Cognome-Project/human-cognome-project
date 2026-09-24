// Unit tests for box::Box / box::Message. Standalone: standard library
// only, no test framework. Prints one line per check; the process exits
// non-zero if any check failed.
#include <cstdio>
#include <optional>
#include <stdexcept>
#include <string>

#include "box.h"

namespace {

int g_failures = 0;

void check(bool ok, const std::string &what) {
  if (ok) {
    std::printf("ok   %s\n", what.c_str());
  } else {
    std::printf("FAIL %s\n", what.c_str());
    ++g_failures;
  }
}

void test_fifo_order() {
  box::Box b;
  b.push(box::Message{"first", std::nullopt});
  b.push(box::Message{"second", std::nullopt});
  b.push(box::Message{"third", std::nullopt});

  check(b.size() == 3, "three pushed messages give size 3");
  check(!b.empty(), "a box with pushed messages is not empty");

  box::Message m1 = b.pop();
  box::Message m2 = b.pop();
  box::Message m3 = b.pop();
  check(m1.payload == "first" && m2.payload == "second" && m3.payload == "third",
        "pop returns messages in FIFO (push) order");
}

void test_occupancy_transitions() {
  box::Box b;
  check(b.empty(), "a freshly constructed box is empty");
  check(b.size() == 0, "a freshly constructed box has size 0");

  b.push(box::Message{"x", std::nullopt});
  check(!b.empty(), "box becomes non-empty after a push");
  check(b.size() == 1, "box size reflects one pushed message");

  b.pop();
  check(b.empty(), "box becomes empty again after its only message is popped");
  check(b.size() == 0, "box size returns to 0 after draining");
}

void test_pop_empty_is_not_ok() {
  box::Box b;
  bool threw = false;
  try {
    b.pop();
  } catch (const std::logic_error &) {
    threw = true;
  }
  check(threw, "pop() on an empty box surfaces as !ok (throws), never UB");
}

void test_reply_to_carried_unexamined() {
  box::Box b;
  endpoint::EndpointId reply{7, 3};
  b.push(box::Message{"payload", reply});
  box::Message m = b.pop();
  check(m.reply_to.has_value() && *m.reply_to == reply,
        "a Message's reply_to endpoint round-trips through the box unexamined");

  b.push(box::Message{"no reply", std::nullopt});
  box::Message m2 = b.pop();
  check(!m2.reply_to.has_value(), "a Message with no reply_to carries none through the box");
}

}  // namespace

int main() {
  test_fifo_order();
  test_occupancy_transitions();
  test_pop_empty_is_not_ok();
  test_reply_to_carried_unexamined();

  if (g_failures == 0) {
    std::printf("PASS box_test\n");
    return 0;
  }
  std::printf("FAIL box_test (%d failed)\n", g_failures);
  return 1;
}
