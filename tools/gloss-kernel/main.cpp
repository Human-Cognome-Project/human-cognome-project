// gloss-kernel — dedicated process: chew through every resolvable gloss in hcp_english.
// Standalone launcher; kernel class is Gem-lift-ready (see GlossKernel.h).
#include "GlossKernel.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

int main(int argc, char** argv)
{
    hcp::KernelConfig cfg;
    const char* host = std::getenv("PGHOST");     if (!host) host = "192.168.68.60";
    const char* port = std::getenv("PGPORT");     if (!port) port = "5435";
    const char* user = std::getenv("PGUSER");     if (!user) user = "hcp";
    const char* pass = std::getenv("PGPASSWORD"); if (!pass) pass = "hcp_dev";
    cfg.conninfo = std::string("host=") + host + " port=" + port +
                   " dbname=hcp_english user=" + user + " password=" + pass;

    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--include-dated")) cfg.includeDated = true;
        else if (!std::strcmp(argv[i], "--limit") && i + 1 < argc) cfg.limitSenses = std::atol(argv[++i]);
        else if (!std::strcmp(argv[i], "--max-residue") && i + 1 < argc) cfg.maxResidue = std::atoi(argv[++i]);
        else if (!std::strcmp(argv[i], "--max-passes") && i + 1 < argc) cfg.maxPasses = std::atoi(argv[++i]);
        else {
            std::fprintf(stderr,
                "usage: gloss-kernel [--limit N] [--max-residue K] [--max-passes N] [--include-dated]\n");
            return 2;
        }
    }
    hcp::GlossKernel k(cfg);
    return k.Run() ? 0 : 1;
}
