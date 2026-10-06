// Check harness for the complete-parent mass guard (#109) in declare_core.
// Same tiny check-macro style as codec/codec_test.cpp.
//
// Takes a libpq conninfo as argv[1] and REFUSES to run unless its dbname
// starts with "hcp_test_" -- it never touches hcp_core. It resets nothing:
// run_mass_guard_test.sh creates the throwaway database and applies the
// schema; this harness only inserts and reads.
#include <libpq-fe.h>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "codec.h"
#include "command_ir.h"
#include "controller.h"
#include "declare_core.h"

namespace {

int g_failures = 0;

void check(bool ok, const std::string &what) {
  std::printf("%s %s\n", ok ? "ok  " : "FAIL", what.c_str());
  if (!ok) {
    ++g_failures;
  }
}

using codec::Address;
using command::AddressSegment;
using command::AddressSpan;
using command::DeclareRecord;
using command::Reference;

Address A(const std::string &token_id) {
  const auto a = codec::decode_token_id(token_id);
  if (!a.has_value()) {
    std::fprintf(stderr, "test bug: undecodable token_id '%s'\n", token_id.c_str());
    std::exit(2);
  }
  return *a;
}

DeclareRecord composite(const std::string &at, std::vector<Reference> parents) {
  DeclareRecord node;
  node.parents = std::vector<command::ConstituentList>{std::move(parents)};
  node.address = AddressSpan{AddressSegment::Direct(A(at))};
  return node;
}

long count(PGconn *conn, const std::string &sql) {
  PGresult *res = PQexec(conn, sql.c_str());
  long n = -1;
  if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) == 1) {
    n = std::atol(PQgetvalue(res, 0, 0));
  }
  PQclear(res);
  return n;
}

}  // namespace

int main(int argc, char **argv) {
  if (argc != 2) {
    std::fprintf(stderr, "usage: %s <conninfo with dbname=hcp_test_*>\n", argv[0]);
    return 2;
  }
  const std::string conninfo = argv[1];
  if (conninfo.find("dbname=hcp_test_") == std::string::npos) {
    std::fprintf(stderr, "refusing: dbname must start with hcp_test_\n");
    return 2;
  }

  dbk::Controller ctl(conninfo);
  PGconn *raw = PQconnectdb(conninfo.c_str());
  if (PQstatus(raw) != CONNECTION_OK) {
    std::fprintf(stderr, "cannot connect: %s\n", PQerrorMessage(raw));
    return 2;
  }

  // Bootstrap atoms minted directly: known mass (AA, AB) and unknown (AC).
  ctl.mint(A("AA"), "known-1", {}, 1);
  ctl.mint(A("AB"), "known-2", {}, 1);
  ctl.mint(A("AC"), "incomplete", {}, std::nullopt);
  check(ctl.attributes_of(A("AC")).has_value() &&
            !ctl.attributes_of(A("AC"))->mass.has_value(),
        "fixture: AC is present with NULL mass");

  // 1. NULL-mass parent -> rejected, nothing written.
  const long tokens_before = count(raw, "SELECT count(*) FROM token");
  const long parents_before = count(raw, "SELECT count(*) FROM token_parent");
  const long children_before = count(raw, "SELECT count(*) FROM token_child");
  {
    declare::Result r = declare::execute(
        ctl, composite("BA", {Reference::ToAddress(A("AA")), Reference::ToAddress(A("AC"))}));
    check(r.outcomes.size() == 1 && !r.outcomes[0].ingested,
          "NULL-mass parent: composite rejected");
    check(!r.outcomes.empty() &&
              r.outcomes[0].reason.find("unknown (NULL) mass") != std::string::npos,
          "NULL-mass parent: reason names the unknown mass");
    check(!ctl.token_exists(A("BA")), "NULL-mass parent: no token row written");
    check(ctl.parents_of(A("BA")).empty(), "NULL-mass parent: no token_parent rows");
    check(count(raw, "SELECT count(*) FROM token") == tokens_before &&
              count(raw, "SELECT count(*) FROM token_parent") == parents_before &&
              count(raw, "SELECT count(*) FROM token_child") == children_before,
          "NULL-mass parent: table row counts unchanged");
  }

  // 1b. Rejected inside a nested declare: the outer command writes nothing.
  {
    auto inner = std::make_shared<DeclareRecord>(
        composite("BB", {Reference::ToAddress(A("AC"))}));
    declare::Result r = declare::execute(
        ctl, composite("BC", {Reference::ToAddress(A("AA")), Reference::ToNested(inner)}));
    check(r.outcomes.size() == 1 && !r.outcomes[0].ingested,
          "nested NULL-mass parent: outer command rejected");
    check(!ctl.token_exists(A("BB")) && !ctl.token_exists(A("BC")),
          "nested NULL-mass parent: neither inner nor outer token written");
  }

  // 2. All parents known -> succeeds as before.
  {
    declare::Result r = declare::execute(
        ctl, composite("BD", {Reference::ToAddress(A("AA")), Reference::ToAddress(A("AB"))}));
    check(r.outcomes.size() == 1 && r.outcomes[0].ingested &&
              r.outcomes[0].address == A("BD"),
          "known-mass parents: composite ingested");
    const auto parents = ctl.parents_of(A("BD"));
    check(parents.size() == 2 && parents[0].parent == A("AA") &&
              parents[1].parent == A("AB"),
          "known-mass parents: constituents linked in order");
  }

  // 3. A nested parent minted inline by the composite itself is exempt.
  {
    auto inner = std::make_shared<DeclareRecord>(
        composite("BE", {Reference::ToAddress(A("AA")), Reference::ToAddress(A("AB"))}));
    declare::Result r = declare::execute(
        ctl, composite("BF", {Reference::ToAddress(A("AB")), Reference::ToNested(inner)}));
    check(r.outcomes.size() == 1 && r.outcomes[0].ingested,
          "inline-nested parent: exempt from the guard" +
              (r.outcomes.empty() ? std::string() : " [" + r.outcomes[0].reason + "]"));
  }

  PQfinish(raw);
  std::printf("%s declare_core_mass_guard_test\n", g_failures == 0 ? "PASS" : "FAIL");
  return g_failures == 0 ? 0 : 1;
}
