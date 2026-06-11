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
    std::string dbname = "hcp_english";

    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--include-dated")) cfg.includeDated = true;
        else if (!std::strcmp(argv[i], "--keep-case")) cfg.keepCase = true;
        else if (!std::strcmp(argv[i], "--limit") && i + 1 < argc) cfg.limitSenses = std::atol(argv[++i]);
        else if (!std::strcmp(argv[i], "--max-residue") && i + 1 < argc) cfg.maxResidue = std::atoi(argv[++i]);
        else if (!std::strcmp(argv[i], "--max-passes") && i + 1 < argc) cfg.maxPasses = std::atoi(argv[++i]);
        else if (!std::strcmp(argv[i], "--dbname") && i + 1 < argc) dbname = argv[++i];
        else if (!std::strcmp(argv[i], "--suffix") && i + 1 < argc) cfg.tableSuffix = argv[++i];
        else if (!std::strcmp(argv[i], "--word-regex") && i + 1 < argc) cfg.wordRegex = argv[++i];
        else {
            std::fprintf(stderr,
                "usage: gloss-kernel [--dbname DB] [--suffix _lang] [--word-regex RE] [--keep-case]\n"
                "                    [--limit N] [--max-residue K] [--max-passes N] [--include-dated]\n"
                "language = data: same engine, different {coremap,scaffold,lemma_fix,patterns}<suffix>\n");
            return 2;
        }
    }
    cfg.conninfo = std::string("host=") + host + " port=" + port +
                   " dbname=" + dbname + " user=" + user + " password=" + pass;
    hcp::GlossKernel k(cfg);
    return k.Run() ? 0 : 1;
}
