#include "cold_reader.h"

#include <cmath>
#include <iomanip>
#include <sstream>

#include "pg.h"

namespace warm {
using namespace detail;
namespace {
codec::Address address_value(const std::string &text) {
  auto address = codec::decode_token_id(text);
  if (!address) throw Error("malformed stored cold address");
  address_text(*address);
  return *address;
}
}  // namespace
ColdReader::ColdReader(const std::string &conninfo)
    : pg_(std::make_unique<Pg>(conninfo, true)) {}
ColdReader::~ColdReader() = default;
bool ColdReader::read_only() {
  auto r = pg_->query("SHOW default_transaction_read_only");
  return cell(r, 0, 0) == "on";
}
std::optional<ColdRecord> ColdReader::read(const codec::Address &address) {
  const Params p{address_array(address)};
  Transaction tx(*pg_, true);
  auto r = pg_->query(
      "SELECT notation,mass FROM public.token WHERE token_id=$1::text[]", p);
  if (!PQntuples(r.get())) {
    tx.commit();
    return {};
  }
  ColdRecord record{address, nullable(r, 0, 0), mass(r, 0, 1), {}, {}};
  r = pg_->query(
      "SELECT array_to_string(parent_token_id,'.'),ordinal,mass "
      "FROM public.token_parent "
      "WHERE token_id=$1::text[] ORDER BY ordinal",
      p);
  for (int i = 0; i < PQntuples(r.get()); ++i) {
    const auto ordinal = std::stoull(cell(r, i, 1));
    if (ordinal > UINT32_MAX) throw Error("cold ordinal overflow");
    record.parents.push_back(
        {address_value(cell(r, i, 0)), std::uint32_t(ordinal), mass(r, i, 2)});
  }
  r = pg_->query(
      "SELECT array_to_string(group_token_id,'.') FROM public.member_of "
      "WHERE token_id=$1::text[] ORDER BY group_token_id",
      p);
  for (int i = 0; i < PQntuples(r.get()); ++i)
    record.member_of.push_back(address_value(cell(r, i, 0)));
  tx.commit();
  return record;
}
}  // namespace warm
