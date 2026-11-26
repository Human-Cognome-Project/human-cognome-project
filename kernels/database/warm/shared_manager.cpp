#include "shared_manager.h"

#include <map>
#include <set>

namespace warm {
SharedManager::SharedManager(Toolbox &store, std::vector<ObjectKey> prefixes,
                             Source source)
    : store_(store),
      prefixes_(std::move(prefixes)),
      source_(std::move(source)) {
  if (prefixes_.empty() || !source_)
    throw Unauthorized("shared sources required");
  for (const auto &p : prefixes_) {
    // No private/temporary prefix families or unassigned roots are delegated.
    if (!p.scope.nil() || p.address.empty() ||
        (address_text(p.address).substr(0, 2) != "00" &&
         address_text(p.address).substr(0, 2) != "02"))
      throw Unauthorized("shared source authority required");
  }
}
void SharedManager::gate(const ObjectKey &object) const {
  // Scope is checked BEFORE even consulting the public prefix directory.
  if (!object.scope.nil()) throw Unauthorized("shared scope required");
  for (const auto &p : prefixes_)
    if (contains(p, object)) return;
  throw Unauthorized("outside shared source authority");
}
void SharedManager::gate(const Bundle &b) const {
  for (const auto &t : b.tools) {
    gate(t.ref.object);
    for (const auto &p : t.parents) gate(p.target.object);
    for (const auto &g : t.member_of) gate(g.object);
  }
  for (const auto &g : b.geometry) gate(g.object);
  for (const auto &o : b.occurrences) gate(o.definition);
  for (const auto &o : b.structural_occurrences)
    gate(o.definition);
  for (const auto &f : b.focal_fields)
    gate(f.object);
}
std::optional<ColdRecord> SharedManager::read(const ObjectKey &object) {
  gate(object);
  ++activity_.source_calls;
  return source_(object);
}
SharedTicket SharedManager::publish(Bundle b, std::optional<Uuid> expected) {
  gate(b);  // Entire request checked before storage preparation/lookup.
  ++activity_.publications;
  return SharedTicket(store_.publish(store_.prepare(std::move(b)), expected));
}
SharedTicket SharedManager::publish_selection(const SharedTicket &catalogue,
    const Uuid &catalogue_consumer, Bundle recipe, const std::vector<ToolRef> &roots) {
  gate(recipe);
  for (const auto &pin : roots) gate(pin.object);
  if (recipe.recipe_version != 2 || !recipe.tools.empty() || roots.empty() ||
      recipe.ref.selection == catalogue.ref().selection)
    throw Error("invalid public catalogue selection");

  std::set<ObjectKey> selected;
  for (const auto &row : recipe.structural_occurrences)
    if (row.id.path.empty()) selected.insert(row.definition);
  if (selected.size() != roots.size()) throw Error("selection requires exact root pins");
  for (const auto &pin : roots)
    if (pin.revision.nil() || selected.erase(pin.object) != 1)
      throw Error("selection requires exact root pins");

  const auto source = pinned(catalogue, catalogue_consumer);
  if (source.bundle.recipe_version != 2) throw Error("structural catalogue required");
  gate(source.bundle);
  std::map<ObjectKey,const ToolRef *> advertised;
  for (const auto &row : source.bundle.structural_occurrences)
    if (row.id.path.empty()) advertised.emplace(row.definition,nullptr);
  for (const auto &tool : source.bundle.tools) {
    const auto root = advertised.find(tool.ref.object);
    if (root != advertised.end()) root->second = &tool.ref;
  }
  for (const auto &pin : roots) {
    const auto root = advertised.find(pin.object);
    if (root == advertised.end() || !root->second || !(*root->second == pin))
      throw Unauthorized("definition pin outside acquired catalogue");
  }

  ++activity_.publications;
  return SharedTicket(store_.publish(store_.prepare(std::move(recipe),roots), {}));
}

Acquired SharedManager::acquire(const SharedTicket &t, const Uuid &consumer) {
  ++activity_.acquisitions;
  return store_.acquire(t.ref_.selection, consumer, t.ref_.revision);
}
Acquired SharedManager::pinned(const SharedTicket &t, const Uuid &consumer) {
  ++activity_.pinned_reads;
  return store_.read_pinned(t.ref_, consumer);
}
void SharedManager::release(const SharedTicket &t, const Uuid &consumer) {
  ++activity_.releases;
  store_.release(t.ref_, consumer);
}
}  // namespace warm
