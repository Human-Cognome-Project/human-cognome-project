// ============================================================================
// HCPEulerReassembly_FUTURE.cpp — Bond-only reassembly via Eulerian path
// ============================================================================
//
// STATUS: Parked for future revisit after conceptual modeling begins.
//
// CONTEXT (2026-02-23):
// This is a working Hierholzer's algorithm implementation that finds AN
// Eulerian path through the PBM bond graph. It correctly consumes all edges
// (verified: 10481/10481 on Yellow Wallpaper, valid Euler graph with 0
// imbalanced nodes).
//
// PROBLEM: The Euler path is not unique. Hub tokens (common words like "the")
// have many outgoing edges, and the algorithm picks a valid but wrong
// traversal order. The output uses all the same bonds but reconstructs a
// different text than the original.
//
// WHY WE'RE KEEPING IT: Once the conceptual mesh is operational, it should
// provide much stronger constraints for disambiguating edge selection at
// hub nodes. At that point, bond-only reassembly may become viable again
// with the mesh as an additional constraint layer — eliminating the need
// for positional storage.
//
// CURRENT SOLUTION: Dual storage — PBM bonds for inference + positional
// tree for exact reconstruction. See HCPParticlePipeline::Reassemble()
// for the active implementation.
//
// Includes an Euler path diagnostic that verifies degree balance before
// attempting reconstruction. Useful for validating bond graph integrity.
// ============================================================================

#include "HCPParticlePipeline.h"
#include "HCPVocabulary.h"
#include <AzCore/std/sort.h>

namespace HCPEngine
{

// Euler path diagnostic: verifies the bond graph has valid degree conditions.
// Returns true if exactly 1 start node (out=in+1), 1 end node (in=out+1),
// and all other nodes are balanced (out==in).
bool DiagnoseEulerConditions(const PBMData& pbmData)
{
    AZStd::unordered_map<AZStd::string, int> inDeg, outDeg;
    for (const auto& bond : pbmData.bonds)
    {
        outDeg[bond.tokenA] += bond.count;
        inDeg[bond.tokenB] += bond.count;
    }

    AZStd::unordered_map<AZStd::string, bool> allTokens;
    for (const auto& [t, _] : inDeg) { allTokens[t] = true; }
    for (const auto& [t, _] : outDeg) { allTokens[t] = true; }

    int startNodes = 0, endNodes = 0, imbalanced = 0;
    for (const auto& [token, _] : allTokens)
    {
        int out = outDeg.count(token) ? outDeg[token] : 0;
        int in = inDeg.count(token) ? inDeg[token] : 0;
        if (out == in + 1)
        {
            startNodes++;
            fprintf(stderr, "[Euler] START node: %s (out=%d, in=%d)\n",
                token.c_str(), out, in);
        }
        else if (in == out + 1)
        {
            endNodes++;
            fprintf(stderr, "[Euler] END node: %s (out=%d, in=%d)\n",
                token.c_str(), out, in);
        }
        else if (out != in)
        {
            imbalanced++;
            fprintf(stderr, "[Euler] IMBALANCED: %s (out=%d, in=%d, diff=%d)\n",
                token.c_str(), out, in, out - in);
        }
    }
    fprintf(stderr, "[Euler] Summary: %d start, %d end, %d imbalanced, %zu total nodes\n",
        startNodes, endNodes, imbalanced, allTokens.size());
    fflush(stderr);

    return (startNodes == 1 && endNodes == 1 && imbalanced == 0);
}

// Hierholzer's algorithm with rare-first edge selection.
// Finds AN Eulerian path (not necessarily THE original sequence).
// Consumes all edges on a valid Euler graph.
AZStd::vector<AZStd::string> ReassembleViaEulerPath(const PBMData& pbmData)
{
    AZStd::vector<AZStd::string> sequence;

    if (pbmData.bonds.empty() || pbmData.firstFpbA.empty())
    {
        return sequence;
    }

    struct Edge
    {
        AZStd::string target;
        int remaining;
    };
    AZStd::unordered_map<AZStd::string, AZStd::vector<Edge>> outgoing;

    size_t totalEdges = 0;
    for (const auto& bond : pbmData.bonds)
    {
        outgoing[bond.tokenA].push_back({bond.tokenB, bond.count});
        totalEdges += bond.count;
    }

    // Sort each token's outgoing edges: rarest first
    for (auto& [token, edges] : outgoing)
    {
        AZStd::sort(edges.begin(), edges.end(),
            [](const Edge& a, const Edge& b) { return a.remaining < b.remaining; });
    }

    // Stack-based Hierholzer's
    AZStd::vector<AZStd::string> stack;
    AZStd::vector<AZStd::string> result;
    result.reserve(totalEdges + 1);

    stack.push_back(pbmData.firstFpbA);

    while (!stack.empty())
    {
        const AZStd::string& current = stack.back();

        bool found = false;
        {
            auto it = outgoing.find(current);
            if (it != outgoing.end())
            {
                auto& edges = it->second;
                for (auto& edge : edges)
                {
                    if (edge.remaining > 0)
                    {
                        edge.remaining--;
                        AZStd::string nextToken = edge.target;
                        AZStd::sort(edges.begin(), edges.end(),
                            [](const Edge& a, const Edge& b) {
                                return a.remaining < b.remaining;
                            });
                        stack.push_back(AZStd::move(nextToken));
                        found = true;
                        break;
                    }
                }
            }
        }

        if (!found)
        {
            result.push_back(stack.back());
            stack.pop_back();
        }
    }

    // Hierholzer's produces the path in reverse
    sequence.reserve(result.size());
    for (auto it = result.rbegin(); it != result.rend(); ++it)
    {
        sequence.push_back(AZStd::move(*it));
    }

    size_t edgesConsumed = sequence.size() > 0 ? sequence.size() - 1 : 0;
    fprintf(stderr, "[EulerReassembly] %zu tokens, %zu/%zu edges consumed\n",
        sequence.size(), edgesConsumed, totalEdges);
    fflush(stderr);

    return sequence;
}

} // namespace HCPEngine
