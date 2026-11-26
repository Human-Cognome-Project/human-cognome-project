#include "prepared_letter_profile.h"
#include "live_study.h"
#include "capture_options.h"
#include "letter_projection_fixture.h"
#include <iostream>

namespace {
enum class Preset { EditableEnglish, WholeFieldEnglish, UnicodeI, Retained };
void help() {
  std::cout << "viewport_live_study [--threads N] [--direct] [--whole-field | --unicode-i | --retained] [--letter ADDRESS] [--response-work-limit N] [--commands FILE]\n"
      "Commands: observe; list; add ENTRY [TEMPLATE_ENTRY]; add ENTRY DEFINITION REVISION; remove ENTRY; lod aggregate|parts; tick [N]; quit\n"
      "The fixture English-pair catalogue declares entries 2 and 10. Both start live.\n"
      "A new root entry copies a declared template (default 2) within capacity.\n"
      "--unicode-i selects the fixed í cohort. Use lod parts and lod aggregate to exchange C3 AD and í.\n"
      "The fixed cohort reports unsupported for add and remove.\n"
      "--retained runs the shared publishing owner on a fresh dedicated warm v2 database.\n"
      "Use open SELECTION REVISION with the offered retained recipe pin.\n"
      "With --retained, repeat --letter ADDRESS to offer a catalogue and use open DEFINITION REVISION.\n";
}
}
int main(int argc, char **argv) {
  try {
    engine::InstanceSettings settings; settings.read_environment=false; settings.cpu_threads=1;
    auto path=field::HostPath::BulkCopy;
    std::string command_file;
    std::vector<std::string> letters;
    Preset preset=Preset::EditableEnglish;
    std::optional<viewport::ResponseRequest> responses;
    for (int i=1; i<argc; ++i) {
      const std::string arg=argv[i];
      if (arg == "--help") { help(); return 0; }
      if (arg == "--direct") path=field::HostPath::CpuDirect;
      else if (arg == "--whole-field" || arg == "--unicode-i" || arg == "--retained") {
        const auto selected=arg=="--whole-field" ? Preset::WholeFieldEnglish :
            arg=="--unicode-i" ? Preset::UnicodeI : Preset::Retained;
        if (preset!=Preset::EditableEnglish && preset!=selected)
          throw std::invalid_argument("select one study profile");
        preset=selected;
      }
      else if (arg == "--letter" && i+1<argc) letters.emplace_back(argv[++i]);
      else if (arg == "--response-work-limit" && i+1<argc)
        responses=viewport::ResponseRequest{viewport::parse_count("--response-work-limit",argv[++i],0,UINT64_MAX)};
      else if (arg == "--threads" && i+1<argc)
        settings.cpu_threads=int(viewport::parse_count("--threads",argv[++i],1,1024));
      else if (arg == "--commands" && i+1<argc) command_file=argv[++i];
      else throw std::invalid_argument("unknown or incomplete option");
    }
    if (!letters.empty() && preset!=Preset::Retained) throw std::invalid_argument("--letter requires --retained");
    viewport::ignore_live_sigpipe();
    viewport::LiveCommandInput input(command_file,preset==Preset::Retained);
    const viewport::LiveStudyInput run{input,std::cout,settings,path,responses};
    if (preset==Preset::Retained) {
#ifdef HCP_HAS_RETAINED_LIVE_STUDY
      return viewport::run_retained_live_study(run,letters);
#else
      throw std::invalid_argument("retained studies require the warm integration build");
#endif
    }
    letter_fixture::Fixture fixture;
    const bool unicode=preset==Preset::UnicodeI;
    const bool whole_field=preset==Preset::WholeFieldEnglish;
    const char *profile=unicode ? "live-unicode-exchange" :
        whole_field ? "whole-field-english-pair" : "editable-english-pair";
    const char *label=unicode ? "Fixed Unicode í fixture" : "Editable English-pair fixture";
    auto request=[&] {
      if (unicode) {
        const auto endpoint=fixture.ref(fixture.accented);
        return viewport::unicode_i_submission(std::move(fixture.base),endpoint,warm::new_uuid(),path);
      }
      const auto english=fixture.ref(fixture.english);
      if (whole_field) {
        const auto membership=fixture.add(letter_fixture::object("02.00.00.00.00"),"English",0);
        fixture.base.tools.at(fixture.tools.at(fixture.english)).member_of={membership};
        const auto assembled=assembly::ActiveAssembly::assemble(
            viewport::whole_english_pair(std::move(fixture.base),english,membership),warm::new_uuid(),false);
        auto source=std::make_shared<const assembly::ActiveAssembly>(assembled.expose_all(warm::Exposure::Aggregate));
        return viewport::whole_english_pair_submission(std::move(source),english,membership,path);
      }
      auto source=std::make_shared<const assembly::ActiveAssembly>(assembly::ActiveAssembly::assemble(
          viewport::prepared_english_pair(std::move(fixture.base),english),warm::new_uuid(),false));
      return viewport::editable_english_pair_submission(std::move(source),english,path);
    }();
    return viewport::run_live_study(run,std::move(request),label,
        {{"kind","fixture_only"},{"profile",profile}},unicode?4.:2.,unicode?4:2);
  } catch (const std::exception &error) { std::cerr << "live study: " << error.what() << '\n'; return 1; }
}
