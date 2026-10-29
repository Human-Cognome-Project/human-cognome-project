// Offline audit of the emitted PTX, complementing semantic LLVM audits and
// on-device packet tests. Restricted to this qualification's sm_61/PTX 6.3.
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
namespace {
void require(bool ok, const char *why) {
  if (!ok) throw std::runtime_error(why);
}
std::vector<std::string> lines(const std::string &text) {
  std::vector<std::string> out;
  std::istringstream input(text);
  for (std::string line; std::getline(input, line);) {
    const auto begin = line.find_first_not_of(" \t\r");
    if (begin == std::string::npos || line.compare(begin, 2, "//") == 0) continue;
    const auto end = line.find_last_not_of(" \t\r");
    out.push_back(line.substr(begin, end - begin + 1));
  }
  return out;
}
int check(const std::vector<std::string> &code, bool require_sites = true) {
  const std::regex selector(R"(^atom\.(or|xor)\.b32\s+[^,]+,\s*\[[^;]+\],\s*(0|1);$)");
  int sites = 0;
  bool target = false, version = false;
  for (std::size_t i = 0; i < code.size(); ++i) {
    if (code[i] == ".target sm_61") target = true;
    if (code[i] == ".version 6.3") version = true;
    std::smatch match;
    if (!std::regex_match(code[i], match, selector)) continue;
    if (!((match[1] == "or" && match[2] == "0") ||
          (match[1] == "xor" && match[2] == "1"))) continue;
    require(i && i + 1 < code.size() && code[i-1] == "membar.gl;" &&
                code[i+1] == "membar.gl;",
            "final PTX selector lacks adjacent device-wide barriers");
    ++sites;
  }
  require(target && version, "PTX target/version outside qualified contract");
  require(!require_sites || sites, "final PTX has no recognized selector sites");
  return sites;
}
}
int main(int argc, char **argv) {
  try {
    require(argc >= 2, "usage: cuda_ptx_audit_test PTX_FILE [...]");
    int files = 0, sites = 0, negatives = 0;
    for (int a = 1; a < argc; ++a) {
      std::ifstream input(argv[a]);
      require(bool(input), "cannot read PTX file");
      const std::string text((std::istreambuf_iterator<char>(input)), {});
      auto code = lines(text);
      const int n = check(code);
      sites += n;
      ++files;
      // Remove each side of one real selector independently; the audit must
      // reject both. Empty/fake files cannot produce a passing audit.
      std::size_t selected = 0;
      for (std::size_t i = 1; i + 1 < code.size(); ++i)
        if ((code[i].find("atom.or.b32") == 0 || code[i].find("atom.xor.b32") == 0) &&
            code[i-1] == "membar.gl;" && code[i+1] == "membar.gl;") {
          // Only select an operation recognized by check().
          const auto candidate = std::vector<std::string>{".version 6.3", ".target sm_61",
              "membar.gl;", code[i], "membar.gl;"};
          if (check(candidate, false)) { selected = i; break; }
        }
      require(selected, "no PTX negative-control site");
      for (int side : {-1, 1}) {
        auto broken = code;
        broken.erase(broken.begin() + selected + side);
        bool rejected = false;
        try { check(broken); }
        catch (const std::runtime_error &) { rejected = true; }
        require(rejected, "PTX audit accepted missing barrier");
        ++negatives;
      }
      std::cout << "ptx_file=" << std::filesystem::path(argv[a]).filename().string()
                << ",selector_sites=" << n << " PASS\n";
    }
    std::cout << "cuda_ptx_audit,files=" << files << ",selector_sites=" << sites
              << ",missing_barrier_rejections=" << negatives << " PASS\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
