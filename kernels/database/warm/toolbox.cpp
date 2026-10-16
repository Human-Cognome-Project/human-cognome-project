#include "toolbox.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>

#include "pg.h"

namespace warm {
using namespace detail;
namespace {
void need(bool ok, const char *message) {
  if (!ok) throw Error(message);
}
void nonnil(const Uuid &id) { need(!id.nil(), "nil identity"); }
void insert(Pg &pg, const std::string &table, const std::string &columns,
            const Params &p, const std::string &suffix = "") {
  std::string sql =
      "INSERT INTO warm_toolbox." + table + " (" + columns + ") VALUES (";
  for (std::size_t i = 0; i < p.size(); ++i) {
    if (i) sql += ',';
    sql += '$' + std::to_string(i + 1);
  }
  pg.command(sql + ')' + suffix, p);
}
std::string uuid_array(std::vector<Uuid> values) {
  std::sort(values.begin(), values.end());
  std::string out = "{";
  for (const auto &v : values) {
    if (out.size() > 1) out += ',';
    out += v.str();
  }
  return out + "}";
}
Binding binding(const Result &r, int i) {
  Binding b;
  b.prefix = object(r, i, 0);
  b.kind = BindingKind(std::stoi(cell(r, i, 2)));
  b.epoch = Uuid(cell(r, i, 3));
  b.policy = Uuid(cell(r, i, 4));
  b.source_handle = cell(r, i, 5);
  b.snapshot_hash = nullable(r, i, 6);
  std::istringstream writers(cell(r, i, 7));
  std::string actor;
  while (std::getline(writers, actor, ','))
    if (!actor.empty()) b.writers.emplace_back(actor);
  return b;
}
const std::string binding_columns =
    "scope,array_to_string(prefix,'.'),kind,epoch,policy,source_handle,"
    "snapshot_hash,array_to_string(writers,',')";
bool same_binding(const Binding &a, const Binding &b) {
  return a.prefix == b.prefix && a.kind == b.kind && a.epoch == b.epoch &&
         a.policy == b.policy && a.source_handle == b.source_handle &&
         a.snapshot_hash == b.snapshot_hash &&
         uuid_array(a.writers) == uuid_array(b.writers);
}
void write_authority(const Context &c, const Binding &b) {
  if (std::find(b.writers.begin(), b.writers.end(), c.actor) ==
          b.writers.end() ||
      c.role == WriterRole::Reader ||
      (b.kind != BindingKind::Temporary && c.role != WriterRole::Manager) ||
      (!b.prefix.scope.nil() && b.prefix.scope != c.instance))
    throw Unauthorized("writer binding/role");
}
std::vector<Binding> ordered(std::vector<Binding> bindings) {
  std::sort(bindings.begin(), bindings.end(),
            [](const auto &a, const auto &b) { return a.prefix < b.prefix; });
  return bindings;
}
}  // namespace

Administrator::Administrator(const std::string &s)
    : pg_(std::make_unique<Pg>(s)) {}
Administrator::~Administrator() = default;
void Administrator::configure(std::vector<Binding> directory) {
  validate_directory(directory);
  directory = ordered(std::move(directory));
  Transaction tx(*pg_);
  // Directory administration is rare, trusted configuration work. Ordinary
  // prepare/publish operations lock only the addressed binding/object rows.
  pg_->command(
      "LOCK TABLE warm_toolbox.root_binding IN SHARE ROW EXCLUSIVE MODE");
  auto old = pg_->query(
      "SELECT " + binding_columns +
      " FROM warm_toolbox.root_binding ORDER BY scope,prefix FOR UPDATE");
  for (int i = 0; i < PQntuples(old.get()); ++i) {
    const auto previous = binding(old, i);
    auto next = std::find_if(
        directory.begin(), directory.end(),
        [&](const auto &b) { return b.prefix == previous.prefix; });
    need(next != directory.end(), "removing a retained binding is unsupported");
    need(next->kind == previous.kind, "binding ownership kind is immutable");
    if (next->epoch == previous.epoch)
      need(next->source_handle == previous.source_handle &&
               next->snapshot_hash == previous.snapshot_hash,
           "source changes require a new validated epoch");
    if (!same_binding(*next, previous))
      need(next->policy != previous.policy,
           "binding changes require a new policy version");
  }
  for (const auto &b : directory) {
    auto p = params(b.prefix);
    p.push_back(std::to_string(int(b.kind)));
    p.push_back(b.epoch.str());
    p.push_back(b.policy.str());
    p.push_back(b.source_handle);
    p.push_back(b.snapshot_hash);
    p.push_back(uuid_array(b.writers));
    insert(*pg_, "root_binding",
           "scope,prefix,kind,epoch,policy,source_handle,snapshot_hash,writers",
           p,
           " ON CONFLICT(scope,prefix) DO UPDATE SET "
           "epoch=EXCLUDED.epoch,policy=EXCLUDED.policy,"
           "source_handle=EXCLUDED.source_handle,snapshot_hash=EXCLUDED."
           "snapshot_hash,writers=EXCLUDED.writers");
  }
  tx.commit();
}
std::vector<BundleRef> Administrator::affected(const ObjectKey &prefix,
                                               const Uuid &epoch) {
  auto p = params(prefix);
  p.push_back(epoch.str());
  auto r = pg_->query(
      "SELECT selection,bundle_revision FROM warm_toolbox.epoch_use "
      "WHERE scope=$1 AND prefix=$2 AND epoch=$3 ORDER BY "
      "selection,bundle_revision",
      p);
  std::vector<BundleRef> out;
  for (int i = 0; i < PQntuples(r.get()); ++i)
    out.push_back({Uuid(cell(r, i, 0)), Uuid(cell(r, i, 1))});
  return out;
}
Toolbox::Toolbox(const std::string &s, Context c)
    : pg_(std::make_unique<Pg>(s)), context_(std::move(c)) {
  nonnil(context_.actor);
  nonnil(context_.instance);
  need(context_.role == WriterRole::Reader ||
           context_.role == WriterRole::Manager ||
           context_.role == WriterRole::Temporary,
       "invalid trusted role");
}
Toolbox::~Toolbox() = default;
void Toolbox::read_authority(const ObjectKey &o) const {
  key(o);
  if (!o.scope.nil() && o.scope != context_.instance)
    throw Unauthorized("cross-instance object");
}
Binding Toolbox::resolve_impl(const ObjectKey &o) {
  read_authority(o);
  ObjectKey prefix = o;
  while (!prefix.address.empty()) {
    auto r = pg_->query(
        "SELECT " + binding_columns +
            " FROM warm_toolbox.root_binding WHERE scope=$1 AND prefix=$2",
        params(prefix));
    if (PQntuples(r.get())) return binding(r, 0);
    prefix.address.pop_back();
  }
  throw UnboundRoot("object has no concrete authorized binding");
}
Binding Toolbox::resolve(const ObjectKey &o) { return resolve_impl(o); }
std::vector<Binding> Toolbox::used_bindings(const Bundle &b) {
  std::map<ObjectKey, Binding> unique;
  for (const auto &t : b.tools) {
    auto root = resolve_impl(t.ref.object);
    unique.emplace(root.prefix, std::move(root));
  }
  std::vector<Binding> out;
  for (const auto &[unused, root] : unique) out.push_back(root);
  return out;
}
Prepared Toolbox::prepare(Bundle request,
                          const std::vector<ToolRef> &existing_roots) {
  if (context_.role == WriterRole::Reader)
    throw Unauthorized("read-only client cannot prepare");
  if (!request.owner.nil() && request.owner != context_.instance)
    throw Unauthorized("bundle owner");
  request.owner = context_.instance;
  request.creator = context_.actor;
  Prepared candidate;
  candidate.preparer_ = context_;
  Transaction tx(*pg_, true);
  std::map<ObjectKey, std::size_t> supplied;
  for (std::size_t i = 0; i < request.tools.size(); ++i) {
    const auto &t = request.tools[i];
    read_authority(t.ref.object);
    need(supplied.emplace(t.ref.object, i).second, "duplicate supplied object");
  }
  std::set<ObjectKey> roots;
  for (const auto &ref : existing_roots) {
    read_authority(ref.object);
    nonnil(ref.revision);
    need(!supplied.count(ref.object) && roots.insert(ref.object).second,
         "existing roots must be distinct from supplied bodies and each other");
  }
  std::function<void(const ToolRef &)> follow = [&](const ToolRef &ref) {
    read_authority(ref.object);
    nonnil(ref.revision);
    const auto pinned = candidate.references_.find(ref.object);
    if (pinned != candidate.references_.end()) {
      if (!(pinned->second == ref))
        throw Conflict("existing references pin inconsistent revisions");
      return;
    }
    auto stored = read_tool(ref);
    const auto input = supplied.find(ref.object);
    if (input != supplied.end()) {
      auto &body = request.tools[input->second];
      if (!(body.ref == ref) || tool_body(body) != tool_body(stored))
        throw Conflict("supplied dependency differs from existing reference");
      body = stored;
    } else {
      supplied.emplace(ref.object, request.tools.size());
      request.tools.push_back(stored);
    }
    candidate.references_.emplace(ref.object, ref);
    for (const auto &parent : stored.parents) follow(parent.target);
    for (const auto &group : stored.member_of) follow(group);
  };
  for (const auto &ref : existing_roots) follow(ref);
  for (auto &t : request.tools)
    if (!candidate.references_.count(t.ref.object)) t.author = context_.actor;
  validate(request);
  candidate.bundle_ = std::move(request);
  candidate.bindings_ = used_bindings(candidate.bundle_);
  bool can_prepare = false;
  for (const auto &root : candidate.bindings_) {
    try {
      write_authority(context_, root);
      can_prepare = true;
    } catch (const Unauthorized &) {
    }
  }
  if (!can_prepare) throw Unauthorized("no authorized preparation binding");
  for (const auto &t : candidate.bundle_.tools) {
    const auto &root = *std::find_if(
        candidate.bindings_.begin(), candidate.bindings_.end(),
        [&](const auto &b) { return contains(b.prefix, t.ref.object); });
    if (t.epoch != root.epoch) throw Stale("preparation input epoch");
  }
  tx.commit();
  return candidate;
}
Tool Toolbox::read_tool(const ToolRef &ref) {
  auto p = params(ref);
  auto r = pg_->query(
      "SELECT epoch,recipe,notation,mass,author FROM "
      "warm_toolbox.tool_revision "
      "WHERE scope=$1 AND address=$2 AND revision=$3",
      p);
  need(PQntuples(r.get()) == 1, "missing tool revision");
  Tool tool{ref,
            Uuid(cell(r, 0, 0)),
            cell(r, 0, 1),
            nullable(r, 0, 2),
            mass(r, 0, 3),
            {},
            {},
            Uuid(cell(r, 0, 4))};
  r = pg_->query(
      "SELECT "
      "parent_scope,array_to_string(parent_address,'.'),parent_"
      "revision,ordinal,stored_mass,contribution "
      "FROM warm_toolbox.tool_parent WHERE scope=$1 AND address=$2 "
      "AND revision=$3 ORDER BY ordinal",
      p);
  for (int i = 0; i < PQntuples(r.get()); ++i)
    tool.parents.push_back({reference(r, i, 0),
                            std::uint32_t(std::stoull(cell(r, i, 3))),
                            mass(r, i, 4), *mass(r, i, 5)});
  r = pg_->query(
      "SELECT group_scope,array_to_string(group_address,'.'),group_revision "
      "FROM warm_toolbox.tool_member_of "
      "WHERE scope=$1 AND address=$2 AND revision=$3 ORDER BY "
      "group_scope,group_address,group_revision",
      p);
  for (int i = 0; i < PQntuples(r.get()); ++i)
    tool.member_of.push_back(reference(r, i, 0));
  return tool;
}
BundleRef Toolbox::publish(Prepared candidate,
                           std::optional<Uuid> expected_head) {
  if (expected_head) nonnil(*expected_head);
  if (context_.actor != candidate.preparer_.actor ||
      context_.instance != candidate.preparer_.instance ||
      context_.role != candidate.preparer_.role)
    throw Unauthorized("candidate belongs to another trusted context");
  auto &b = candidate.bundle_;
  validate(b);
  Transaction tx(*pg_);
  for (const auto &snapshot : candidate.bindings_) {
    auto r = pg_->query("SELECT " + binding_columns +
                            " FROM warm_toolbox.root_binding WHERE scope=$1 "
                            "AND prefix=$2 FOR SHARE",
                        params(snapshot.prefix));
    if (PQntuples(r.get()) != 1 || !same_binding(snapshot, binding(r, 0)))
      throw Conflict("binding/policy/epoch changed during preparation");
  }
  std::sort(b.tools.begin(), b.tools.end(),
            [](const auto &a, const auto &other) {
              return a.ref.object < other.ref.object;
            });
  std::map<std::string, ToolRef> resolved;
  std::map<std::string, bool> exists;
  std::map<std::string, Binding> roots;
  for (const auto &t : b.tools) {
    const auto &root = *std::find_if(
        candidate.bindings_.begin(), candidate.bindings_.end(),
        [&](const auto &r) { return contains(r.prefix, t.ref.object); });
    roots.emplace(key(t.ref.object), root);
    const auto reference = candidate.references_.find(t.ref.object);
    if (reference != candidate.references_.end()) {
      // Exact immutable reuse: no object-slot claim, creator check, head lookup
      // or retargeting. The complete stored dependency closure is pinned too.
      read_authority(t.ref.object);
      need(t.ref == reference->second, "existing reference changed");
      resolved.emplace(key(t.ref.object), reference->second);
      exists.emplace(key(t.ref.object), true);
      continue;
    }
    auto p = params(t.ref.object);
    p.push_back(context_.actor.str());
    insert(*pg_, "object_slot", "scope,address,creator", p,
           " ON CONFLICT DO NOTHING");
    p = params(t.ref.object);
    auto slot = pg_->query(
        "SELECT creator,temporary_revision FROM warm_toolbox.object_slot WHERE "
        "scope=$1 AND address=$2 FOR UPDATE",
        p);
    auto head_p = p;
    head_p.push_back(t.epoch.str());
    head_p.push_back(t.recipe);
    auto head = pg_->query(
        "SELECT revision FROM warm_toolbox.tool_head WHERE scope=$1 "
        "AND address=$2 AND epoch=$3 AND recipe=$4 FOR UPDATE",
        head_p);
    ToolRef chosen = t.ref;
    bool found = PQntuples(head.get()) && !PQgetisnull(head.get(), 0, 0);
    if (found) chosen.revision = Uuid(cell(head, 0, 0));
    if (root.kind == BindingKind::Temporary && !PQgetisnull(slot.get(), 0, 1)) {
      if (Uuid(cell(slot, 0, 0)) != context_.actor)
        throw Unauthorized("temporary creator owns address");
      chosen.revision = Uuid(cell(slot, 0, 1));
      found = true;
    }
    if (!found) write_authority(context_, root);
    resolved.emplace(key(t.ref.object), chosen);
    exists.emplace(key(t.ref.object), found);
  }
  for (auto &t : b.tools) {
    t.ref = resolved.at(key(t.ref.object));
    for (auto &p : t.parents) p.target = resolved.at(key(p.target.object));
    for (auto &g : t.member_of) g = resolved.at(key(g.object));
  }
  // Compare only after every dependency revision has been pinned/remapped.
  for (auto &t : b.tools)
    if (exists.at(key(t.ref.object))) {
      auto old = read_tool(t.ref);
      if (tool_body(old) != tool_body(t))
        throw Conflict(
            "immutable tool body differs for epoch/recipe or "
            "temporary address");
      t.author = old.author;
    }
  for (const auto &t : b.tools)
    if (!exists.at(key(t.ref.object))) {
      const auto &root = roots.at(key(t.ref.object));
      auto p = params(t.ref);
      p.push_back(address_array(root.prefix.address));
      p.push_back(t.epoch.str());
      p.push_back(t.recipe);
      p.push_back(t.notation);
      p.push_back(number(t.mass));
      p.push_back(context_.actor.str());
      p.push_back(root.snapshot_hash);
      insert(*pg_, "tool_revision",
             "scope,address,revision,binding_prefix,epoch,recipe,notation,mass,"
             "author,snapshot_hash",
             p);
    }
  for (const auto &t : b.tools)
    if (!exists.at(key(t.ref.object))) {
      for (const auto &parent : t.parents) {
        auto p = params(t.ref);
        p.push_back(std::to_string(parent.ordinal));
        append(p, parent.target);
        p.push_back(number(parent.stored_mass));
        p.push_back(std::to_string(parent.contribution));
        insert(*pg_, "tool_parent",
               "scope,address,revision,ordinal,parent_scope,parent_address,"
               "parent_revision,stored_mass,contribution",
               p);
      }
      for (const auto &group : t.member_of) {
        auto p = params(t.ref);
        append(p, group);
        insert(
            *pg_, "tool_member_of",
            "scope,address,revision,group_scope,group_address,group_revision",
            p);
      }
      auto p = params(t.ref.object);
      p.push_back(t.epoch.str());
      p.push_back(t.recipe);
      p.push_back(t.ref.revision.str());
      insert(*pg_, "tool_head", "scope,address,epoch,recipe,revision", p);
      if (roots.at(key(t.ref.object)).kind == BindingKind::Temporary) {
        p = params(t.ref);
        pg_->command(
            "UPDATE warm_toolbox.object_slot SET "
            "temporary_revision=$3 WHERE scope=$1 AND address=$2",
            p);
      }
    }
  insert(*pg_, "bundle_head", "selection", {b.ref.selection.str()},
         " ON CONFLICT DO NOTHING");
  auto head = pg_->query(
      "SELECT revision FROM warm_toolbox.bundle_head WHERE "
      "selection=$1 FOR UPDATE",
      {b.ref.selection.str()});
  const auto previous = nullable(head, 0, 0);
  if (previous) {
    auto owner = pg_->query(
        "SELECT owner FROM warm_toolbox.bundle_revision "
        "WHERE selection=$1 AND revision=$2",
        {b.ref.selection.str(), *previous});
    if (Uuid(cell(owner, 0, 0)) != context_.instance)
      throw Unauthorized("selection belongs to another instance");
  }
  if (previous != (expected_head
                       ? std::optional<std::string>(expected_head->str())
                       : std::nullopt))
    throw Conflict("selection head changed");
  auto p = params(b.ref);
  p.push_back(b.owner.str());
  p.push_back(b.creator.str());
  p.push_back(std::to_string(b.recipe_version));
  insert(*pg_, "bundle_revision",
         "selection,revision,owner,creator,recipe_version", p);
  for (const auto &t : b.tools) {
    auto g =
        std::find_if(b.geometry.begin(), b.geometry.end(),
                     [&](const auto &v) { return v.object == t.ref.object; });
    p = params(b.ref);
    append(p, t.ref);
    if (g != b.geometry.end())
      append(p, g->centroid);
    else
      p.insert(p.end(), 3, std::nullopt);
    insert(*pg_, "bundle_definition",
           "selection,bundle_revision,scope,address,revision,x,y,z", p);
    if (g != b.geometry.end())
      for (std::size_t i = 0; i < g->spheres.size(); ++i) {
        p = params(b.ref);
        append(p, g->object);
        p.push_back(std::to_string(i));
        append(p, g->spheres[i].centre);
        p.push_back(real(g->spheres[i].radius));
        insert(*pg_, "bundle_sphere",
               "selection,bundle_revision,scope,address,ordinal,x,y,z,radius",
               p);
      }
  }
  for (std::size_t i = 0; i < b.nodes.size(); ++i) {
    p = params(b.ref);
    p.push_back(std::to_string(i));
    p.push_back(b.nodes[i].parent < 0 ? std::nullopt
                                      : std::optional<std::string>(
                                            std::to_string(b.nodes[i].parent)));
    p.push_back(std::to_string(int(b.nodes[i].role)));
    insert(*pg_, "bundle_node", "selection,bundle_revision,node,parent,role",
           p);
  }
  for (const auto &o : b.occurrences) {
    p = params(b.ref);
    append(p, o.id);
    append(p, o.definition);
    p.push_back(std::to_string(o.node));
    append(p, o.position);
    append(p, o.velocity);
    insert(*pg_, "bundle_occurrence",
           "selection,bundle_revision,entry,path,scope,address,node,px,py,pz,"
           "vx,vy,vz",
           p);
  }
  for (const auto &s : b.selections) {
    p = params(b.ref);
    p.push_back(std::to_string(s.entry));
    p.push_back(std::to_string(int(s.exposure)));
    insert(*pg_, "bundle_selection", "selection,bundle_revision,entry,exposure",
           p);
    for (std::size_t i = 0; i < s.parts.size(); ++i) {
      p = params(b.ref);
      p.push_back(std::to_string(s.entry));
      p.push_back(std::to_string(i));
      p.push_back(path_array(s.parts[i].path));
      insert(*pg_, "bundle_part",
             "selection,bundle_revision,entry,ordinal,path", p);
    }
  }
  for (std::size_t i = 0; i < b.contacts.size(); ++i) {
    p = params(b.ref);
    p.push_back(std::to_string(i));
    append(p, b.contacts[i].a);
    append(p, b.contacts[i].b);
    insert(*pg_, "bundle_contact",
           "selection,bundle_revision,ordinal,a_entry,a_path,b_entry,b_path",
           p);
  }
  for (const auto &t : b.tools) {
    for (const auto &parent : t.parents) {
      p = params(b.ref);
      append(p, parent.target.object);
      append(p, t.ref.object);
      insert(*pg_, "bundle_child",
             "selection,bundle_revision,parent_scope,parent_address,child_"
             "scope,child_address",
             p, " ON CONFLICT DO NOTHING");
    }
    for (const auto &g : t.member_of) {
      p = params(b.ref);
      append(p, t.ref.object);
      append(p, g.object);
      insert(*pg_, "bundle_member_of",
             "selection,bundle_revision,member_scope,member_address,group_"
             "scope,group_address",
             p);
      p = params(b.ref);
      append(p, g.object);
      append(p, t.ref.object);
      insert(*pg_, "bundle_members",
             "selection,bundle_revision,group_scope,group_address,member_scope,"
             "member_address",
             p);
    }
  }
  for (const auto &root : candidate.bindings_) {
    p = params(root.prefix);
    p.push_back(root.epoch.str());
    p.push_back(b.ref.selection.str());
    p.push_back(b.ref.revision.str());
    insert(*pg_, "epoch_use", "scope,prefix,epoch,selection,bundle_revision",
           p);
  }
  pg_->command(
      "UPDATE warm_toolbox.bundle_head SET revision=$2 WHERE selection=$1",
      params(b.ref));
  tx.commit();
  return b.ref;
}
Acquired Toolbox::read_bundle(const BundleRef &ref) {
  auto p = params(ref);
  auto r = pg_->query(
      "SELECT owner,creator,recipe_version FROM warm_toolbox.bundle_revision "
      "WHERE selection=$1 AND revision=$2",
      p);
  need(PQntuples(r.get()) == 1, "missing bundle revision");
  Acquired out;
  auto &b = out.bundle;
  b.ref = ref;
  b.owner = Uuid(cell(r, 0, 0));
  b.creator = Uuid(cell(r, 0, 1));
  b.recipe_version = std::stoi(cell(r, 0, 2));
  if (b.owner != context_.instance) throw Unauthorized("bundle owner");
  r = pg_->query(
      "SELECT scope,array_to_string(address,'.'),revision,x,y,z FROM "
      "warm_toolbox.bundle_definition "
      "WHERE selection=$1 AND bundle_revision=$2 ORDER BY scope,address",
      p);
  for (int i = 0; i < PQntuples(r.get()); ++i) {
    auto tool = read_tool(reference(r, i, 0));
    read_authority(tool.ref.object);
    b.tools.push_back(tool);
    if (!PQgetisnull(r.get(), i, 3)) {
      Geometry g{tool.ref.object, point(r, i, 3), {}};
      auto gp = p;
      append(gp, g.object);
      auto spheres = pg_->query(
          "SELECT x,y,z,radius FROM warm_toolbox.bundle_sphere "
          "WHERE selection=$1 AND bundle_revision=$2 "
          "AND scope=$3 AND address=$4 ORDER BY ordinal",
          gp);
      for (int j = 0; j < PQntuples(spheres.get()); ++j)
        g.spheres.push_back(
            {point(spheres, j, 0), real_value(cell(spheres, j, 3))});
      b.geometry.push_back(std::move(g));
    }
  }
  r = pg_->query(
      "SELECT parent,role FROM warm_toolbox.bundle_node WHERE "
      "selection=$1 AND bundle_revision=$2 ORDER BY node",
      p);
  for (int i = 0; i < PQntuples(r.get()); ++i)
    b.nodes.push_back(
        {PQgetisnull(r.get(), i, 0) ? -1 : std::stoi(cell(r, i, 0)),
         ScaleRole(std::stoi(cell(r, i, 1)))});
  r = pg_->query(
      "SELECT "
      "entry,array_to_string(path,','),scope,array_to_string("
      "address,'.'),node,px,py,pz,vx,vy,vz "
      "FROM warm_toolbox.bundle_occurrence WHERE selection=$1 AND "
      "bundle_revision=$2 ORDER BY entry,path",
      p);
  for (int i = 0; i < PQntuples(r.get()); ++i)
    b.occurrences.push_back({occurrence_id(r, i, 0), object(r, i, 2),
                             std::stoi(cell(r, i, 4)), point(r, i, 5),
                             point(r, i, 8)});
  r = pg_->query(
      "SELECT entry,exposure FROM warm_toolbox.bundle_selection "
      "WHERE selection=$1 AND bundle_revision=$2 ORDER BY entry",
      p);
  for (int i = 0; i < PQntuples(r.get()); ++i) {
    Selection s{
        std::stoull(cell(r, i, 0)), Exposure(std::stoi(cell(r, i, 1))), {}};
    auto sp = p;
    sp.push_back(std::to_string(s.entry));
    auto parts = pg_->query(
        "SELECT entry,array_to_string(path,',') FROM warm_toolbox.bundle_part "
        "WHERE selection=$1 AND bundle_revision=$2 AND entry=$3 ORDER BY "
        "ordinal",
        sp);
    for (int j = 0; j < PQntuples(parts.get()); ++j)
      s.parts.push_back(occurrence_id(parts, j, 0));
    b.selections.push_back(std::move(s));
  }
  r = pg_->query(
      "SELECT "
      "a_entry,array_to_string(a_path,','),b_entry,array_to_string("
      "b_path,',') FROM warm_toolbox.bundle_contact "
      "WHERE selection=$1 AND bundle_revision=$2 ORDER BY ordinal",
      p);
  for (int i = 0; i < PQntuples(r.get()); ++i)
    b.contacts.push_back({occurrence_id(r, i, 0), occurrence_id(r, i, 2)});
  const auto links = [&](const std::string &table, const std::string &a,
                         const std::string &other) {
    auto rows = pg_->query(
        "SELECT " + a + "_scope,array_to_string(" + a + "_address,'.')," +
            other + "_scope,array_to_string(" + other +
            "_address,'.') "
            "FROM warm_toolbox." +
            table +
            " WHERE selection=$1 AND bundle_revision=$2 ORDER BY 1,2,3,4",
        p);
    Links list;
    for (int i = 0; i < PQntuples(rows.get()); ++i)
      list.emplace_back(object(rows, i, 0), object(rows, i, 2));
    std::sort(list.begin(), list.end());
    return list;
  };
  out.lists.children = links("bundle_child", "parent", "child");
  out.lists.member_of = links("bundle_member_of", "member", "group");
  out.lists.members = links("bundle_members", "group", "member");
  Projection expected;
  for (const auto &t : b.tools) {
    for (const auto &parent : t.parents)
      expected.children.emplace_back(parent.target.object, t.ref.object);
    for (const auto &g : t.member_of) {
      expected.member_of.emplace_back(t.ref.object, g.object);
      expected.members.emplace_back(g.object, t.ref.object);
    }
  }
  const auto same = [](Links expected, const Links &actual) {
    std::sort(expected.begin(), expected.end());
    expected.erase(std::unique(expected.begin(), expected.end()),
                   expected.end());
    return expected == actual;
  };
  need(same(expected.children, out.lists.children) &&
           same(expected.member_of, out.lists.member_of) &&
           same(expected.members, out.lists.members),
       "incomplete or conflicting projected reciprocals");
  validate(b);
  return out;
}
Acquired Toolbox::acquire(const Uuid &selection, const Uuid &consumer) {
  nonnil(selection);
  nonnil(consumer);
  auto head = pg_->query(
      "SELECT revision FROM warm_toolbox.bundle_head WHERE selection=$1",
      {selection.str()});
  need(PQntuples(head.get()) == 1 && !PQgetisnull(head.get(), 0, 0),
       "selection not published");
  const BundleRef ref{selection, Uuid(cell(head, 0, 0))};
  // Revisions are immutable and retained. Discover direct binding references
  // before locking, then lock bindings before the head in publication order.
  auto out = read_bundle(ref);
  auto roots = used_bindings(out.bundle);
  Transaction tx(*pg_);
  for (auto &root : roots) {
    auto r = pg_->query("SELECT " + binding_columns +
                            " FROM warm_toolbox.root_binding WHERE scope=$1 "
                            "AND prefix=$2 FOR SHARE",
                        params(root.prefix));
    need(PQntuples(r.get()) == 1, "missing root binding");
    root = binding(r, 0);
    for (const auto &t : out.bundle.tools)
      if (contains(root.prefix, t.ref.object) && t.epoch != root.epoch)
        throw Stale("source epoch superseded");
  }
  head = pg_->query(
      "SELECT revision FROM warm_toolbox.bundle_head WHERE "
      "selection=$1 FOR SHARE",
      {selection.str()});
  if (cell(head, 0, 0) != ref.revision.str())
    throw Conflict("selection changed during acquire");
  auto p = params(ref);
  p.push_back(consumer.str());
  p.push_back(context_.actor.str());
  p.push_back(context_.instance.str());
  insert(*pg_, "bundle_lease",
         "selection,bundle_revision,consumer,actor,instance", p,
         " ON CONFLICT DO NOTHING");
  auto lease = pg_->query(
      "SELECT actor,instance FROM warm_toolbox.bundle_lease WHERE "
      "selection=$1 AND bundle_revision=$2 AND consumer=$3",
      {ref.selection.str(), ref.revision.str(), consumer.str()});
  if (Uuid(cell(lease, 0, 0)) != context_.actor ||
      Uuid(cell(lease, 0, 1)) != context_.instance)
    throw Unauthorized("consumer lease ownership");
  tx.commit();
  return out;
}
Acquired Toolbox::read_pinned(const BundleRef &ref, const Uuid &consumer) {
  nonnil(ref.selection);
  nonnil(ref.revision);
  nonnil(consumer);
  auto p = params(ref);
  p.push_back(consumer.str());
  Transaction tx(*pg_, true);
  auto r = pg_->query(
      "SELECT actor,instance FROM warm_toolbox.bundle_lease WHERE "
      "selection=$1 AND bundle_revision=$2 AND consumer=$3",
      p);
  if (PQntuples(r.get()) != 1 || Uuid(cell(r, 0, 0)) != context_.actor ||
      Uuid(cell(r, 0, 1)) != context_.instance)
    throw Unauthorized("no owned pin");
  auto out = read_bundle(ref);
  tx.commit();
  return out;
}
void Toolbox::release(const BundleRef &ref, const Uuid &consumer) {
  nonnil(ref.selection);
  nonnil(ref.revision);
  nonnil(consumer);
  auto p = params(ref);
  p.push_back(consumer.str());
  Transaction tx(*pg_);
  auto r = pg_->query(
      "SELECT actor,instance FROM warm_toolbox.bundle_lease WHERE selection=$1 "
      "AND bundle_revision=$2 AND consumer=$3 FOR UPDATE",
      p);
  if (PQntuples(r.get())) {
    if (Uuid(cell(r, 0, 0)) != context_.actor ||
        Uuid(cell(r, 0, 1)) != context_.instance)
      throw Unauthorized("cannot release another consumer's pin");
    pg_->command(
        "DELETE FROM warm_toolbox.bundle_lease WHERE selection=$1 AND "
        "bundle_revision=$2 AND consumer=$3",
        p);
  }
  tx.commit();
}
}  // namespace warm
