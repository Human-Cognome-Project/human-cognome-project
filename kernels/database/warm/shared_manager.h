#pragma once

#include <functional>

#include "cold_reader.h"
#include "toolbox.h"

namespace warm {
// A capability minted only by the shared publication path. Recovery takes this
// capability, not an arbitrary private selection ID supplied in a request.
class SharedTicket {
 public:
  const BundleRef &ref() const { return ref_; }

 private:
  friend class SharedManager;
  explicit SharedTicket(BundleRef ref) : ref_(std::move(ref)) {}
  BundleRef ref_;
};

// The embedding owner supplies a dedicated shared-only warm store and a
// resolver containing only public sources/credentials. Local active inputs
// have no API here. Rejection messages deliberately contain no supplied codes.
class SharedManager {
 public:
  struct Activity {
    std::size_t source_calls{0}, publications{0}, acquisitions{0},
        pinned_reads{0}, releases{0};
  };
  using Source = std::function<std::optional<ColdRecord>(const ObjectKey &)>;
  SharedManager(Toolbox &shared_store, std::vector<ObjectKey> public_prefixes,
                Source public_source);
  std::optional<ColdRecord> read(const ObjectKey &object);
  SharedTicket publish(Bundle explicitly_public_recipe,
                       std::optional<Uuid> expected_head = {});
  Acquired acquire(const SharedTicket &ticket, const Uuid &consumer);
  Acquired pinned(const SharedTicket &ticket, const Uuid &consumer);
  void release(const SharedTicket &ticket, const Uuid &consumer);
  const Activity &activity() const { return activity_; }

 private:
  void gate(const ObjectKey &object) const;
  void gate(const Bundle &bundle) const;
  Toolbox &store_;
  std::vector<ObjectKey> prefixes_;
  Source source_;
  Activity activity_;
};
}  // namespace warm
