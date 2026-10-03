// Check harness for the command-level transaction scope (#110) in Controller.
// Same tiny check-macro style as codec/codec_test.cpp.
//
// Takes a libpq conninfo as argv[1] and REFUSES to run unless its dbname
// starts with "hcp_test_" -- it never touches hcp_core. It resets nothing:
// run_txn_test.sh creates the throwaway database and applies the schema.
#include <libpq-fe.h>

#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>

#include "codec.h"
#include "controller.h"

namespace {

int g_failures = 0;

void check(bool ok, const std::string &what) {
  std::printf("%s %s\n", ok ? "ok  " : "FAIL", what.c_str());
  if (!ok) {
    ++g_failures;
  }
}

codec::Address A(const std::string &token_id) {
  const auto a = codec::decode_token_id(token_id);
  if (!a.has_value()) {
    std::fprintf(stderr, "test bug: undecodable token_id '%s'\n", token_id.c_str());
    std::exit(2);
  }
  return *a;
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
  const char *kMembership =
      "SELECT (SELECT count(*) FROM member_of) + (SELECT count(*) FROM members)";

  // 3. Standalone mint (no scope) still commits on its own.
  check(!ctl.in_transaction(), "standalone: no scope active");
  check(!ctl.mint(A("AA"), "standalone", {}, 1), "standalone mint: freshly minted");
  check(ctl.token_exists(A("AA")), "standalone mint: token_exists");
  check(count(raw, "SELECT count(*) FROM token") == 1,
        "standalone mint: raw connection sees the row");
  check(ctl.mint(A("AA"), "standalone", {}, 1), "standalone mint: re-mint is idempotent no-op");

  // 1. Command {mint; add_membership} with add_membership forced to fail
  // (the group does not exist -> FK violation): full rollback.
  const long tokens_before = count(raw, "SELECT count(*) FROM token");
  const long parents_before = count(raw, "SELECT count(*) FROM token_parent");
  const long children_before = count(raw, "SELECT count(*) FROM token_child");
  const long membership_before = count(raw, kMembership);
  bool threw = false;
  try {
    ctl.with_transaction([&] {
      ctl.mint(A("AB"), "rolled-back", {{A("AA"), 1}}, 1);
      ctl.add_membership(A("AB"), A("ZZ"));  // ZZ was never minted
    });
  } catch (const std::runtime_error &) {
    threw = true;
  }
  check(threw, "failing command: error propagates");
  check(!ctl.in_transaction(), "failing command: scope closed");
  check(!ctl.token_exists(A("AB")), "failing command: no token row");
  check(count(raw, "SELECT count(*) FROM token") == tokens_before &&
            count(raw, "SELECT count(*) FROM token_parent") == parents_before &&
            count(raw, "SELECT count(*) FROM token_child") == children_before,
        "failing command: token/parent/child row counts unchanged");
  check(count(raw, kMembership) == membership_before,
        "failing command: no membership rows");
  // The connection is usable afterwards (not stuck in an aborted transaction).
  check(!ctl.mint(A("AC"), "after-rollback", {}, 1), "failing command: connection usable afterwards");

  // 2. Valid command {mint; add_membership}: both committed atomically.
  ctl.with_transaction([&] {
    ctl.mint(A("AD"), "member", {}, 1);
    ctl.add_membership(A("AD"), A("AA"));
    check(ctl.in_transaction(), "valid command: scope active during the command");
    check(count(raw, "SELECT count(*) FROM token WHERE notation = 'member'") == 0,
          "valid command: uncommitted rows invisible to another connection");
  });
  check(!ctl.in_transaction(), "valid command: scope closed");
  check(ctl.token_exists(A("AD")), "valid command: token committed");
  check(ctl.member_of(A("AD")).size() == 1 && ctl.members_of(A("AA")).size() == 1,
        "valid command: both membership directions committed");
  check(count(raw, kMembership) == membership_before + 2,
        "valid command: membership row counts +2");

  // Explicit begin/rollback leaves nothing; misuse is refused.
  ctl.begin();
  ctl.mint(A("AE"), "explicit", {}, 1);
  ctl.rollback();
  check(!ctl.token_exists(A("AE")), "explicit rollback: nothing written");
  ctl.begin();
  bool nested = false;
  try {
    ctl.begin();
  } catch (const std::runtime_error &) {
    nested = true;
  }
  ctl.rollback();
  check(nested, "nested begin: refused");
  bool bare_commit = false;
  try {
    ctl.commit();
  } catch (const std::runtime_error &) {
    bare_commit = true;
  }
  check(bare_commit, "commit with no scope: refused");

  // Standalone failure still rolls back its own transaction.
  bool standalone_threw = false;
  try {
    ctl.add_membership(A("AA"), A("ZZ"));
  } catch (const std::runtime_error &) {
    standalone_threw = true;
  }
  check(standalone_threw && count(raw, kMembership) == membership_before + 2,
        "standalone add_membership failure: own rollback, nothing written");

  PQfinish(raw);
  std::printf("%s controller_txn_test\n", g_failures == 0 ? "PASS" : "FAIL");
  return g_failures == 0 ? 0 : 1;
}
