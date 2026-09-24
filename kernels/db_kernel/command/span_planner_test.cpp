// Standalone check harness for the address span planner. Same style as
// codec/codec_test.cpp: a tiny check macro, one line per check, PASS/FAIL
// summary, non-zero exit on any failure. Standard library + codec +
// command_ir/span_planner only.
#include <cstdio>
#include <memory>
#include <string>

#include "codec.h"
#include "command_ir.h"
#include "span_planner.h"

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

using command::AddressSegment;
using command::AddressSpan;
using command::DeclareRecord;
using command::PlanStatus;

codec::Address addr(const std::string &token_id) {
  auto a = codec::decode_token_id(token_id);
  if (!a.has_value()) {
    std::fprintf(stderr, "test bug: %s is not a decodable token_id\n",
                 token_id.c_str());
    std::abort();
  }
  return *a;
}

// ---------------------------------------------------------------------
// successor()
// ---------------------------------------------------------------------

void test_successor() {
  // Plain within-element increment: AA -> AB.
  {
    auto next = command::successor(addr("AA"));
    check(next.has_value() && *next == addr("AB"), "successor(AA) == AB");
  }
  // Carry into the previous element: AZ's last alphabet char is 'Z'
  // (index 24), so AA.zz (max couplet) rolls the last element and
  // carries into the first: AA.zz -> AB.AA.
  {
    auto next = command::successor(addr("AA.zz"));
    check(next.has_value() && *next == addr("AB.AA"),
          "successor(AA.zz) carries into the previous element -> AB.AA");
  }
  // Full overflow: the single-element max (zz) has nowhere to carry to.
  {
    auto next = command::successor(addr("zz"));
    check(!next.has_value(),
          "successor(zz) overflows the leftmost element -> nullopt (G5, "
          "not guessed here)");
  }
  // Partial (wildcard) elements have no single successor.
  {
    codec::Address partial = {codec::make_partial_element(0)};
    auto next = command::successor(partial);
    check(!next.has_value(), "successor of a partial address is nullopt");
  }
  // Empty address has no successor.
  {
    auto next = command::successor(codec::Address{});
    check(!next.has_value(), "successor of an empty address is nullopt");
  }
}

// ---------------------------------------------------------------------
// span_length()
// ---------------------------------------------------------------------

void test_span_length() {
  {
    auto len = command::span_length(addr("AA"), addr("AA"));
    check(len.has_value() && *len == 1, "span_length(AA, AA) == 1 (inclusive)");
  }
  {
    auto len = command::span_length(addr("AA"), addr("AD"));
    check(len.has_value() && *len == 4, "span_length(AA, AD) == 4");
  }
  {
    // AA.zz -> AB.AB spans exactly 3: AA.zz, AB.AA, AB.AB.
    auto len = command::span_length(addr("AA.zz"), addr("AB.AB"));
    check(len.has_value() && *len == 3,
          "span_length carries across an element boundary correctly");
  }
  {
    auto len = command::span_length(addr("AD"), addr("AA"));
    check(!len.has_value(), "span_length rejects a backward run (to precedes from)");
  }
  {
    auto len = command::span_length(addr("AA"), addr("AA.AA"));
    check(!len.has_value(), "span_length rejects mismatched address depth");
  }
}

// ---------------------------------------------------------------------
// plan()
// ---------------------------------------------------------------------

void test_plan_direct_and_pin_cover_n() {
  AddressSpan span = {AddressSegment::Direct(addr("AA")),
                       AddressSegment::Pin(addr("AB")),
                       AddressSegment::Direct(addr("AC"))};
  auto result = command::plan(span, 3);
  check(result.status == PlanStatus::kValid, "3 self-delimiting segments cover N=3");
  check(result.slots.size() == 3, "plan() returns one slot per covered position");
  check(result.slots[1].address.has_value() && *result.slots[1].address == addr("AB"),
        "a pin segment's requested address is planned as its slot value");
}

void test_plan_cover_n_mismatch_fails() {
  AddressSpan span = {AddressSegment::Direct(addr("AA")),
                       AddressSegment::Direct(addr("AB"))};
  auto result = command::plan(span, 3);
  check(result.status == PlanStatus::kInvalid,
        "2 self-delimiting segments do not cover N=3 -- rejected");
  check(!result.reason.empty(), "a cover-N failure carries a reason");
}

void test_plan_from_to_fixed_run() {
  AddressSpan span = {AddressSegment::From(addr("AA"), addr("AD"))};
  auto result = command::plan(span, 4);
  check(result.status == PlanStatus::kValid, "FROM:AA TO:AD covers N=4");
  bool sequential = result.slots.size() == 4 &&
                     result.slots[0].address == addr("AA") &&
                     result.slots[1].address == addr("AB") &&
                     result.slots[2].address == addr("AC") &&
                     result.slots[3].address == addr("AD");
  check(sequential, "FROM..TO fills sequentially via the successor helper");
}

