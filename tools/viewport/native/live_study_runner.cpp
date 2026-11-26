#include "live_study.h"
#include "capture_options.h"
#include "assembly/prepared_occurrence_edit.h"
#include <iostream>
#include <sstream>

namespace viewport {
namespace {
const warm::ToolRef &catalogue_pin(
    const std::vector<assembly::detail::AdmissionBody> &catalogue,
    const std::string &definition, const std::string &revision_text) {
  warm::Uuid revision;
  try {
    revision = warm::Uuid(revision_text);
  } catch (const warm::Error &) {
    throw UsageError("invalid definition revision");
  }
  for (const auto &body : catalogue)
    if (body.occurrence.source.path.empty() && body.definition.revision == revision &&
        warm::key(body.definition.object) == definition)
      return body.definition;
  throw UsageError("definition pin is not in the declared source catalogue");
}
const char *status(field::UpdateStatus value) {
  switch (value) {
    case field::UpdateStatus::Ok: return "ok";
    case field::UpdateStatus::NoChange: return "no_change";
    case field::UpdateStatus::InvalidInput: return "invalid_input";
    case field::UpdateStatus::Stale: return "stale";
    case field::UpdateStatus::CapacityExhausted: return "capacity_exhausted";
    case field::UpdateStatus::Unsupported: return "unsupported";
    case field::UpdateStatus::InvalidNumerics: return "invalid_numerics";
    case field::UpdateStatus::Unready: return "unready";
  }
  return "unknown";
}
}

void write_live_event(const LiveStudyInput &input, const Json &event) {
  if (input.commands.interruptible()) input.commands.write_output(event.dump());
  else write_live_output(input.output,event.dump());
}

int run_live_study(const LiveStudyInput &input, assembly::PreparedStudySubmission request,
                   const std::string &label, const Json &source_metadata,
                   std::optional<double> physical_mass, std::optional<std::uint64_t> source_mass) {
  const auto source=request.source;
  const auto catalogue=request.bindings.bodies;
  const auto limits=request.capacity.limits();
  const auto resources=request.resources;
  assembly::ActiveSession session(input.settings);
  const auto submitted=session.submit_prepared(std::move(request),source->owner());
  if (!submitted.handle || session.on_cuda()) throw std::runtime_error("CPU study submission failed");
  const auto handle=*submitted.handle;
  field::Parameters64 parameters;
  const auto observe=[&] {
    return viewport::observe(session,source->owner(),handle,viewport::IntentRequest{parameters.gravity,1000000},input.responses);
  };
  const auto emit=[&](Json result) {
    const auto state=observe();
    auto context=viewport::context(*source,handle,state.tick,label);
    const auto declared=context.occurrences;
    context.occurrences.clear();
    for (const auto &body : state.report.prepared.particles) {
      const auto id=std::string(body.occurrence.id);
      const auto existing=declared.find(id);
      if (existing != declared.end()) {
        auto display=existing->second;
        if(state.report.prepared_exposure)display.exposure=*state.report.prepared_exposure==warm::Exposure::Parts?"parts":"aggregate";
        context.occurrences.emplace(id,std::move(display));
      }
      else {
        // The only new-entry operation copies a declared root aggregate.
        // Labels describe the validated definition; live membership comes
        // exclusively from the completed owner report above.
        const assembly::Origin *origin=nullptr;
        for (const auto &item : state.report.origins)
          if (assembly::occurrence_text(item.occurrence) == id) origin=&item;
        if (!origin || !origin->shared || (!origin->occurrence.source.path.empty() &&
            origin->occurrence.source.path.size()!=2))
          throw std::runtime_error("unsupported live display provenance");
        std::string label;
        for (const auto &tool : source->shared_recipe().tools)
          if (tool.ref == *origin->shared && tool.notation) label=std::string(*tool.notation);
        auto aggregate=origin->occurrence;aggregate.source.path.clear();
        context.occurrences.emplace(id,viewport::DisplayContext{origin->occurrence.source.path.empty()?"aggregate":"parts",
            assembly::occurrence_text(aggregate),std::move(label)});
      }
    }
    context.source=source_metadata;
    if(state.report.prepared_exposure)result["exposure"]=*state.report.prepared_exposure==warm::Exposure::Parts?"parts":"aggregate";
    result["completed_tick"]=state.tick;
    result["generation"]=*state.prepared_generation;
    result["owner_metadata_bytes"]=resources->result().live_bytes;
    result["owner_setup_peak_bytes"]=resources->result().peak_bytes;
    result["live_occurrences"]=Json::array();
    for (std::size_t i=0; i<state.body_records.size(); ++i) {
      const auto &claim=state.body_records[i];
      const auto capacity=state.report.prepared.capacity;
      const auto velocity=[&](int axis) {return state.report.particles64.at((field::kVelX+axis)*capacity+claim.record);};
      result["live_occurrences"].push_back({{"id",std::string(state.report.prepared.particles[i].occurrence.id)},
        {"context",claim.context},{"incarnation",claim.incarnation},{"address",claim.address},{"record",claim.record},
        {"velocity",{velocity(0),velocity(1),velocity(2)}}});
    }
    result["observation"]=viewport::snapshot(state,context);
    write_live_event(input,result);
  };
  Json available=Json::array();
  for (const auto &entry : catalogue)
    available.push_back({{"entry",entry.occurrence.source.entry},{"id",assembly::occurrence_text(entry.occurrence)},
        {"definition",warm::key(entry.definition.object)},{"revision",entry.definition.revision.str()}});
  Json created{{"command","create"},{"status","ok"},{"catalogue",available},
    {"actual_backend","native_cpu"},{"host_path",input.path == field::HostPath::BulkCopy ? "bulk" : "direct"},
    {"capacities",{{"particles",limits[field::Budget::Particles]},{"fields",limits[field::Budget::Fields]},
      {"participation",limits[field::Budget::Participation]},{"contacts",limits[field::Budget::Contacts]},
      {"batch",limits[field::Budget::Batch]},{"scratch_bytes",limits[field::Budget::ScratchBytes]}}}};
  if (physical_mass) created["physical_mass_per_occurrence"] = *physical_mass;
  if (source_mass) created["exact_source_mass_per_occurrence"] = *source_mass;
  created["mass_policy"] = physical_mass
      ? (source_metadata.at("kind")=="fixture_only" ? "baseline_source_mass" : "retained_profile_mass")
      : "definition_source_mass";
  emit(std::move(created));
  bool rejected=false;
  std::string line;
  while (input.commands.read_line(line)) {
    std::istringstream words(line);
    std::string command, argument, extra;
    if (!(words>>command) || command[0] == '#') continue;
    words>>argument; words>>extra;
    try {
      if (command == "quit" && argument.empty()) break;
      if (!extra.empty() && command != "add") throw UsageError("too many command arguments");
      std::string revision, trailing;
      if (command == "add") words>>revision;
      if (words>>trailing) throw UsageError("too many command arguments");
      if ((command == "observe" || command == "list") && argument.empty()) {
        emit({{"command",command},{"status","ok"},{"catalogue",available}});
      } else if(command == "lod" && (argument=="aggregate" || argument=="parts")) {
        assembly::PreparedLodEdit edit{argument=="parts"?warm::Exposure::Parts:warm::Exposure::Aggregate,*observe().prepared_generation};
        edit.preparation_work=10000000;edit.placement_work=100000000;edit.placement_cells=65536;edit.hierarchy_work=1000000;
        const auto result=session.lod_prepared(handle,source->owner(),edit);
        const auto &p=result.publication;
        if(p.status!=field::UpdateStatus::Ok && p.status!=field::UpdateStatus::NoChange)rejected=true;
        Json claims{{"sampled",p.claims.sampled}};
        if(p.claims.sampled) {
          const auto &c=p.claims;
          claims["particles"]={{"before",c.particles_before},{"peak",c.particles_peak},{"after",c.particles_after}};
          claims["fields"]={{"before",c.fields_before},{"peak",c.fields_peak},{"after",c.fields_after}};
        }
        emit({{"command",command},{"requested_exposure",argument},{"status",status(p.status)},
          {"native_claims",std::move(claims)},{"preparation_work",result.preparation.charged},
          {"failure",{{"budget",field::budget_name(p.failure.budget)},{"required",p.failure.required},{"capacity",p.failure.capacity}}}});
      } else if (command == "tick") {
        const auto ticks=argument.empty() ? 1 : viewport::parse_count("tick",argument,1,1000000);
        std::uint64_t completed=0;
        for (; completed<ticks && !input.commands.stopped(); ++completed) {
          input.commands.check_consumer();
          const auto result=session.tick_prepared(handle,source->owner(),parameters,1000000000,1000000);
          if (result.status != field::TickStatus::Ok) throw std::runtime_error("native tick refused");
        }
        emit({{"command",command},{"status",completed==ticks ? "ok" : "stopped"},
              {"ticks",completed}});
      } else if ((command == "add" || command == "remove") && !argument.empty()) {
        const auto entry=viewport::parse_count("entry",argument,0,UINT64_MAX);
        assembly::PreparedOccurrenceEdit edit{{source->study(),{entry,{}}},command == "add"};
        if (!revision.empty()) {
          edit.definition_pin=catalogue_pin(catalogue,extra,revision);
        } else if (command == "add") {
          bool declared_entry=false;
          for (const auto &item : catalogue) declared_entry |= item.occurrence.source.entry == entry;
          const auto template_entry=extra.empty() ? (declared_entry ? entry : 2)
              : viewport::parse_count("template",extra,0,UINT64_MAX);
          for (const auto &item : catalogue)
            if (item.occurrence.source.entry == template_entry && item.occurrence.source.path.empty())
              edit.clone_from=item.occurrence;
          if (!edit.clone_from) throw UsageError("template is not in the declared source catalogue");
        }
        edit.expected_generation=*observe().prepared_generation;
        edit.preparation_work=10000000; edit.placement_work=100000000;
        edit.placement_cells=65536; edit.hierarchy_work=1000000;
        const auto result=session.edit_prepared(handle,source->owner(),edit);
        const auto code=result.publication.status;
        if (code != field::UpdateStatus::Ok && code != field::UpdateStatus::NoChange) rejected=true;
        Json claims{{"sampled",result.publication.claims.sampled}};
        if (result.publication.claims.sampled) {
          const auto &c=result.publication.claims;
          claims["particles"]={{"before",c.particles_before},{"peak",c.particles_peak},{"after",c.particles_after}};
          claims["fields"]={{"before",c.fields_before},{"peak",c.fields_peak},{"after",c.fields_after}};
        }
        emit({{"command",command},{"entry",entry},{"status",status(code)},
          {"native_claims",std::move(claims)},
          {"preparation_status",int(result.preparation.status)},
          {"preparation_work",result.preparation.charged}});
      } else throw UsageError("expected observe, list, add ENTRY [TEMPLATE_ENTRY], add ENTRY DEFINITION REVISION, remove ENTRY, lod aggregate|parts, tick [N] or quit");
    } catch (const UsageError &error) {
      rejected=true; emit({{"command",command},{"status","invalid_input"},{"message",error.what()}});
    }
  }
  return rejected ? 1 : 0;
}
}  // namespace viewport
