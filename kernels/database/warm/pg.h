#pragma once

#include <libpq-fe.h>

#include <cmath>
#include <iomanip>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "values.h"

namespace warm::detail {
using Params = std::vector<std::optional<std::string>>;
using Result = std::unique_ptr<PGresult, decltype(&PQclear)>;

class Pg {
 public:
  explicit Pg(const std::string &conninfo, bool read_only = false) {
    // Options after expanded dbname override any options in conninfo.
    const char *keys[] = {"dbname", read_only ? "options" : nullptr, nullptr};
    const char *values[] = {
        conninfo.c_str(),
        read_only ? "-c default_transaction_read_only=on" : "", nullptr};
    conn_ = PQconnectdbParams(keys, values, 1);
    if (!conn_ || PQstatus(conn_) != CONNECTION_OK) {
      if (conn_) PQfinish(conn_);
      conn_ = nullptr;
      throw Error(
          "PostgreSQL connection failed (connection details suppressed)");
    }
    try {
      command("SET search_path = pg_catalog, warm_toolbox");
    } catch (...) {
      PQfinish(conn_);
      conn_ = nullptr;
      throw;
    }
  }
  ~Pg() {
    if (conn_) PQfinish(conn_);
  }
  Pg(const Pg &) = delete;
  Pg &operator=(const Pg &) = delete;
  Result query(const std::string &sql, const Params &params = {}) {
    std::vector<const char *> values;
    for (const auto &p : params) values.push_back(p ? p->c_str() : nullptr);
    Result result(PQexecParams(conn_, sql.c_str(), int(values.size()), nullptr,
                               values.data(), nullptr, nullptr, 0),
                  PQclear);
    if (!result || (PQresultStatus(result.get()) != PGRES_COMMAND_OK &&
                    PQresultStatus(result.get()) != PGRES_TUPLES_OK)) {
      // SQLSTATE identifies a constraint/transaction failure without printing
      // server detail containing personal rows or connection information.
      const char *state =
          result ? PQresultErrorField(result.get(), PG_DIAG_SQLSTATE) : nullptr;
      throw Error(std::string("PostgreSQL statement failed: ") +
                  (state ? state : "connection"));
    }
    return result;
  }
  void command(const std::string &sql, const Params &params = {}) {
    query(sql, params);
  }

 private:
  PGconn *conn_{nullptr};
};
class Transaction {
 public:
  explicit Transaction(Pg &pg, bool read_only = false) : pg_(pg) {
    pg_.command(read_only ? "BEGIN ISOLATION LEVEL REPEATABLE READ READ ONLY"
                          : "BEGIN");
  }
  ~Transaction() {
    if (!done_) try {
        pg_.command("ROLLBACK");
      } catch (...) {
      }
  }
  void commit() {
    pg_.command("COMMIT");
    done_ = true;
  }

 private:
  Pg &pg_;
  bool done_{false};
};
inline std::string cell(const Result &r, int row, int col) {
  return PQgetvalue(r.get(), row, col);
}
inline std::optional<std::string> nullable(const Result &r, int row, int col) {
  if (PQgetisnull(r.get(), row, col)) return {};
  return cell(r, row, col);
}
inline ObjectKey object(const Result &r, int row, int col) {
  auto address = codec::decode_token_id(cell(r, row, col + 1));
  if (!address) throw Error("invalid stored address");
  ObjectKey o{Uuid(cell(r, row, col)), *address};
  key(o);
  return o;
}
inline ToolRef reference(const Result &r, int row, int col) {
  return {object(r, row, col), Uuid(cell(r, row, col + 2))};
}
inline void append(Params &p, const ObjectKey &o) {
  p.push_back(o.scope.str());
  p.push_back(address_array(o.address));
}
inline void append(Params &p, const ToolRef &r) {
  append(p, r.object);
  p.push_back(r.revision.str());
}
inline Params params(const ObjectKey &o) {
  Params p;
  append(p, o);
  return p;
}
inline Params params(const ToolRef &r) {
  Params p;
  append(p, r);
  return p;
}
inline Params params(const BundleRef &b) {
  return {b.selection.str(), b.revision.str()};
}
inline std::optional<std::string> number(std::optional<std::uint64_t> v) {
  if (v) return std::to_string(*v);
  return {};
}
inline std::optional<std::uint64_t> mass(const Result &r, int row, int col) {
  const auto text = nullable(r, row, col);
  if (!text) return {};
  if (text->empty() || text->front() == '-')
    throw Error("negative stored mass");
  return std::stoull(*text);
}
inline std::string real(double d) {
  if (!std::isfinite(d)) throw Error("non-finite initial value");
  std::ostringstream out;
  out << std::setprecision(17) << d;
  return out.str();
}
inline void append(Params &p, Point v) {
  p.push_back(real(v.x));
  p.push_back(real(v.y));
  p.push_back(real(v.z));
}
inline Point point(const Result &r, int row, int col) {
  return {std::stod(cell(r, row, col)), std::stod(cell(r, row, col + 1)),
          std::stod(cell(r, row, col + 2))};
}
inline std::string path_array(const std::vector<std::uint32_t> &path) {
  std::string out = "{";
  for (auto value : path) {
    if (out.size() > 1) out += ',';
    out += std::to_string(value);
  }
  return out + "}";
}
inline OccurrenceId occurrence_id(const Result &r, int row, int col) {
  OccurrenceId id{std::stoull(cell(r, row, col)), {}};
  std::istringstream path(cell(r, row, col + 1));
  std::string value;
  while (std::getline(path, value, ','))
    if (!value.empty()) {
      const auto n = std::stoull(value);
      if (n > UINT32_MAX) throw Error("stored path overflow");
      id.path.push_back(std::uint32_t(n));
    }
  return id;
}
inline void append(Params &p, const OccurrenceId &id) {
  p.push_back(std::to_string(id.entry));
  p.push_back(path_array(id.path));
}
}  // namespace warm::detail
