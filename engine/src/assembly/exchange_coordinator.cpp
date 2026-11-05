#include "assembly/exchange_coordinator.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace assembly::detail {
namespace {
void need(bool ok, const char *message) { if (!ok) throw warm::Error(message); }
bool same(const FieldKey &a, const FieldKey &b) {
  return a.kind == b.kind && a.object == b.object;
}
}

ExchangeCoordinator::ExchangeCoordinator(std::unique_ptr<ExchangeBindings> bound,
    const std::vector<ExchangeIncoming> &incoming) : bound_(std::move(bound)) {
  need(bool(bound_), "missing trusted exchange bindings");
  auto &b = *bound_;
  const auto &s = b.h_.prepared_storage();
  need(b.validate(b.generation_, UINT64_MAX).status == field::UpdateStatus::Ok,
       "stale exchange preparation");
  const auto &sources = b.contract_.incoming();
  need(incoming.size() == sources.size(), "incomplete incoming alternative");
  mapping_generation_ = b.generation_;
  for (const auto &o : b.occurrences_)
    if (!b.outgoing_rows_[o.handle.record]) next_.push_back(o);
  survivors_ = next_.size();
  // Definitions always precede the other commands, in declaration order. No
  // numerical start or velocity can enter this structural plan.
  for (const auto &source : sources) {
    const ExchangeIncoming *input = nullptr;
    for (const auto &v : incoming) if (v.occurrence == source.occurrence) {
      need(!input, "duplicate incoming occurrence"); input = &v;
    }
    need(input && input->definition == source.definition && input->node == source.node,
         "incoming definition or alternative differs from declaration");
    need(source.mass && source.mass <= (UINT64_C(1) << 53) && input->kind > 0 &&
         input->kind <= 255 && input->address >= 0 && std::isfinite(input->radius) &&
         input->radius >= 0 && input->geometry.complete,
         "unsupported prepared incoming intrinsic");
    field::UpdateCommand command;
    command.operation = field::UpdateOp::DefineParticle;
    command.address = input->address;
    command.kind = input->kind;
    command.exact_mass = source.mass;
    intrinsics_.push_back({commands_.size(), commands_.size(), double(source.mass),
                           input->radius, input->geometry});
    commands_.push_back(command);
    next_.push_back({source.occurrence, {}});
  }
  for (const auto &handle : b.outgoing_) {
    field::UpdateCommand c;
    c.operation = field::UpdateOp::RetireParticle;
    c.first = field::UpdateRef::existing(handle);
    commands_.push_back(c);
  }
  for (const auto &edge : b.edges_) {
    field::UpdateCommand c;
    c.operation = field::UpdateOp::RemoveParticipation;
    c.row = edge.row; c.generation = b.generation_;
    commands_.push_back(c);
  }
  for (std::size_t row = 0; row < s.live.contacts; ++row) {
    const auto &contact = s.contacts[row];
    if (b.outgoing_rows_[contact.first.record] || b.outgoing_rows_[contact.second.record]) {
      field::UpdateCommand c;
      c.operation = field::UpdateOp::RemoveContact;
      c.row = row; c.generation = b.generation_;
      commands_.push_back(c);
    }
  }
  // Every byte inherits the owning letter's membership set. Binding preparation
  // already checked its exact committed rows; enumerate distinct logical fields.
  std::vector<FieldKey> memberships;
  for (const auto &f : b.fields_) if (f.field.kind == GroupKind::Membership)
    for (const auto &edge : b.edges_)
      if (edge.header.second.record == f.handle.record) {
        memberships.push_back(f.field); break;
      }
  std::size_t position = s.live.edges - b.edges_.size();
  for (std::size_t i = 0; i < sources.size(); ++i) {
    const auto &source = sources[i];
    const auto add = [&](const FieldKey &key, std::uint64_t count, std::uint64_t ordinal) {
      const auto f = std::find_if(b.fields_.begin(), b.fields_.end(),
          [&](const auto &v) { return same(key, v.field); });
      need(f != b.fields_.end(), "incoming exchange field is not prepared");
      need(count && count <= source.mass, "invalid incoming exact participation");
      field::UpdateCommand c;
      c.operation = field::UpdateOp::InsertParticipation;
      c.first = field::UpdateRef::pending(int(i));
      c.second = field::UpdateRef::existing(f->handle);
      c.exact_count = count; c.share = float(double(count) / double(source.mass));
      c.position = position++; c.ordinal = ordinal;
      commands_.push_back(c);
    };
    add({GroupKind::SiblingDefinition, source.definition.object}, source.mass, 0);
    for (const auto &parent : source.parents)
      add({GroupKind::ParentValue, parent.target.object}, parent.contribution, parent.ordinal);
    for (const auto &membership : memberships) add(membership, source.mass, 0);
  }
  need(commands_.size() <= s.capacity.limits()[field::Budget::Batch], "exchange command backing exhausted");
  need(next_.size() <= (UINT64_MAX - 16) / 16 && b.work_ <= UINT64_MAX - 16 * (next_.size() + 1),
       "exchange coordinator work overflow");
  work_ = b.work_ + 16 * (next_.size() + 1);
}

