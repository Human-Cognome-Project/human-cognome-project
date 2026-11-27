#include "live_restart.h"
#include "capture_options.h"
#include "prepared_letter_profile.h"

#include <algorithm>
#include <fstream>

namespace viewport {
namespace {
constexpr auto schema = "hcp.retained.cpu-position-restart";

Json revisions(const warm::Bundle &recipe) {
  auto pins=Json::object();
  for (const auto &tool : recipe.tools)
    pins[warm::key(tool.ref.object)]=tool.ref.revision.str();
  return pins;
}
Json point(const warm::Point &p) { return Json::array({p.x,p.y,p.z}); }
std::vector<std::string> catalogue_letters(const warm::Bundle &recipe) {
  std::vector<std::string> letters;
  for (const auto &row : recipe.structural_occurrences)
    if (row.id.path.empty()) letters.push_back(warm::address_text(row.definition.address));
  return letters;
}
std::optional<std::string> catalogue_focus_byte(const warm::Bundle &recipe) {
  if (recipe.focal_fields.size()!=1) throw UsageError("restart requires one catalogue focus");
  const auto &focus=recipe.focal_fields.front();
  if (focus.kind==warm::FieldKind::SiblingDefinition) return warm::address_text(focus.object.address);
  return std::nullopt;
}
Json structure(const warm::Bundle &recipe) {
  auto tools=Json::object(), geometry=Json::object();
  auto occurrences=Json::array(), focus=Json::array(), nodes=Json::array();
  for (const auto &tool : recipe.tools) {
    auto parents=Json::array();
    std::vector<std::string> memberships;
    for (const auto &p : tool.parents)
      parents.push_back({{"object",warm::key(p.target.object)},{"ordinal",p.ordinal},
          {"stored_mass",p.stored_mass ? Json(*p.stored_mass) : Json(nullptr)},{"count",p.contribution}});
    for (const auto &m : tool.member_of) memberships.push_back(warm::key(m.object));
    std::sort(memberships.begin(),memberships.end());
    tools[warm::key(tool.ref.object)]={{"notation",tool.notation ? Json(std::string(*tool.notation)) : Json(nullptr)},
        {"mass",tool.mass ? Json(*tool.mass) : Json(nullptr)},{"parents",std::move(parents)},
        {"member_of",std::move(memberships)}};
  }
  for (const auto &g : recipe.geometry) {
    auto spheres=Json::array();
    for (const auto &s : g.spheres) spheres.push_back({point(s.centre),s.radius});
    geometry[warm::key(g.object)]={{"centre",point(g.centroid)},{"spheres",std::move(spheres)}};
  }
  for (const auto &row : recipe.structural_occurrences)
    occurrences.push_back({row.id.entry,row.id.path,row.node,warm::key(row.definition)});
  for (const auto &row : recipe.focal_fields) focus.push_back({int(row.kind),warm::key(row.object)});
  for (const auto &row : recipe.nodes) nodes.push_back({row.parent,int(row.role)});
  return {{"tools",std::move(tools)},{"geometry",std::move(geometry)},
          {"occurrences",std::move(occurrences)},{"focus",std::move(focus)},{"nodes",std::move(nodes)}};
}
void verify_structure(const RetainedRestart &saved,const warm::Bundle &recipe) {
  if (saved.letters!=catalogue_letters(recipe) || saved.focus_byte!=catalogue_focus_byte(recipe))
    throw UsageError("saved catalogue selectors differ from the pinned recipe");
  if (saved.structure!=structure(recipe))
    throw UsageError("saved catalogue structure differs from the selected recipe");
}
warm::Uuid uuid(const Json &value) {
  warm::Uuid id(value.get<std::string>());
  if (id.nil()) throw UsageError("restart identity is empty");
  return id;
}
warm::BundleRef pin(const Json &value) { return {uuid(value.at("selection")),uuid(value.at("revision"))}; }
Json pin(const warm::BundleRef &value) {
  return {{"selection",value.selection.str()},{"revision",value.revision.str()}};
}
warm::ObjectKey public_object(const Json &value) {
  const auto text=value.get<std::string>();
  const auto slash=text.find('/');
  if (slash==std::string::npos || !warm::Uuid(text.substr(0,slash)).nil())
    throw UsageError("restart requires public definitions");
  const auto decoded=codec::decode_token_id(text.substr(slash+1));
  if (!decoded) throw UsageError("invalid restart definition address");
  return {{},*decoded};
}
field::FrameAnchor coordinates(const Json &value) {
  if (!value.is_array() || value.size()!=3) throw UsageError("restart coordinates require three components");
  return value.get<field::FrameAnchor>();
}
}

RetainedRestart read_retained_restart(const std::string &path) {
  try {
    std::ifstream input(path,std::ios::binary | std::ios::ate);
    if (!input) throw UsageError("restart file is unavailable");
    const auto size=input.tellg();
    constexpr std::streamoff maximum_bytes=8*1024*1024;
    if (size<=0 || size>maximum_bytes) throw UsageError("restart file has an unsupported size");
    std::string data(std::size_t(size),'\0');
    input.seekg(0);
    if (!input.read(data.data(),size)) throw UsageError("restart file could not be read");
    const auto file=Json::parse(data);
    const auto &version=file.at("version");
    if (file.at("schema")!=schema || (version!=1 && version!=2))
      throw UsageError("unsupported restart format");
    RetainedRestart out;
    if (version==2) {
      const auto &value=file.at("dt");
      if (!value.is_number()) throw UsageError("invalid restart timestep");
      out.dt=value.get<double>();
      if (!(out.dt>0 && std::isnormal(out.dt))) throw UsageError("invalid restart timestep");
    }
    out.study=uuid(file.at("study"));
    out.catalogue=pin(file.at("catalogue"));
    out.recipe=pin(file.at("recipe"));
    if (!file.at("saved_tick").is_number_unsigned()) throw UsageError("invalid saved tick");
    out.tick=file.at("saved_tick").get<std::uint64_t>();
    const auto exposure=file.at("exposure");
    if (exposure!="aggregate" && exposure!="parts") throw UsageError("invalid restart exposure");
    out.exposure=exposure=="parts" ? warm::Exposure::Parts : warm::Exposure::Aggregate;
    out.letters=file.at("letters").get<std::vector<std::string>>();
    if (out.letters.empty()) throw UsageError("restart catalogue is empty");
    if (!file.at("focus_byte").is_null()) out.focus_byte=file.at("focus_byte").get<std::string>();
    out.structure=file.at("structure");
    out.revisions=file.at("revisions");
    out.state.frame=coordinates(file.at("frame"));
    const auto &positions=file.at("positions");
    if (!positions.is_array() || positions.empty()) throw UsageError("restart positions are missing");
    for (const auto &row : positions) {
      if (!row.at("entry").is_number_unsigned() || !row.at("path").is_array())
        throw UsageError("invalid restart occurrence");
      warm::OccurrenceId id{row.at("entry").get<std::uint64_t>(),{}};
      for (const auto &ordinal : row.at("path")) {
        if (!ordinal.is_number_unsigned() || ordinal.get<std::uint64_t>()>UINT32_MAX)
          throw UsageError("invalid restart path");
        id.path.push_back(ordinal.get<std::uint32_t>());
      }
      out.state.positions.push_back({std::move(id), {public_object(row.at("definition")),uuid(row.at("revision"))},
          row.at("units").get<double>(),coordinates(row.at("position"))});
    }
    return out;
  } catch (const Json::exception &) {
    throw UsageError("invalid restart file");
  } catch (const warm::Error &) {
    throw UsageError("invalid restart identity");
  }
}

void verify_retained_restart(const RetainedRestart &saved,const warm::Bundle &recipe) {
  verify_structure(saved,recipe);
  if (saved.recipe.selection!=recipe.ref.selection || saved.recipe.revision!=recipe.ref.revision ||
      saved.revisions!=revisions(recipe))
    throw UsageError("saved recipe differs from the selected catalogue revision");
}
void rebind_retained_revisions(const RetainedRestart &saved,warm::Bundle &recipe) {
  verify_structure(saved,recipe);
  if (!saved.revisions.is_object() || saved.revisions.size()!=recipe.tools.size())
    throw UsageError("saved catalogue differs from the retained sources");
  const auto rebind=[&](warm::ToolRef &ref) {
    const auto key=warm::key(ref.object);
    if (!saved.revisions.contains(key)) throw UsageError("saved catalogue pin is missing");
    ref.revision=uuid(saved.revisions.at(key));
  };
  try {
    for (auto &tool : recipe.tools) {
      rebind(tool.ref);
      for (auto &parent : tool.parents) rebind(parent.target);
      for (auto &group : tool.member_of) rebind(group);
    }
  } catch (const Json::exception &) {
    throw UsageError("invalid catalogue revision");
  } catch (const warm::Error &) {
    throw UsageError("invalid catalogue revision");
  }
  recipe.ref=saved.catalogue;
}

void save_retained_restart(const std::string &path,const CompletedState &state,
    const warm::Bundle &recipe,const warm::BundleRef &catalogue,double dt) {
  auto positions=Json::array();
  const auto &report=state.report;
  for (const auto &particle : report.prepared.particles) {
    const auto &body=particle.occurrence;
    const auto origin=std::find_if(report.origins.begin(),report.origins.end(),[&](const auto &item) {
      return assembly::occurrence_text(item.occurrence)==std::string(body.id);
    });
    if (origin==report.origins.end() || !origin->shared ||
        std::none_of(recipe.structural_occurrences.begin(),recipe.structural_occurrences.end(),[&](const auto &row) {
          return row.id==origin->occurrence.source && row.definition==origin->shared->object;
        })) throw UsageError("restart requires declared catalogue occurrences");
    field::FrameAnchor position;
    for (int axis=0; axis<3; ++axis)
      position[axis]=report.particles64.at((field::kPosX+axis)*report.prepared.capacity+body.slot);
    positions.push_back({{"entry",origin->occurrence.source.entry},{"path",origin->occurrence.source.path},
        {"definition",warm::key(origin->shared->object)},{"revision",origin->shared->revision.str()},
        {"units",report.prepared.scales.at(body.study_node).units},{"position",position}});
  }
  const auto byte_focus=catalogue_focus_byte(recipe);
  try {
    write_atomic(path,{{"schema",schema},{"version",2},{"dt",dt},{"study",report.handle.study.str()},
        {"catalogue",pin(catalogue)},{"recipe",pin(recipe.ref)},{"saved_tick",state.tick},
        {"exposure",report.prepared_exposure==warm::Exposure::Parts ? "parts" : "aggregate"},
        {"letters",catalogue_letters(recipe)},{"focus_byte",byte_focus ? Json(*byte_focus) : Json(nullptr)},{"structure",structure(recipe)},
        {"revisions",revisions(recipe)},{"frame",report.frame_anchor},{"positions",std::move(positions)}});
  } catch (const std::exception &) {
    throw UsageError("restart file could not be saved");
  }
}

assembly::PreparedStudySubmission retained_restart_submission(
    std::shared_ptr<const assembly::ActiveAssembly> source,const RetainedRestart &saved,field::HostPath path) {
  verify_retained_restart(saved,source->shared_recipe());
  if (source->study()!=saved.study) throw UsageError("restart belongs to a different study");
  std::vector<std::uint64_t> entries;
  for (const auto &row : saved.state.positions)
    if (entries.empty() || entries.back()!=row.occurrence.entry) entries.push_back(row.occurrence.entry);
  auto request=editable_retained_catalogue_submission(std::move(source),entries.front(),path);
  request.lod_alternative->initially_live=std::move(entries);
  request.lod_alternative->initial_exposure=saved.exposure;
  request.restart=saved.state;
  return request;
}
}
