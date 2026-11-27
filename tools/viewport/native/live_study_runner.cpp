#include "live_study.h"
#include "live_restart.h"
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
const char *status(field::TickStatus value) {
  switch (value) {
    case field::TickStatus::Ok: return "ok";
    case field::TickStatus::Unsupported: return "unsupported";
    case field::TickStatus::Stale: return "stale";
    case field::TickStatus::Unready: return "unready";
    case field::TickStatus::InvalidInput: return "invalid_input";
    case field::TickStatus::InvalidNumerics: return "invalid_numerics";
    case field::TickStatus::CapacityExhausted: return "capacity_exhausted";
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
                   std::optional<double> physical_mass, std::optional<std::uint64_t> source_mass,
                   std::optional<std::uint64_t> saved_tick) {
  auto source=request.source;
  const auto catalogue=request.bindings.bodies;
  const bool catalogue_edits=request.lod_alternative && !request.lod_alternative->initially_live.empty();
  const bool byte_focus=request.capacity.separate_membership_roots();
  const auto limits=request.capacity.limits();
  auto resources=request.resources;
  std::optional<warm::BundleRef> catalogue_ref;
  if (catalogue_edits && source_metadata.contains("catalogue_selection"))
    catalogue_ref=warm::BundleRef{warm::Uuid(source_metadata.at("catalogue_selection").get<std::string>()),
        warm::Uuid(source_metadata.at("catalogue_revision").get<std::string>())};
  assembly::ActiveSession session(input.settings);
  const auto submitted=session.submit_prepared(std::move(request),source->owner());
  if (!submitted.handle || session.on_cuda()) {
    if (saved_tick) throw UsageError(std::string("native CPU restart refused ")+status(submitted.publication.status));
    throw std::runtime_error("CPU study submission failed");
  }
  auto handle=*submitted.handle;
  field::Parameters64 parameters;
  if (input.dt) parameters.dt=*input.dt;
  std::optional<double> last_tick_dt;
  auto period=input.period;
  bool running=input.running;
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
    context.source["parameters"]["dt"]=parameters.dt;
    context.source["last_tick_dt"]=last_tick_dt ? Json(*last_tick_dt) : Json(nullptr);
    result["dt"]=parameters.dt;
    result["pace_ms"]=period.count();
    result["running"]=running;
    if(state.report.prepared_exposure)result["exposure"]=*state.report.prepared_exposure==warm::Exposure::Parts?"parts":"aggregate";
    result["completed_tick"]=state.tick;
    result["generation"]=*state.prepared_generation;
    result["owner_metadata_bytes"]=resources->result().live_bytes;
    result["owner_setup_peak_bytes"]=resources->result().peak_bytes;
    if (byte_focus) {
      result["focus"]={{"kind","parent"},{"object",warm::key(source->shared_recipe().focal_fields.front().object)}};
      result["live_fields"]=Json::array();
      for (const auto &comparison : state.comparisons) {
        const auto &group=state.report.prepared.groups.at(comparison.field.record);
        result["live_fields"].push_back({{"comparison",std::to_string(comparison.comparison)},
          {"kind",int(group.kind)},{"object",group.value},{"record",comparison.field.record}});
      }
    }
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
  if (saved_tick) created["saved_tick"]=*saved_tick;
  created["mass_policy"] = physical_mass
      ? (source_metadata.at("kind")=="fixture_only" ? "baseline_source_mass" : "retained_profile_mass")
      : "definition_source_mass";
  emit(std::move(created));
  bool rejected=false;
  std::string line;
  auto due=LiveClock::now()+period;
  bool handled_command=false;
  while (!input.commands.stopped()) {
    // Service an overdue tick even if a driver continuously queries the study.
    const bool tick_due=running && handled_command && LiveClock::now()>=due;
    const auto read=tick_due ? LiveRead::Deadline :
        input.commands.read_until(line,running ? std::optional{due} : std::nullopt);
    if (read==LiveRead::Closed) break;
    const bool automatic=read==LiveRead::Deadline;
    handled_command=!automatic;
    if (automatic) {
      if (input.commands.stopped()) break;
      due=LiveClock::now()+period;
      line="tick";
    }
    std::istringstream words(line);
    std::string command, argument, extra;
    if (!(words>>command) || command[0] == '#') continue;
    words>>argument; words>>extra;
    try {
      if (command == "quit" && argument.empty()) break;
      if (!extra.empty() && command != "add" && command != "tick") throw UsageError("too many command arguments");
      std::string revision, trailing;
      if (command == "add") words>>revision;
      if (words>>trailing) throw UsageError("too many command arguments");
      if (command=="dt") {
        if (!argument.empty()) parameters.dt=parse_live_dt(argument);
        emit({{"command",command},{"status","ok"}});
      } else if (command=="run") {
        if (!argument.empty())
          period=std::chrono::milliseconds(parse_count("run period",argument,1,86400000));
        running=true;
        emit({{"command",command},{"status","ok"}});
        due=LiveClock::now()+period;
      } else if (command=="pause" && argument.empty()) {
        running=false;
        emit({{"command",command},{"status","ok"}});
      } else if ((command == "observe" || command == "list") && argument.empty()) {
        emit({{"command",command},{"status","ok"},{"catalogue",available}});
      } else if ((command == "save" || command == "load") && !argument.empty()) {
        if (!catalogue_ref) throw UsageError("position restart requires a retained letter catalogue");
        if (command=="save") {
          const auto state=observe();
          save_retained_restart(argument,state,source->shared_recipe(),*catalogue_ref,parameters.dt);
          emit({{"command",command},{"status","ok"},{"saved_tick",state.tick}});
        } else {
          const auto saved=read_retained_restart(argument);
          if (saved.catalogue.selection!=catalogue_ref->selection || saved.catalogue.revision!=catalogue_ref->revision)
            throw UsageError("saved catalogue pin differs from the open catalogue");
          auto candidate=retained_restart_submission(std::make_shared<const assembly::ActiveAssembly>(source->revise({})),saved,input.path);
          auto candidate_source=candidate.source;
          auto candidate_resources=candidate.resources;
          const auto result=session.restore_prepared(handle,std::move(candidate),source->owner());
          if (!result.handle) {
            rejected=true;
            emit({{"command",command},{"status",status(result.publication.status)},{"saved_tick",saved.tick}});
            continue;
          }
          parameters.dt=saved.dt;
          last_tick_dt.reset();
          handle=*result.handle;
          source=std::move(candidate_source);
          resources=std::move(candidate_resources);
          emit({{"command",command},{"status","ok"},{"saved_tick",saved.tick}});
        }
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
        const auto geometry_work=extra.empty() ? limits[field::Budget::GeometryWork]
            : viewport::parse_count("geometry work",extra,0,INT64_MAX);
        std::uint64_t completed=0;
        std::optional<field::TickResult> refusal;
        for (; completed<ticks && !input.commands.stopped(); ++completed) {
          input.commands.check_consumer();
          const auto result=session.tick_prepared(handle,source->owner(),parameters,
              limits[field::Budget::TickWork],geometry_work);
          if (result.status != field::TickStatus::Ok) {
            refusal=result;
            break;
          }
        }
        Json result{{"command",command},{"status",refusal ? status(refusal->status) : completed==ticks ? "ok" : "stopped"},
                    {"requested_ticks",ticks},{"ticks",completed},{"automatic",automatic}};
        if (completed) last_tick_dt=parameters.dt;
        if (refusal && automatic) running=false;
        if (refusal) {
          rejected=true;
          const auto &attempt=*refusal;
          if (attempt.failure.exhausted())
            result["failure"]={{"budget",field::budget_name(attempt.failure.budget)},
                {"required",attempt.failure.required},{"capacity",attempt.failure.capacity}};
          result["attempt"]={{"required_work",attempt.required_work},{"geometry_work",attempt.geometry_work},
              {"current_contacts",attempt.current.contacts},{"candidate_contacts",attempt.candidate.contacts}};
          if (attempt.status==field::TickStatus::Unready) {
            write_live_event(input,result);
            return 1;
          }
        }
        emit(std::move(result));
      } else if ((command == "add" || command == "remove") && !argument.empty()) {
        const auto entry=viewport::parse_count("entry",argument,0,UINT64_MAX);
        assembly::PreparedOccurrenceEdit edit{{source->study(),{entry,{}}},command == "add"};
        if (!revision.empty()) {
          edit.definition_pin=catalogue_pin(catalogue,extra,revision);
        } else if (command == "add" && (!extra.empty() || !catalogue_edits)) {
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
      } else throw UsageError("expected dt [X], run [MS], pause, observe, list, add ENTRY [TEMPLATE_ENTRY], add ENTRY DEFINITION REVISION, remove ENTRY, lod aggregate|parts, tick [N [GEOMETRY_WORK]], save FILE, load FILE or quit");
    } catch (const UsageError &error) {
      rejected=true; emit({{"command",command},{"status","invalid_input"},{"message",error.what()}});
    }
  }
  return rejected ? 1 : 0;
}
}  // namespace viewport
