#include "controller.h"

#include <libpq-fe.h>

#include <set>
#include <stdexcept>
#include <string>

namespace dbk {

namespace {

// Render a codec address as a Postgres text[] array literal, one couplet per
// element, each element double-quoted (e.g. {"AA","AB"}). Couplets are drawn
// from the base-50 alphabet plus the partial marker '*', none of which need
// escaping, so plain double-quoting is enough. Throws if the address is not a
// valid codec address.
std::string address_to_pg_array(const codec::Address &addr) {
  if (!codec::is_valid_address(addr) || addr.empty()) {
    throw std::runtime_error("address_to_pg_array: invalid or empty address");
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

// The dot-joined human/debug rendering of an address, for token.token_text.
std::string address_to_text(const codec::Address &addr) {
  const auto text = codec::encode_token_id(addr);
  if (!text.has_value()) {
    throw std::runtime_error("address_to_text: invalid address");
  }
  return *text;
}

// Parse a dot-joined couplet string (as returned by array_to_string(col,'.'))
// back into a codec address. Throws if it does not decode.
codec::Address text_to_address(const std::string &text) {
  const auto addr = codec::decode_token_id(text);
  if (!addr.has_value()) {
    throw std::runtime_error("text_to_address: undecodable token_id '" + text + "'");
  }
  return *addr;
}

// Increments `addr` in place as a base-50 counter over full couplets — the
// second character increments fastest, carrying into the first character,
// then carrying left into the preceding element — used only to compute
// gather's exclusive upper bound (a private implementation detail of this
// primitive, not a general span-arithmetic export; the span planner's own
// address-successor helper, per PLAN.md II.2, is a separate later module).
// Every element touched must already be full (non-partial); gather never
// calls this on a wildcard element itself. Returns false if every element
// was already at the alphabet's maximum, i.e. `addr` has no successor —
// gather leaves its upper bound open in that case (there is nothing
// addressable beyond it).
bool increment_full_address(codec::Address &addr) {
  for (std::size_t i = addr.size(); i-- > 0;) {
    codec::AddressElement &e = addr[i];
    if (e.second + 1 < codec::kAlphabetSize) {
      ++e.second;
      return true;
    }
    e.second = 0;
    if (e.first + 1 < codec::kAlphabetSize) {
      ++e.first;
      return true;
    }
    e.first = 0;
    // This element wrapped to its minimum; carry into the element to the
    // left (the loop continues) or, if this was the leftmost, overflow.
  }
  return false;
}

// The [low, high) bounds for a gather() over a terminal-wildcard address.
// `high` is nullopt when the wildcard's trunk is the last one reachable
// (no successor exists) — gather then leaves the range open-ended.
struct WildcardBounds {
  codec::Address low;
  std::optional<codec::Address> high;
};

// `w` must be a valid, non-empty address whose last element is partial
// (checked by the caller). `low` is the smallest full address the trunk
// contains: the fixed leading elements plus a full couplet built from the
// wildcard's fixed first character and the alphabet's minimum second
// character. `high`, when present, is the smallest full address of the
// NEXT trunk at the same position — exclusive, and it also bounds every
// address nested deeper under this trunk, because array comparison orders
// a shared prefix's longer extension strictly after the point the two
// arrays first diverge (see NOTES.md "Gather primitive"). When the
// wildcard's first character is already the alphabet's last, the next
// trunk carries into the fixed leading elements (incremented as a base-50
// counter); if those are exhausted too (or the wildcard has no leading
// elements at all), there is no trunk above this one and the range is left
// open (`high` stays nullopt).
WildcardBounds wildcard_bounds(const codec::Address &w) {
  WildcardBounds b;
  const codec::AddressElement &last = w.back();

  b.low.assign(w.begin(), w.end() - 1);
  b.low.push_back(codec::AddressElement{last.first, 0, false});

  if (last.first + 1 < codec::kAlphabetSize) {
    codec::Address high(w.begin(), w.end() - 1);
    high.push_back(codec::AddressElement{static_cast<uint8_t>(last.first + 1), 0, false});
    b.high = std::move(high);
  } else if (w.size() > 1) {
    codec::Address prefix(w.begin(), w.end() - 1);
    if (increment_full_address(prefix)) {
      b.high = std::move(prefix);
    }
    // else: the leading elements are also exhausted -> open-ended.
  }
  // else: no leading elements and the wildcard's first char is already the
  // alphabet's last -> this is the very last trunk in the address space,
  // open-ended.
  return b;
}

}  // namespace

Controller::Controller(const std::string &conninfo) : conn_(nullptr) {
  conn_ = PQconnectdb(conninfo.c_str());
  if (conn_ == nullptr || PQstatus(conn_) != CONNECTION_OK) {
    std::string msg = "Controller: connection failed";
    if (conn_ != nullptr) {
      msg += ": ";
      msg += PQerrorMessage(conn_);
      PQfinish(conn_);
      conn_ = nullptr;
    }
    throw std::runtime_error(msg);
  }
}

Controller::~Controller() {
  if (conn_ != nullptr) {
    PQfinish(conn_);
    conn_ = nullptr;
  }
}

namespace {

// Run one parameterized statement. `params` holds the text-format parameter
// values; a nullptr entry is passed to Postgres as SQL NULL. On success the
// caller owns the returned PGresult and must PQclear it. Throws
// std::runtime_error on any command/tuple error, with the server's message.
PGresult *run(pg_conn *conn, const char *sql,
              const std::vector<const char *> &params) {
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

// ---- Read side ------------------------------------------------------------

bool Controller::token_exists(const codec::Address &token_id) {
  const std::string arr = address_to_pg_array(token_id);
  PGresult *res =
      run(conn_, "SELECT 1 FROM token WHERE token_id = $1::text[]", {arr.c_str()});
  const bool present = PQntuples(res) > 0;
  PQclear(res);
  return present;
}

std::vector<ParentEntry> Controller::parents_of(const codec::Address &token_id) {
  const std::string arr = address_to_pg_array(token_id);
  PGresult *res = run(conn_,
                      "SELECT ordinal, array_to_string(parent_token_id, '.'), mass "
                      "FROM token_parent WHERE token_id = $1::text[] "
                      "ORDER BY ordinal",
                      {arr.c_str()});
  std::vector<ParentEntry> out;
  const int n = PQntuples(res);
  out.reserve(static_cast<std::size_t>(n));
  for (int i = 0; i < n; ++i) {
    ParentEntry e;
    e.ordinal = std::stoi(PQgetvalue(res, i, 0));
    e.parent = text_to_address(PQgetvalue(res, i, 1));
    if (!PQgetisnull(res, i, 2)) {
      e.mass = std::stoi(PQgetvalue(res, i, 2));
    }
    out.push_back(std::move(e));
  }
  PQclear(res);
  return out;
}

std::vector<codec::Address> Controller::children_of(const codec::Address &token_id) {
  const std::string arr = address_to_pg_array(token_id);
  PGresult *res = run(conn_,
                      "SELECT array_to_string(child_token_id, '.') "
                      "FROM token_child WHERE token_id = $1::text[] "
                      "ORDER BY child_token_id",
                      {arr.c_str()});
  std::vector<codec::Address> out;
  const int n = PQntuples(res);
  out.reserve(static_cast<std::size_t>(n));
  for (int i = 0; i < n; ++i) {
    out.push_back(text_to_address(PQgetvalue(res, i, 0)));
  }
  PQclear(res);
  return out;
}

std::vector<codec::Address> Controller::members_of(const codec::Address &token_id) {
  const std::string arr = address_to_pg_array(token_id);
  PGresult *res = run(conn_,
                      "SELECT array_to_string(member_token_id, '.') "
                      "FROM members WHERE token_id = $1::text[] "
                      "ORDER BY member_token_id",
                      {arr.c_str()});
  std::vector<codec::Address> out;
  const int n = PQntuples(res);
  out.reserve(static_cast<std::size_t>(n));
  for (int i = 0; i < n; ++i) {
    out.push_back(text_to_address(PQgetvalue(res, i, 0)));
  }
  PQclear(res);
  return out;
}

std::vector<codec::Address> Controller::member_of(const codec::Address &token_id) {
  const std::string arr = address_to_pg_array(token_id);
  PGresult *res = run(conn_,
                      "SELECT array_to_string(group_token_id, '.') "
                      "FROM member_of WHERE token_id = $1::text[] "
                      "ORDER BY group_token_id",
                      {arr.c_str()});
  std::vector<codec::Address> out;
  const int n = PQntuples(res);
  out.reserve(static_cast<std::size_t>(n));
  for (int i = 0; i < n; ++i) {
    out.push_back(text_to_address(PQgetvalue(res, i, 0)));
  }
  PQclear(res);
  return out;
}

std::optional<TokenAttributes> Controller::attributes_of(
    const codec::Address &token_id) {
  const std::string arr = address_to_pg_array(token_id);
  PGresult *res =
      run(conn_, "SELECT notation, mass FROM token WHERE token_id = $1::text[]",
          {arr.c_str()});
  std::optional<TokenAttributes> out;
  if (PQntuples(res) > 0) {
    TokenAttributes attrs;
    if (!PQgetisnull(res, 0, 0)) {
      attrs.notation = PQgetvalue(res, 0, 0);
    }
    if (!PQgetisnull(res, 0, 1)) {
      attrs.mass = std::stoi(PQgetvalue(res, 0, 1));
    }
    out = std::move(attrs);
  }
  PQclear(res);
  return out;
}

std::vector<codec::Address> Controller::gather(const codec::Address &prefix) {
  if (!codec::is_valid_address(prefix) || prefix.empty() || !prefix.back().partial) {
    throw std::runtime_error(
        "gather: prefix must be a valid, non-empty terminal-wildcard address");
  }
  const WildcardBounds bounds = wildcard_bounds(prefix);
  const std::string low_arr = address_to_pg_array(bounds.low);

  PGresult *res;
  std::string high_arr;  // kept alive across the run() call below
  if (bounds.high.has_value()) {
    high_arr = address_to_pg_array(*bounds.high);
    res = run(conn_,
              "SELECT array_to_string(token_id, '.') FROM token "
              "WHERE token_id >= $1::text[] AND token_id < $2::text[] "
              "ORDER BY token_id",
              {low_arr.c_str(), high_arr.c_str()});
  } else {
    res = run(conn_,
              "SELECT array_to_string(token_id, '.') FROM token "
              "WHERE token_id >= $1::text[] "
              "ORDER BY token_id",
              {low_arr.c_str()});
  }
  std::vector<codec::Address> out;
  const int n = PQntuples(res);
  out.reserve(static_cast<std::size_t>(n));
  for (int i = 0; i < n; ++i) {
    out.push_back(text_to_address(PQgetvalue(res, i, 0)));
  }
  PQclear(res);
  return out;
}

std::vector<codec::Address> Controller::gather(const codec::Address &from,
                                               const codec::Address &to) {
  const auto is_full = [](const codec::Address &a) {
    if (!codec::is_valid_address(a) || a.empty()) {
      return false;
    }
    for (const codec::AddressElement &e : a) {
      if (e.partial) {
        return false;
      }
    }
    return true;
  };
  if (!is_full(from)) {
    throw std::runtime_error("gather: from must be a valid, fully-specified address");
  }
  if (!is_full(to)) {
    throw std::runtime_error("gather: to must be a valid, fully-specified address");
  }

  const std::string from_arr = address_to_pg_array(from);
  const std::string to_arr = address_to_pg_array(to);
  PGresult *res = run(conn_,
                      "SELECT array_to_string(token_id, '.') FROM token "
                      "WHERE token_id >= $1::text[] AND token_id <= $2::text[] "
                      "ORDER BY token_id",
                      {from_arr.c_str(), to_arr.c_str()});
  std::vector<codec::Address> out;
  const int n = PQntuples(res);
  out.reserve(static_cast<std::size_t>(n));
  for (int i = 0; i < n; ++i) {
    out.push_back(text_to_address(PQgetvalue(res, i, 0)));
  }
  PQclear(res);
  return out;
}

// ---- Write side -----------------------------------------------------------

bool Controller::mint(const codec::Address &token_id, const std::string &notation,
                      const std::vector<Constituent> &constituents,
                      std::optional<int> mass) {
  const std::string composite_arr = address_to_pg_array(token_id);

  PGresult *begun = run(conn_, "BEGIN", {});
  PQclear(begun);

  try {
    // SEE: idempotent if the token_id already exists.
    PGresult *seen = run(conn_, "SELECT 1 FROM token WHERE token_id = $1::text[]",
                         {composite_arr.c_str()});
    const bool existed = PQntuples(seen) > 0;
    PQclear(seen);
    if (existed) {
      PGresult *committed = run(conn_, "COMMIT", {});
      PQclear(committed);
      return true;
    }

    // token_text is the dot-joined rendering of the identity, derived here.
    const std::string text = address_to_text(token_id);

    // Optional pass-through attribute: absent mass -> SQL NULL, never a
    // stand-in 0. Stored verbatim; nothing is computed from the
    // constituents here. No `type`/kind is written — the column is gone.
    std::string mass_str;
    const char *mass_param = nullptr;
    if (mass.has_value()) {
      mass_str = std::to_string(*mass);
      mass_param = mass_str.c_str();
    }

    // MINT: the token row itself.
    PGresult *minted =
        run(conn_,
            "INSERT INTO token (token_id, token_text, notation, mass) "
            "VALUES ($1::text[], $2, $3, $4)",
            {composite_arr.c_str(), text.c_str(), notation.c_str(), mass_param});
    PQclear(minted);

    // LINK: one ordered token_parent row per constituent. WIRE (below) needs
    // the distinct parents; collect them here so a constituent repeated at
    // several ordinals yields a single reverse edge.
    std::set<std::string> distinct_parents;
    for (std::size_t i = 0; i < constituents.size(); ++i) {
      const std::string parent_arr = address_to_pg_array(constituents[i].address);
      const std::string ordinal = std::to_string(i);  // 0-based (OPEN TEST SLOT 2)
      // Per-element mass: absent -> SQL NULL, never a stand-in 0.
      std::string elem_mass_str;
      const char *elem_mass_param = nullptr;
      if (constituents[i].mass.has_value()) {
        elem_mass_str = std::to_string(*constituents[i].mass);
        elem_mass_param = elem_mass_str.c_str();
      }
      PGresult *linked =
          run(conn_,
              "INSERT INTO token_parent (token_id, ordinal, parent_token_id, mass) "
              "VALUES ($1::text[], $2, $3::text[], $4)",
              {composite_arr.c_str(), ordinal.c_str(), parent_arr.c_str(),
               elem_mass_param});
      PQclear(linked);
      distinct_parents.insert(parent_arr);
    }

    // WIRE: fold AND wire — point each distinct parent back at the composite.
    for (const std::string &parent_arr : distinct_parents) {
      PGresult *wired =
          run(conn_,
              "INSERT INTO token_child (token_id, child_token_id) "
              "VALUES ($1::text[], $2::text[])",
              {parent_arr.c_str(), composite_arr.c_str()});
      PQclear(wired);
    }

    PGresult *committed = run(conn_, "COMMIT", {});
    PQclear(committed);
    return false;
  } catch (...) {
    // Best-effort rollback; ignore its own status so the original error wins.
    PGresult *rolled = PQexec(conn_, "ROLLBACK");
    if (rolled != nullptr) {
      PQclear(rolled);
    }
    throw;
  }
}

void Controller::add_membership(const codec::Address &member,
                                const codec::Address &group) {
  const std::string member_arr = address_to_pg_array(member);
  const std::string group_arr = address_to_pg_array(group);

  PGresult *begun = run(conn_, "BEGIN", {});
  PQclear(begun);
  try {
    // Idempotent, SEE-style: ON CONFLICT DO NOTHING on each side, so
    // re-adding an already-present pair is a clean no-op rather than a
    // duplicate-key error, and either side can independently heal a
    // one-sided/inconsistent row without disturbing the other.
    //
    // member_of: member -> group (upward).
    PGresult *mo = run(conn_,
                       "INSERT INTO member_of (token_id, group_token_id) "
                       "VALUES ($1::text[], $2::text[]) "
                       "ON CONFLICT DO NOTHING",
                       {member_arr.c_str(), group_arr.c_str()});
    PQclear(mo);
    // members: group -> member (downward), the reciprocal.
    PGresult *mm = run(conn_,
                       "INSERT INTO members (token_id, member_token_id) "
                       "VALUES ($1::text[], $2::text[]) "
                       "ON CONFLICT DO NOTHING",
                       {group_arr.c_str(), member_arr.c_str()});
    PQclear(mm);
    PGresult *committed = run(conn_, "COMMIT", {});
    PQclear(committed);
  } catch (...) {
    PGresult *rolled = PQexec(conn_, "ROLLBACK");
    if (rolled != nullptr) {
      PQclear(rolled);
    }
    throw;
  }
}

void Controller::rekey(const codec::Address &old_id, const codec::Address &new_id) {
  const std::string old_arr = address_to_pg_array(old_id);
  const std::string new_arr = address_to_pg_array(new_id);
  const std::string new_text = address_to_text(new_id);

  PGresult *begun = run(conn_, "BEGIN", {});
  PQclear(begun);
  try {
    // Read the old row's carry-over attributes (notation, mass); also
    // confirms old_id exists.
    PGresult *old_row =
        run(conn_, "SELECT notation, mass FROM token WHERE token_id = $1::text[]",
            {old_arr.c_str()});
    if (PQntuples(old_row) == 0) {
      PQclear(old_row);
      throw std::runtime_error("rekey: old_id does not exist");
    }
    const bool notation_null = PQgetisnull(old_row, 0, 0);
    const std::string notation = notation_null ? "" : PQgetvalue(old_row, 0, 0);
    const bool mass_null = PQgetisnull(old_row, 0, 1);
    const std::string mass_str = mass_null ? "" : PQgetvalue(old_row, 0, 1);
    PQclear(old_row);

    // Rekeying onto a token_id that already exists would merge two
    // identities into one address — forbidden ("no aliasing/forwarding",
    // no MERGE).
    PGresult *new_seen =
        run(conn_, "SELECT 1 FROM token WHERE token_id = $1::text[]", {new_arr.c_str()});
    const bool new_exists = PQntuples(new_seen) > 0;
    PQclear(new_seen);
    if (new_exists) {
      throw std::runtime_error("rekey: new_id already exists (would merge identities)");
    }

    // INSERT the new token_id row first (FK-safe ordering): every
    // referencing row below is repointed to a new_id that already exists,
    // so no referencing row is ever left pointing at a nonexistent parent.
    PGresult *inserted =
        run(conn_,
            "INSERT INTO token (token_id, token_text, notation, mass) "
            "VALUES ($1::text[], $2, $3, $4)",
            {new_arr.c_str(), new_text.c_str(),
             notation_null ? nullptr : notation.c_str(),
             mass_null ? nullptr : mass_str.c_str()});
    PQclear(inserted);

    // Cascade-repoint every referencing row across all four relationship
    // stores, both columns where old_id could appear.
    const struct {
      const char *table;
      const char *column;
    } repoints[] = {
        {"token_parent", "token_id"},        {"token_parent", "parent_token_id"},
        {"token_child", "token_id"},         {"token_child", "child_token_id"},
        {"members", "token_id"},             {"members", "member_token_id"},
        {"member_of", "token_id"},           {"member_of", "group_token_id"},
    };
    for (const auto &r : repoints) {
      const std::string sql = std::string("UPDATE ") + r.table + " SET " + r.column +
                              " = $1::text[] WHERE " + r.column + " = $2::text[]";
      PGresult *updated = run(conn_, sql.c_str(), {new_arr.c_str(), old_arr.c_str()});
      PQclear(updated);
    }

    // DELETE the old row last: by now nothing references it, so this is
    // safe under the database's default (no-cascade) FK behaviour.
    PGresult *deleted =
        run(conn_, "DELETE FROM token WHERE token_id = $1::text[]", {old_arr.c_str()});
    PQclear(deleted);

    PGresult *committed = run(conn_, "COMMIT", {});
    PQclear(committed);
  } catch (...) {
    PGresult *rolled = PQexec(conn_, "ROLLBACK");
    if (rolled != nullptr) {
      PQclear(rolled);
    }
    throw;
  }
}

void Controller::delete_token(const codec::Address &id) {
  const std::string arr = address_to_pg_array(id);
  // No transaction needed: a single statement. If other rows still
  // reference this token_id (token_parent/token_child/members/member_of),
  // the database's default FK behaviour applies as-is and run() throws —
  // no cascade/repoint policy is invented here (open seam G10).
  PGresult *res =
      run(conn_, "DELETE FROM token WHERE token_id = $1::text[]", {arr.c_str()});
  PQclear(res);
}

void Controller::delete_pair(const codec::Address &member, const codec::Address &group) {
  const std::string member_arr = address_to_pg_array(member);
  const std::string group_arr = address_to_pg_array(group);

  PGresult *begun = run(conn_, "BEGIN", {});
  PQclear(begun);
  try {
    // member_of: remove the (member, group) row.
    PGresult *mo = run(conn_,
                       "DELETE FROM member_of WHERE token_id = $1::text[] "
                       "AND group_token_id = $2::text[]",
                       {member_arr.c_str(), group_arr.c_str()});
    PQclear(mo);
    // members: remove the stored reciprocal (group, member) row.
    PGresult *mm = run(conn_,
                       "DELETE FROM members WHERE token_id = $1::text[] "
                       "AND member_token_id = $2::text[]",
                       {group_arr.c_str(), member_arr.c_str()});
    PQclear(mm);
    PGresult *committed = run(conn_, "COMMIT", {});
    PQclear(committed);
  } catch (...) {
    PGresult *rolled = PQexec(conn_, "ROLLBACK");
    if (rolled != nullptr) {
      PQclear(rolled);
    }
    throw;
  }
}

}  // namespace dbk
