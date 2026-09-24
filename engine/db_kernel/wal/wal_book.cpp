#include "wal_book.h"

#include <libpq-fe.h>

#include <stdexcept>
#include <string>

namespace wal {

namespace {

// Render a codec address as a Postgres text[] array literal, one couplet per
// element (e.g. {"AA","AB"}) -- same rendering discipline as
// controller.cpp's address_to_pg_array, EXCEPT here an EMPTY address is
// legitimate input: it is the mass obligation's addr_b sentinel
// (wal_schema.sql, W-1), not an error. controller.cpp's version rejects
// empty because hcp3_core addresses are never empty; that rule is specific
// to that schema, not this one (WAL-PLAN.md "Standing": own-schema
// latitude).
std::string address_to_pg_array(const codec::Address &addr) {
  if (!codec::is_valid_address(addr)) {
    throw std::runtime_error("address_to_pg_array: invalid address");
  }
  std::string out = "{";
  for (std::size_t i = 0; i < addr.size(); ++i) {
    if (i > 0) {
      out.push_back(',');
    }
    const codec::AddressElement &e = addr[i];
    out.push_back('"');
    out.push_back(codec::kAlphabet[e.first]);
    out.push_back(e.partial ? codec::kPartialMarker : codec::kAlphabet[e.second]);
    out.push_back('"');
  }
  out.push_back('}');
  return out;
}

// The dot-joined rendering of one WHOLE address (controller.cpp's
// address_to_text) -- used only for history.footprint, whose elements are
// each a whole token_id's dot-join, not a raw couplet array
// (wal_schema.sql's footprint column comment).
std::string address_to_text(const codec::Address &addr) {
  const auto text = codec::encode_token_id(addr);
  if (!text.has_value()) {
    throw std::runtime_error("address_to_text: invalid address");
  }
  return *text;
}

// Parse a dot-joined couplet string (array_to_string(col, '.')) back into a
// codec address -- same as controller.cpp's text_to_address. An empty
// string decodes to the empty Address (codec::decode_token_id's documented
// behaviour), which round-trips the mass addr_b sentinel correctly.
codec::Address text_to_address(const std::string &text) {
  const auto addr = codec::decode_token_id(text);
  if (!addr.has_value()) {
    throw std::runtime_error("text_to_address: undecodable token_id '" + text + "'");
  }
  return *addr;
}

// A text[] literal whose elements are each a WHOLE token's dot-joined text
// -- history.footprint's shape, distinct from address_to_pg_array's
// per-couplet rendering.
std::string footprint_array(const std::vector<codec::Address> &tokens) {
  std::string out = "{";
  for (std::size_t i = 0; i < tokens.size(); ++i) {
    if (i > 0) {
      out.push_back(',');
    }
    out.push_back('"');
    out += address_to_text(tokens[i]);
    out.push_back('"');
  }
  out.push_back('}');
  return out;
}

const char *kind_to_text(ObligationKind kind) {
  switch (kind) {
    case ObligationKind::kStructure:
      return "structure";
    case ObligationKind::kMembership:
      return "membership";
    case ObligationKind::kMass:
      return "mass";
  }
  throw std::runtime_error("kind_to_text: unknown ObligationKind");
}

ObligationKind text_to_kind(const std::string &text) {
  if (text == "structure") return ObligationKind::kStructure;
  if (text == "membership") return ObligationKind::kMembership;
  if (text == "mass") return ObligationKind::kMass;
  throw std::runtime_error("text_to_kind: unknown obligation kind '" + text + "'");
}

const char *scope_to_text(Scope scope) {
  switch (scope) {
    case Scope::kLocal:
      return "local";
    case Scope::kGlobal:
      return "global";
  }
  throw std::runtime_error("scope_to_text: unknown Scope");
}

// Run one parameterized statement. `params` holds the text-format parameter
// values; a nullptr entry is passed to Postgres as SQL NULL. On success the
// caller owns the returned PGresult and must PQclear it. Throws
// std::runtime_error on any command/tuple error, with the server's message
// -- same as controller.cpp's run().
PGresult *run(pg_conn *conn, const char *sql, const std::vector<const char *> &params) {
  PGresult *res = PQexecParams(conn, sql, static_cast<int>(params.size()),
                               /*paramTypes=*/nullptr, params.data(),
                               /*paramLengths=*/nullptr, /*paramFormats=*/nullptr,
                               /*resultFormat=*/0);
  const ExecStatusType st = PQresultStatus(res);
  if (st != PGRES_COMMAND_OK && st != PGRES_TUPLES_OK) {
    std::string msg = "query failed: ";
    msg += PQresultErrorMessage(res);
    PQclear(res);
    throw std::runtime_error(msg);
  }
  return res;
}

}  // namespace

WalBook::WalBook(const std::string &conninfo) : conn_(nullptr) {
  conn_ = PQconnectdb(conninfo.c_str());
  if (conn_ == nullptr || PQstatus(conn_) != CONNECTION_OK) {
    std::string msg = "WalBook: connection failed";
    if (conn_ != nullptr) {
      msg += ": ";
      msg += PQerrorMessage(conn_);
      PQfinish(conn_);
      conn_ = nullptr;
    }
    throw std::runtime_error(msg);
  }
}

WalBook::~WalBook() {
  if (conn_ != nullptr) {
    PQfinish(conn_);
    conn_ = nullptr;
  }
}

void WalBook::open(const Obligation &identity, const Source &source, Lsn lsn) {
  const std::string kind = kind_to_text(identity.kind);
  const std::string a = address_to_pg_array(identity.addr_a);
  const std::string b = address_to_pg_array(identity.addr_b);
  const std::string lsn_str = std::to_string(lsn);
  PGresult *res =
      run(conn_,
          "INSERT INTO obligation (kind, addr_a, addr_b, source, lsn) "
          "VALUES ($1, $2::text[], $3::text[], $4, $5) "
          "ON CONFLICT (kind, addr_a, addr_b) DO NOTHING",
          {kind.c_str(), a.c_str(), b.c_str(), source.c_str(), lsn_str.c_str()});
  PQclear(res);
}

void WalBook::close(const Obligation &identity) {
  const std::string kind = kind_to_text(identity.kind);
  const std::string a = address_to_pg_array(identity.addr_a);
  const std::string b = address_to_pg_array(identity.addr_b);
  PGresult *res = run(conn_,
                      "DELETE FROM obligation WHERE kind = $1 "
                      "AND addr_a = $2::text[] AND addr_b = $3::text[]",
                      {kind.c_str(), a.c_str(), b.c_str()});
  PQclear(res);
}

bool WalBook::is_open(const Obligation &identity) {
  const std::string kind = kind_to_text(identity.kind);
  const std::string a = address_to_pg_array(identity.addr_a);
  const std::string b = address_to_pg_array(identity.addr_b);
  PGresult *res = run(conn_,
                      "SELECT 1 FROM obligation WHERE kind = $1 "
                      "AND addr_a = $2::text[] AND addr_b = $3::text[]",
                      {kind.c_str(), a.c_str(), b.c_str()});
  const bool present = PQntuples(res) > 0;
  PQclear(res);
  return present;
}

std::vector<Obligation> WalBook::list_open(std::optional<ObligationKind> kind,
                                           std::optional<codec::Address> addr_a) {
  if (addr_a.has_value() && !kind.has_value()) {
    throw std::invalid_argument(
        "list_open: addr_a filter requires kind -- addr_a alone is not a "
        "PK-leading prefix");
  }

  PGresult *res;
  std::string kind_str;
  std::string a_str;
  if (kind.has_value() && addr_a.has_value()) {
    kind_str = kind_to_text(*kind);
    a_str = address_to_pg_array(*addr_a);
    res = run(conn_,
              "SELECT kind, array_to_string(addr_a, '.'), array_to_string(addr_b, '.') "
              "FROM obligation WHERE kind = $1 AND addr_a = $2::text[] "
              "ORDER BY addr_b",
              {kind_str.c_str(), a_str.c_str()});
  } else if (kind.has_value()) {
    kind_str = kind_to_text(*kind);
    res = run(conn_,
              "SELECT kind, array_to_string(addr_a, '.'), array_to_string(addr_b, '.') "
              "FROM obligation WHERE kind = $1 "
              "ORDER BY addr_a, addr_b",
              {kind_str.c_str()});
  } else {
    res = run(conn_,
              "SELECT kind, array_to_string(addr_a, '.'), array_to_string(addr_b, '.') "
              "FROM obligation "
              "ORDER BY kind, addr_a, addr_b",
              {});
  }

  std::vector<Obligation> out;
  const int n = PQntuples(res);
  out.reserve(static_cast<std::size_t>(n));
  for (int i = 0; i < n; ++i) {
    Obligation ob;
    ob.kind = text_to_kind(PQgetvalue(res, i, 0));
    ob.addr_a = text_to_address(PQgetvalue(res, i, 1));
    ob.addr_b = text_to_address(PQgetvalue(res, i, 2));
    out.push_back(std::move(ob));
  }
  PQclear(res);
  return out;
}

void WalBook::record_seen(const Report &report, const std::optional<Obligation> &settled) {
  std::vector<codec::Address> tokens;
  tokens.push_back(report.primary);
  for (const codec::Address &c : report.counterparts) {
    tokens.push_back(c);
  }
  const std::string footprint = footprint_array(tokens);
  const std::string lsn_str = std::to_string(report.lsn);
  const std::string scope = scope_to_text(report.scope);

  std::string settled_kind_str;
  std::string settled_a_str;
  std::string settled_b_str;
  const char *settled_kind_param = nullptr;
  const char *settled_a_param = nullptr;
  const char *settled_b_param = nullptr;
  if (settled.has_value()) {
    settled_kind_str = kind_to_text(settled->kind);
    settled_a_str = address_to_pg_array(settled->addr_a);
    settled_b_str = address_to_pg_array(settled->addr_b);
    settled_kind_param = settled_kind_str.c_str();
    settled_a_param = settled_a_str.c_str();
    settled_b_param = settled_b_str.c_str();
  }

  PGresult *res =
      run(conn_,
          "INSERT INTO history (source, lsn, footprint, scope, "
          "settled_kind, settled_addr_a, settled_addr_b) "
          "VALUES ($1, $2, $3::text[], $4, $5, $6::text[], $7::text[])",
          {report.source.c_str(), lsn_str.c_str(), footprint.c_str(), scope.c_str(),
           settled_kind_param, settled_a_param, settled_b_param});
  PQclear(res);
}

}  // namespace wal
