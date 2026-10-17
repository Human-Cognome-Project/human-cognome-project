#pragma once

#include <map>
#include <memory>
#include <utility>

#include "values.h"

namespace warm {
namespace detail {
class Pg;
}
using Links = std::vector<std::pair<ObjectKey, ObjectKey>>;
struct Projection {
  Links children, member_of, members;
};
struct Acquired {
  Bundle bundle;
  Projection lists;
};
class Prepared {
 public:
  const BundleRef &ref() const { return bundle_.ref; }

 private:
  friend class Toolbox;
  Prepared() = default;
  Bundle bundle_;
  std::vector<Binding> bindings_;
  std::map<ObjectKey, ToolRef> references_;
  Context preparer_;
};

// Only the embedding owner gets administrative connection credentials. It
// supplies a complete validated directory; configuration never allocates a
// production personal/temp address or accepts a request-selected writer role.
class Administrator {
 public:
  explicit Administrator(const std::string &conninfo);
  ~Administrator();
  void configure(std::vector<Binding> directory);
  std::vector<BundleRef> affected(const ObjectKey &prefix, const Uuid &epoch);

 private:
  std::unique_ptr<detail::Pg> pg_;
};
class Toolbox {
 public:
  Toolbox(const std::string &conninfo, Context trusted_context);
  ~Toolbox();
  Binding resolve(const ObjectKey &object);
  // Existing roots and their stored dependency closure are read, never claimed.
  // Omit their bodies from supplied_recipe; any overlapping supplied dependency
  // must agree with the exact stored revision/body. Authors remain unchanged.
  Prepared prepare(Bundle supplied_recipe,
                   const std::vector<ToolRef> &existing_roots = {});
  BundleRef publish(Prepared candidate, std::optional<Uuid> expected_head);
  // Optional exact head guard refuses before acquiring a different revision's
  // lease. Existing owned pins are never released to undo an unexpected read.
  Acquired acquire(const Uuid &selection, const Uuid &consumer,
                   std::optional<Uuid> expected_revision = {});
  Acquired read_pinned(const BundleRef &bundle, const Uuid &consumer);
  void release(const BundleRef &bundle, const Uuid &consumer);

 private:
  Binding resolve_impl(const ObjectKey &object);
  Tool read_tool(const ToolRef &tool);
  Acquired read_bundle(const BundleRef &bundle);
  std::vector<Binding> used_bindings(const Bundle &bundle);
  void read_authority(const ObjectKey &object) const;
  std::unique_ptr<detail::Pg> pg_;
  Context context_;
};
}  // namespace warm
