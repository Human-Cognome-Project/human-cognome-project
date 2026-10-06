// Check harness for Controller::membership_present (pair probe, both
// directions independently). Same tiny check-macro style as
// controller_txn_test.cpp.
//
// Takes a libpq conninfo as argv[1] and REFUSES to run unless its dbname
// starts with "hcp_test_" -- it never touches hcp_core. It resets nothing:
// run_membership_present_test.sh creates the throwaway database and applies
// the schema. One-sided pairs are built by raw SQL (test construction only).
#include <libpq-fe.h>

#include <cstdio>
#include <cstdlib>
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

void exec(PGconn *conn, const char *sql) {
  PGresult *res = PQexec(conn, sql);
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    std::fprintf(stderr, "test bug: %s: %s\n", sql, PQerrorMessage(conn));
    std::exit(2);
  }
  PQclear(res);
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

  // Tokens: M (member), G (group), and two more for the one-sided pairs.
  for (const char *t : {"AA", "AB", "AC", "AD", "AE"}) {
    ctl.mint(A(t), t, {}, 1);
  }

  // Present both ways: add_membership writes both directions.
  ctl.add_membership(A("AA"), A("AB"));
  auto p = ctl.membership_present(A("AA"), A("AB"));
  check(p.in_member_of && p.in_members, "whole pair: {true, true}");

  // Direction matters: the reversed pair is absent in both tables.
  p = ctl.membership_present(A("AB"), A("AA"));
  check(!p.in_member_of && !p.in_members, "reversed pair: {false, false}");

  // Absent pair between live tokens.
  p = ctl.membership_present(A("AC"), A("AD"));
  check(!p.in_member_of && !p.in_members, "absent pair: {false, false}");

  // Absent pair naming a token that does not exist is simply absent.
  p = ctl.membership_present(A("ZZ"), A("AB"));
  check(!p.in_member_of && !p.in_members, "unknown token: {false, false}");

  // One-sided, member_of only (raw SQL).
  exec(raw, "INSERT INTO member_of (token_id, group_token_id) VALUES ('{AC}', '{AD}')");
  p = ctl.membership_present(A("AC"), A("AD"));
  check(p.in_member_of && !p.in_members, "member_of only: {true, false}");

  // One-sided, members only (raw SQL).
  exec(raw, "INSERT INTO members (token_id, member_token_id) VALUES ('{AE}', '{AC}')");
  p = ctl.membership_present(A("AC"), A("AE"));
  check(!p.in_member_of && p.in_members, "members only: {false, true}");

  // The probe is read-only: nothing healed, the one-sided pairs stay one-sided.
  p = ctl.membership_present(A("AC"), A("AD"));
  check(p.in_member_of && !p.in_members, "probe heals nothing (member_of only stays)");
  p = ctl.membership_present(A("AC"), A("AE"));
  check(!p.in_member_of && p.in_members, "probe heals nothing (members only stays)");

  // A second group on the same member does not disturb the first pair's probe.
  ctl.add_membership(A("AA"), A("AD"));
  p = ctl.membership_present(A("AA"), A("AB"));
  check(p.in_member_of && p.in_members, "second group: first pair still {true, true}");
  p = ctl.membership_present(A("AA"), A("AD"));
  check(p.in_member_of && p.in_members, "second group: new pair {true, true}");

  PQfinish(raw);
  std::printf("%s controller_membership_present_test\n", g_failures == 0 ? "PASS" : "FAIL");
  return g_failures == 0 ? 0 : 1;
}
