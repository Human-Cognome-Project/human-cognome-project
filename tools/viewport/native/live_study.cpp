#include "prepared_letter_profile.h"
#include "snapshot.h"
#include "capture_options.h"
#include "assembly/prepared_occurrence_edit.h"
#include "letter_projection_fixture.h"
#include <fstream>
#include <iostream>
#include <sstream>

namespace {
using viewport::Json;
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
void help() {
  std::cout << "viewport_live_study [--threads N] [--direct] [--response-work-limit N] [--commands FILE]\n"
      "Commands: observe; list; add ENTRY [TEMPLATE_ENTRY]; remove ENTRY; tick [N]; quit\n"
      "The fixture English-pair catalogue declares entries 2 and 10. Both start live.\n"
      "A new root entry copies a declared template (default 2) within capacity.\n";
}
}
int main(int argc, char **argv) {
  try {
    engine::InstanceSettings settings; settings.read_environment=false; settings.cpu_threads=1;
    auto path=field::HostPath::BulkCopy;
    std::string command_file;
    std::optional<viewport::ResponseRequest> responses;
    for (int i=1; i<argc; ++i) {
      const std::string arg=argv[i];
      if (arg == "--help") { help(); return 0; }
      if (arg == "--direct") path=field::HostPath::CpuDirect;
      else if (arg == "--response-work-limit" && i+1<argc)
        responses=viewport::ResponseRequest{viewport::parse_count("--response-work-limit",argv[++i],0,UINT64_MAX)};
      else if (arg == "--threads" && i+1<argc)
        settings.cpu_threads=int(viewport::parse_count("--threads",argv[++i],1,1024));
      else if (arg == "--commands" && i+1<argc) command_file=argv[++i];
      else throw std::invalid_argument("unknown or incomplete option");
    }
    std::ifstream file;
    if (!command_file.empty()) {
      file.open(command_file);
      if (!file) throw std::invalid_argument("cannot open command file");
    }
    std::istream &input=command_file.empty() ? std::cin : file;
    letter_fixture::Fixture fixture;
    const auto english=fixture.ref(fixture.english);
    const auto source=std::make_shared<const assembly::ActiveAssembly>(assembly::ActiveAssembly::assemble(
        viewport::prepared_english_pair(fixture.base,english),warm::new_uuid(),false));
    auto request=viewport::editable_english_pair_submission(source,english,path);
    const auto catalogue=request.bindings.bodies;
    const auto limits=request.capacity.limits();
    const auto resources=request.resources;
    assembly::ActiveSession session(settings);
    const auto submitted=session.submit_prepared(std::move(request),source->owner());
    if (!submitted.handle || session.on_cuda()) throw std::runtime_error("CPU study submission failed");
    const auto handle=*submitted.handle;
    field::Parameters64 parameters;
    const auto observe=[&] {
      return viewport::observe(session,source->owner(),handle,viewport::IntentRequest{parameters.gravity,1000000},responses);
    };
    const auto emit=[&](Json result) {
      const auto state=observe();
      auto context=viewport::context(*source,handle,state.tick,"Editable English-pair fixture");
      const auto declared=context.occurrences;
      context.occurrences.clear();
      for (const auto &body : state.report.prepared.particles) {
        const auto id=std::string(body.occurrence.id);
        const auto existing=declared.find(id);
        if (existing != declared.end()) context.occurrences.emplace(id,existing->second);
        else {
          // The only new-entry operation copies a declared root aggregate.
          // Labels describe the validated definition; live membership comes
          // exclusively from the completed owner report above.
          const assembly::Origin *origin=nullptr;
          for (const auto &item : state.report.origins)
            if (assembly::occurrence_text(item.occurrence) == id) origin=&item;
          if (!origin || !origin->occurrence.source.path.empty() || !origin->shared)
            throw std::runtime_error("unsupported live display provenance");
          std::string label;
          for (const auto &tool : source->shared_recipe().tools)
            if (tool.ref == *origin->shared && tool.notation) label=std::string(*tool.notation);
          context.occurrences.emplace(id,viewport::DisplayContext{"aggregate",id,std::move(label)});
        }
      }
      context.source={{"kind","fixture_only"},{"profile","editable-english-pair"}};
      result["completed_tick"]=state.tick;
      result["generation"]=*state.prepared_generation;
      result["owner_metadata_bytes"]=resources->result().live_bytes;
      result["owner_setup_peak_bytes"]=resources->result().peak_bytes;
      result["live_occurrences"]=Json::array();
      for (std::size_t i=0; i<state.body_records.size(); ++i) {
        const auto &claim=state.body_records[i];
        result["live_occurrences"].push_back({{"id",std::string(state.report.prepared.particles[i].occurrence.id)},
          {"context",claim.context},{"incarnation",claim.incarnation},{"address",claim.address},{"record",claim.record}});
      }
      result["observation"]=viewport::snapshot(state,context);
      std::cout << result.dump() << '\n'; std::cout.flush();
    };
    Json available=Json::array();
    for (const auto &entry : catalogue)
      available.push_back({{"entry",entry.occurrence.source.entry},{"id",assembly::occurrence_text(entry.occurrence)},
          {"definition",warm::key(entry.definition.object)},{"revision",entry.definition.revision.str()}});
    emit({{"command","create"},{"status","ok"},{"catalogue",available},
      {"actual_backend","native_cpu"},{"host_path",path == field::HostPath::BulkCopy ? "bulk" : "direct"},
      {"capacities",{{"particles",limits[field::Budget::Particles]},{"fields",limits[field::Budget::Fields]},
        {"participation",limits[field::Budget::Participation]},{"contacts",limits[field::Budget::Contacts]},
        {"batch",limits[field::Budget::Batch]},{"scratch_bytes",limits[field::Budget::ScratchBytes]}}},
      {"physical_mass_per_occurrence",2.},{"exact_source_mass_per_occurrence",2},
      {"mass_policy","baseline_source_mass"}});
    bool rejected=false;
    std::string line;
    while (std::getline(input,line)) {
      std::istringstream words(line);
      std::string command, argument, extra;
      if (!(words>>command) || command[0] == '#') continue;
      words>>argument; words>>extra;
      try {
        if (command == "quit" && argument.empty()) break;
        if (!extra.empty() && command != "add") throw std::invalid_argument("too many command arguments");
        std::string trailing;
        if (words>>trailing) throw std::invalid_argument("too many command arguments");
        if ((command == "observe" || command == "list") && argument.empty()) {
          emit({{"command",command},{"status","ok"},{"catalogue",available}});
        } else if (command == "tick") {
          const auto ticks=argument.empty() ? 1 : viewport::parse_count("tick",argument,1,1000000);
          for (std::uint64_t i=0; i<ticks; ++i) {
            const auto result=session.tick_prepared(handle,source->owner(),parameters,1000000000,1000000);
            if (result.status != field::TickStatus::Ok) throw std::runtime_error("native tick refused");
          }
          emit({{"command",command},{"status","ok"},{"ticks",ticks}});
        } else if ((command == "add" || command == "remove") && !argument.empty()) {
          const auto entry=viewport::parse_count("entry",argument,0,UINT64_MAX);
          assembly::PreparedOccurrenceEdit edit{{source->study(),{entry,{}}},command == "add",*observe().prepared_generation};
          if (command == "add") {
            bool declared_entry=false;
            for (const auto &item : catalogue) declared_entry |= item.occurrence.source.entry == entry;
            const auto template_entry=extra.empty() ? (declared_entry ? entry : 2)
                : viewport::parse_count("template",extra,0,UINT64_MAX);
            for (const auto &item : catalogue)
              if (item.occurrence.source.entry == template_entry && item.occurrence.source.path.empty())
                edit.clone_from=item.occurrence;
            if (!edit.clone_from) throw std::invalid_argument("template is not in the declared source catalogue");
          }
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
        } else throw std::invalid_argument("expected observe, list, add ENTRY, remove ENTRY, tick [N] or quit");
      } catch (const std::invalid_argument &error) {
        rejected=true; emit({{"command",command},{"status","invalid_input"},{"message",error.what()}});
      }
    }
    if (input.bad()) throw std::runtime_error("command input failed");
    return rejected ? 1 : 0;
  } catch (const std::exception &error) { std::cerr << "live study: " << error.what() << '\n'; return 1; }
}
