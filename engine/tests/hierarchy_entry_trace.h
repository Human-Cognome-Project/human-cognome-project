#pragma once
#include "field/live_hierarchy.h"
#include <cstdlib>
// Linker-only observation: production code has no trace/fault switches.
namespace handoff_trace {
struct Calls { unsigned standalone{}, combined{}; };
thread_local Calls *active{};
struct Scope {
  explicit Scope(Calls &calls) { if (active) std::abort(); active = &calls; }
  ~Scope() { active = nullptr; }
  Scope(const Scope &) = delete;
  Scope &operator=(const Scope &) = delete;
};
}
field::LiveHierarchyResult handoff_real_install(field::detail::LiveHierarchy *,
    std::uint64_t, const field::WeightedCascadeSpec &, std::uint64_t)
    __asm__("__real__ZN5field6detail13LiveHierarchy7installEmRKNS_19WeightedCascadeSpecEm");
field::LiveHierarchyResult handoff_wrap_install(field::detail::LiveHierarchy *,
    std::uint64_t, const field::WeightedCascadeSpec &, std::uint64_t)
    __asm__("__wrap__ZN5field6detail13LiveHierarchy7installEmRKNS_19WeightedCascadeSpecEm");
field::LiveHierarchyResult handoff_wrap_install(field::detail::LiveHierarchy *h,
    std::uint64_t generation, const field::WeightedCascadeSpec &spec, std::uint64_t work) {
  if (handoff_trace::active) ++handoff_trace::active->standalone;
  return handoff_real_install(h,generation,spec,work);
}
extern "C" void handoff_real_commit(field::detail::LiveHierarchy *, field::UpdateResult &)
    __asm__("__real__ZN5field6detail13LiveHierarchy14commit_refreshERNS_12UpdateResultE");
extern "C" void handoff_wrap_commit(field::detail::LiveHierarchy *, field::UpdateResult &)
    __asm__("__wrap__ZN5field6detail13LiveHierarchy14commit_refreshERNS_12UpdateResultE");
extern "C" void handoff_wrap_commit(field::detail::LiveHierarchy *h, field::UpdateResult &result) {
  if (handoff_trace::active) ++handoff_trace::active->combined;
  handoff_real_commit(h,result);
}
