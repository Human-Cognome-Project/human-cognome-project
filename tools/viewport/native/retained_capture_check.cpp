// Readback-only acceptance checker. Captures never become admission inputs.
#include "json.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>

namespace {
using Json = nlohmann::json;
using Pins = std::map<std::string,Json>;
constexpr const char *profile = "prepared-retained-english-pair";
const std::string shared = "00000000-0000-0000-0000-000000000000/";
void need(bool ok, const char *why) { if (!ok) throw std::runtime_error(why); }
Json read(const std::filesystem::path &path) {
  std::ifstream in(path); need(bool(in),"missing evidence file");
  return Json::parse(in);
}
bool positive(const Json &j) {
  if (!j.is_string()) return false;
  const auto value = j.get<std::string>();
  return !value.empty() && std::all_of(value.begin(),value.end(),[](char c) { return c >= '0' && c <= '9'; }) &&
      value.find_first_not_of('0') != std::string::npos;
}
Pins pins(const Json &rows) {
  need(rows.is_array() && !rows.empty(),"empty tool pins");
  Pins out;
  for (const auto &p : rows) {
    const auto object = p.at("object").get<std::string>();
    need(object.find(shared) == 0 && p.at("revision").is_string() && p.at("epoch").is_string(),"invalid shared pin");
    need(out.emplace(object,p).second,"duplicate tool pin");
  }
  return out;
}
void claim(const Json &h, int address, int record) {
  need(positive(h.at("context")) && positive(h.at("incarnation")) &&
       h.at("address") == address && h.at("record") == record,"incomplete or wrong native claim");
}
Json qualify(const Json &before, const Json &after, const Json &activity, const Json &warm) {
  need(warm.at("schema") == 2 && warm.at("roots") == 2 && warm.at("leases") == 0 &&
       warm.at("bundles").size() == 1,"wrong warm publication or unreleased lease");
  const auto &bundle = warm.at("bundles").at(0);
  const auto &pin = after.at("source").at("recipe_pin");
  need(bundle.at("selection") == pin.at("selection") && bundle.at("revision") == pin.at("revision") &&
       bundle.at("head") == pin.at("revision") && bundle.at("version") == 2,"recipe pin differs from warm publication");
  const auto actual = pins(warm.at("tools"));
  need(actual == pins(after.at("source").at("tool_pins")),"capture tool pins differ from warm definitions");
  std::map<std::string,Json> definitions;
  for (const auto &d : warm.at("definitions"))
    need(d.at("selection") == pin.at("selection") && d.at("bundle_revision") == pin.at("revision") &&
         definitions.emplace(d.at("object").get<std::string>(),d.at("revision")).second,"wrong bundle definition pin");
  need(definitions.size() == actual.size(),"incomplete bundle definition pins");
  for (const auto &[object,p] : actual) need(definitions.at(object) == p.at("revision"),"wrong definition revision");
  const auto english = shared + "02.00.00.00.03", byte = shared + "00.00.00.00.23";
  need(actual.at(english).at("mass") == "2","retained English exact mass changed");
  for (const auto *address : {"02.00.00.00.01","02.00.00.00.00","02.00.zz.00.00","02.0z.zz.00.00"})
    need(actual.at(shared+address).at("mass").is_null(),"unknown naming mass was filled");
  std::set<std::pair<std::string,std::string>> edges;
  for (const auto &m : warm.at("memberships")) {
    const auto from = m.at("object").get<std::string>(), to = m.at("group").get<std::string>();
    need(m.at("revision") == actual.at(from).at("revision") &&
         m.at("group_revision") == actual.at(to).at("revision") && edges.emplace(from,to).second,
         "unmatched membership pins");
  }
  const auto alpha = shared+"02.00.00.00.01", en = shared+"02.00.00.00.00",
    germanic = shared+"02.00.zz.00.00", indo = shared+"02.0z.zz.00.00";
  need(edges == std::set<std::pair<std::string,std::string>>{
       {english,alpha},{english,en},{alpha,en},{en,germanic},{germanic,indo}},"wrong retained membership closure");
  bool parent = false;
  for (const auto &p : warm.at("parents")) if (p.at("object") == english) {
    need(!parent && p.at("parent") == byte && p.at("ordinal") == 0 &&
         p.at("stored_mass").is_null() && p.at("contribution") == "2","retained English parent changed");
    parent = true;
  }
  need(parent,"missing retained English parent");
  need(activity.at("study_profile") == profile && activity.at("backend") == "native_cpu" &&
       activity.at("ticks") == 1 && activity.at("requested_ticks") == 1 &&
       activity.at("capture_count") == 2 && activity.at("stopped_by_signal") == false &&
       activity.at("warm_activity") == Json{{"publications",1},{"acquisitions",1},{"releases",1}},
       "incomplete actual acquisition/tick/release cycle");
  need(activity.at("native_demand") == Json{{"fields_loaded",6},{"edges_loaded",12},{"force_requests",12},
       {"force_evaluations",2},{"force_zero_response",10}},"wrong deduplicated cohort behavior");
  Json mapped = Json::array();
  for (const auto *capture : {&before,&after}) {
    const auto &v = *capture, &exec = v.at("execution"), &c = v.at("capture");
    const auto tick = capture == &before ? "0" : "1";
    need(v.at("schema") == "hcp-study-snapshot" && v.at("version") == 1 &&
         v.at("numerics") == Json{{"profile","trial_f64"},{"storage_bits",64},
           {"body_fraction_bits",52},{"field_fraction_bits",52}},"wrong snapshot/numerical profile");
    need(v.at("source").at("kind") == "retained_cold_to_warm_to_active" &&
         v.at("source").at("study_profile") == profile && v.at("source") == after.at("source"),"source pins changed across tick");
    need(c.at("study").is_string() && c.at("session") == before.at("capture").at("session") &&
         c.at("study") == before.at("capture").at("study") &&
         c.at("generation") == before.at("capture").at("generation") &&
         c.at("tick") == tick && c.at("phase") == (capture == &before ? "published_seeded" : "post_tick"),
         "capture identity/tick mismatch");
    need(exec.at("backend") == "native_cpu" && exec.at("mode") == "installed_hierarchy" &&
         positive(exec.at("prepared_generation")) &&
         exec.at("completed_tick") == tick,"missing installed native generation/tick");
    const auto &particles = v.at("particles"), &bodies = exec.at("body_records"), &nodes = exec.at("comparisons");
    need(particles.size() == 2 && bodies.size() == 2 && nodes.size() == 6,"incomplete native capture");
    need(bodies == before.at("execution").at("body_records") &&
         nodes == before.at("execution").at("comparisons"),"native claims changed across tick");
    std::set<std::string> occurrences;
    std::set<int> records;
    for (std::size_t i=0; i<2; ++i) {
      const auto &p = particles.at(i), &h = bodies.at(i);
      const auto id = p.at("id").get<std::string>(), study = c.at("study").get<std::string>();
      const auto first = id == study+"/10", second = id == study+"/2";
      need((first || second) && occurrences.insert(id).second && records.insert(p.at("slot").get<int>()).second &&
           p.at("definition") == english && p.at("exact_mass") == "2" && p.at("mass") == 1.5 &&
           p.at("radius") == 0.125 && p.at("exposure") == "aggregate" && p.at("binding") == 0,
           "wrong retained occurrence mapping or intrinsics");
      claim(h,second ? 40 : 41,p.at("slot").get<int>());
      if (capture == &after) mapped.push_back({{"occurrence",id},{"claim",h}});
    }
    for (std::size_t i=0; i<6; ++i) {
      const auto &n = nodes.at(i);
      claim(n.at("field"),int(i),int(i));
      need(n.at("comparison") == std::to_string(100+i) && n.at("level") == (i ? 1 : 0) &&
           n.at("calculation_bits") == 64 && n.at("storage_bits") == 64 && n.at("fraction_bits") == 52,
           "wrong comparison identity/precision");
    }
    const auto &focus = v.at("frame").at("focal_fields");
    need(focus.size() == 1 && focus.at(0).at("kind") == "parent_value" &&
         focus.at(0).at("value") == byte && focus.at(0).at("mass") == 3,"wrong ParentValue focus");
  }
  // A tick may advance the numeric generation when coverage changes. It must
  // remain the observed generation of that capture, never a fixed seed value.
  need(std::stoull(after.at("execution").at("prepared_generation").get<std::string>()) >=
       std::stoull(before.at("execution").at("prepared_generation").get<std::string>()),"native generation regressed");
  return {{"profile",profile},{"outcome","verified_retained_source_to_tick"},{"retained_tick_qualified",true},
    {"pinned_recipe",pin},{"tool_pins",after.at("source").at("tool_pins")},{"actual_claims",mapped},
    {"comparisons",after.at("execution").at("comparisons")},
    {"native_generation",after.at("execution").at("prepared_generation")},{"completed_tick","1"},
    {"warm_leases",0},{"warm_activity",activity.at("warm_activity")},{"native_demand",activity.at("native_demand")},
    {"lease_release","verified_after_acquisition"}};
}
}
int main(int argc, char **argv) {
  try {
    need(argc == 2,"supply evidence directory");
    const std::filesystem::path dir(argv[1]);
    const auto before = read(dir/"capture/letter-study-tick0.json"), after = read(dir/"capture/letter-study.json"),
      activity = read(dir/"capture/native-evidence.json"), warm = read(dir/"warm-after.json");
    auto value = qualify(before,after,activity,warm);
    // Corrupt real readbacks in memory. Missing pins/claims/ticks/releases must
    // never be qualified by a plausible-looking producer exit or final capture.
    int controls = 0;
    for (int i=0; i<9; ++i) {
      auto bad = after, bad_activity = activity, bad_warm = warm;
      if (i == 0) bad["source"]["recipe_pin"]["revision"] = "wrong";
      if (i == 1) bad["source"]["tool_pins"][0]["revision"] = "wrong";
      if (i == 2) bad["execution"]["body_records"][0]["incarnation"] = "0";
      if (i == 3) bad["execution"]["comparisons"][5]["comparison"] = "999";
      if (i == 4) bad["execution"]["prepared_generation"] = nullptr;
      if (i == 5) bad["execution"]["completed_tick"] = "0";
      if (i == 6) bad_warm["leases"] = 1;
      if (i == 7) bad_activity["warm_activity"]["acquisitions"] = 0;
      if (i == 8) for (auto &p : bad_warm["tools"])
        if (p.at("object") == shared+"02.00.00.00.00") p["mass"] = "1";
      bool refused = false;
      try { qualify(before,bad,bad_activity,bad_warm); } catch (const std::exception &) { refused = true; }
      need(refused,"corrupt evidence accepted"); ++controls;
    }
    value["corrupt_readback_controls"] = controls;
    std::ofstream out(dir/"qualification.json"); out << value.dump(2) << '\n';
    need(bool(out),"cannot write qualification");
    std::cout << "Verified acquired retained pins, two full native claims, six comparisons, tick 1 and lease release; "
              << controls << " corrupt readbacks refused.\n";
    return 0;
  } catch (const std::exception &e) { std::cerr << "retained acceptance: " << e.what() << '\n'; return 1; }
}