void test_plan_open_from_elastic_last_only() {
  // Open FROM as the last segment: elastic, consumes the remainder.
  AddressSpan ok_span = {AddressSegment::Direct(addr("AA")),
                          AddressSegment::From(addr("BA"))};
  auto ok_result = command::plan(ok_span, 3);
  check(ok_result.status == PlanStatus::kValid,
        "an open FROM as the last segment elastically covers the remainder");
  check(ok_result.slots.size() == 3 && *ok_result.slots[1].address == addr("BA") &&
            *ok_result.slots[2].address == addr("BB"),
        "the elastic tail fills sequentially from its origin");

  // Open FROM as an interior segment: rejected.
  AddressSpan bad_span = {AddressSegment::From(addr("BA")),
                           AddressSegment::Direct(addr("AA"))};
  auto bad_result = command::plan(bad_span, 2);
  check(bad_result.status == PlanStatus::kInvalid,
        "an open (un-TO'd) FROM is rejected outside the last position");
}

void test_plan_interior_self_delimiting() {
  // A closed FROM..TO run in an interior position, surrounded by other
  // self-delimiting segments, is legal.
  AddressSpan span = {AddressSegment::Direct(addr("ZZ")),
                       AddressSegment::From(addr("AA"), addr("AC")),
                       AddressSegment::Pin(addr("BA"))};
  auto result = command::plan(span, 5);
  check(result.status == PlanStatus::kValid,
        "a closed FROM..TO run is self-delimiting and legal in an interior position");
  check(result.slots.size() == 5, "interior closed run contributes its exact fixed count");
}

void test_plan_nested_declare_is_one_slot_anywhere() {
  auto nested = std::make_shared<DeclareRecord>();

  // Interior position.
  AddressSpan interior = {AddressSegment::Direct(addr("AA")),
                           AddressSegment::Nested(nested),
                           AddressSegment::Direct(addr("AB"))};
  auto interior_result = command::plan(interior, 3);
  check(interior_result.status == PlanStatus::kValid,
        "a nested declare is a legal interior slot");
  check(interior_result.slots[1].nested == nested.get(),
        "the planned slot records which nested statement occupies it");
  check(!interior_result.slots[1].address.has_value(),
        "a nested declare's slot has no address yet -- resolved at its own mint");

  // Last position.
  AddressSpan last = {AddressSegment::Direct(addr("AA")), AddressSegment::Nested(nested)};
  auto last_result = command::plan(last, 2);
  check(last_result.status == PlanStatus::kValid,
        "a nested declare is also a legal last slot");
}

void test_plan_undeclared_hook() {
  AddressSpan span = {AddressSegment::Direct(addr("AA")), AddressSegment::Undeclared()};
  auto result = command::plan(span, 2);
  check(result.status == PlanStatus::kValid, "the undeclared hook is one inert slot");
  check(result.slots[1].is_undeclared_hook && !result.slots[1].address.has_value(),
        "an undeclared slot carries no address, only the hook marker");
}

void test_plan_after_open_is_pending_seam() {
  AddressSpan span = {AddressSegment::Direct(addr("AA")), AddressSegment::After(addr("BZ"))};
  auto result = command::plan(span, 3);
  check(result.status == PlanStatus::kPendingSeam,
        "an open AFTER tail is structurally valid but pending the trunk map (G5)");
  check(result.slots.size() == 3 && !result.slots[1].address.has_value() &&
            !result.slots[2].address.has_value(),
        "AFTER's slots are counted but left unresolved pending G5");
}

void test_plan_after_to_is_pending_seam() {
  AddressSpan span = {AddressSegment::Direct(addr("AA")),
                       AddressSegment::After(addr("BZ"), addr("CA")),
                       AddressSegment::Direct(addr("DA"))};
  auto result = command::plan(span, 3);
  check(result.status == PlanStatus::kPendingSeam,
        "a closed AFTER..TO segment cannot have its count verified without "
        "the trunk map -- reported pending, not guessed");
}

}  // namespace

int main() {
  test_successor();
  test_span_length();
  test_plan_direct_and_pin_cover_n();
  test_plan_cover_n_mismatch_fails();
  test_plan_from_to_fixed_run();
  test_plan_open_from_elastic_last_only();
  test_plan_interior_self_delimiting();
  test_plan_nested_declare_is_one_slot_anywhere();
  test_plan_undeclared_hook();
  test_plan_after_open_is_pending_seam();
  test_plan_after_to_is_pending_seam();

  if (g_failures == 0) {
    std::printf("PASS span_planner_test\n");
    return 0;
  }
  std::printf("FAIL span_planner_test (%d check(s) failed)\n", g_failures);
  return 1;
}
