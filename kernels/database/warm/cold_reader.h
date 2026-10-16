#pragma once

#include <memory>

#include "values.h"

namespace warm {
namespace detail {
class Pg;
}
struct ColdParent {
  codec::Address address;
  std::uint32_t ordinal;
  std::optional<std::uint64_t> stored_mass;
};
struct ColdRecord {
  codec::Address address;
  std::optional<std::string> notation;
  std::optional<std::uint64_t> mass;
  std::vector<ColdParent> parents;
  std::vector<codec::Address> member_of;
};
// Addressed reads only. Server-enforced read-only sessions, including when the
// embedding owner supplies a conninfo that normally permits writing.
class ColdReader {
 public:
  explicit ColdReader(const std::string &conninfo);
  ~ColdReader();
  bool read_only();
  std::optional<ColdRecord> read(const codec::Address &address);

 private:
  std::unique_ptr<detail::Pg> pg_;
};
}  // namespace warm