void ExchangeCoordinator::prepare_hierarchy(const field::LiveRefreshSpec &spec) {
  need(!sealed_ && !consumed_, "exchange hierarchy already sealed");
  need(spec.nodes && spec.node_count && (!spec.child_count || spec.children) &&
       (!spec.contribution_count || spec.contributions), "incomplete prospective hierarchy");
  nodes_.assign(spec.nodes, spec.nodes + spec.node_count);
  if (spec.child_count) children_.assign(spec.children, spec.children + spec.child_count);
  if (spec.contribution_count)
    contributions_.assign(spec.contributions, spec.contributions + spec.contribution_count);
  sealed_ = true;
}

field::UpdateResult ExchangeCoordinator::execute(std::uint64_t generation,
    std::uint64_t work, std::uint64_t cells, std::uint64_t hierarchy_work) {
  auto &b = *bound_;
  auto &h = b.h_;
  auto result = b.validate(generation, work);
  if (result.status != field::UpdateStatus::Ok) return result;
  if (consumed_) { result.status = field::UpdateStatus::Stale; return result; }
  if (!sealed_) { result.status = field::UpdateStatus::InvalidInput; return result; }
  work = std::min(work, h.prepared_storage().capacity.limits()[field::Budget::PlacementWork]);
  if (work < work_) {
    result.status = field::UpdateStatus::CapacityExhausted;
    result.failure = {field::Budget::PlacementWork, work_, work};
    return result;
  }
  const field::LiveRefreshSpec spec{nodes_.data(), nodes_.size(), children_.data(), children_.size(),
                                   contributions_.data(), contributions_.size()};
  try {
    result = h.stage_current_admission(generation, commands_.data(), commands_.size(),
        intrinsics_.data(), intrinsics_.size(), spec, work - work_, cells, hierarchy_work,
        true, false, nullptr, 0, b.outgoing_.data(), b.outgoing_.size());
    result.structural_visits += work_;
    if (result.status != field::UpdateStatus::Ok) return result;
    // Native claims, numerical/source/hierarchy/contact publication and retirement
    // succeeded. The sealed command indices resolve the actual native handles.
    // Only fixed-size assignments/swaps remain under the same exclusive boundary.
    const auto &s = h.prepared_storage();
    for (std::size_t i = 0; i < intrinsics_.size(); ++i)
      next_[survivors_ + i].handle = s.resolved[i];
    b.occurrences_.swap(next_);
    mapping_generation_ = result.generation;
    consumed_ = true;
    return result;
  } catch (...) {
    h.ready_ = false;
    throw;
  }
}
}  // namespace assembly::detail
