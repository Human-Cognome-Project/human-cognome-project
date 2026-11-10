#include "assembly/exchange_coordinator.h"
#include "field/placement.h"
#include "engine/transfer.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace assembly::detail {
namespace {
void need(bool ok, const char *message) { if (!ok) throw warm::Error(message); }
}

ExchangeCoordinator::ExchangeCoordinator(field::Harness &h, const std::shared_ptr<ReservationDomain> &domain)
    : bound_(reserve_unique<ExchangeBindings>(domain,[&](auto *p) {
        ::new (static_cast<void *>(p)) ExchangeBindings(h,domain);
      })), commands_(ReservationAllocator<field::UpdateCommand>(domain.get())),
      intrinsics_(ReservationAllocator<field::PlacementIntrinsic>(domain.get())),
      next_(domain), next_fields_(domain),
      field_commands_(ReservationAllocator<int>(domain.get())), nodes_(ReservationAllocator<field::LiveRefreshNode>(domain.get())),
      children_(ReservationAllocator<int>(domain.get())), contributions_(ReservationAllocator<field::LiveRefreshContribution>(domain.get())) {
  reserve(h);
}

void ExchangeCoordinator::reserve(const field::Harness &h) {
  const auto &limits = h.prepared_storage().capacity.limits();
  commands_.reserve(limits[field::Budget::Batch]);
  intrinsics_.reserve(limits[field::Budget::Batch]);
  nodes_.reserve(limits[field::Budget::CascadeNodes]);
  children_.reserve(limits[field::Budget::CascadeLinks]);
  contributions_.reserve(limits[field::Budget::CascadeReferences]);
  field_commands_.reserve(exchange_map_capacity(limits[field::Budget::Fields],
      std::min(limits[field::Budget::Fields],limits[field::Budget::Batch])));
}

ExchangeCoordinator::ExchangeCoordinator(ReservedUnique<ExchangeBindings> bound,
    const std::vector<ExchangeIncoming> &incoming,
    const std::vector<ExchangeNewField> &new_fields) : bound_(std::move(bound)) {
  need(bool(bound_), "missing trusted exchange bindings");
  reserve(bound_->h_);
  auto rows = bound_->occurrences_;
  auto fields = bound_->fields_;
  next_.swap(rows);
  next_fields_.swap(fields);
  // Standalone setup remains separately scoped from operational preparation.
  need(prepare(incoming, new_fields,UINT64_MAX).status == PreparationStatus::Ok,
       "setup command work overflow");
}

PreparationResult ExchangeCoordinator::prepare(const std::vector<ExchangeIncoming> &incoming,
    const std::vector<ExchangeNewField> &new_fields, std::uint64_t allowance) {
  sealed_ = false;
  consumed_ = true; // A failed preparation must never expose a partial plan.
  PreparationWork meter(allowance);
  try {
  meter.charge(1); // Fixed admission before clearing candidate logical lengths.
  commands_.clear(); intrinsics_.clear();
  next_.clear(); next_fields_.clear(); field_commands_.clear();
  nodes_.clear(); children_.clear(); contributions_.clear();
  need(bool(bound_), "missing trusted exchange bindings");
  auto &b = *bound_;
  const auto &s = b.h_.prepared_storage();
  const auto append = [&](const field::UpdateCommand &command) {
    need(commands_.size() < s.capacity.limits()[field::Budget::Batch],
         "exchange command backing exhausted");
    meter.charge(sizeof(command)); commands_.push_back(command);
  };
  meter.charge(b.work_);
  need(b.validate(b.generation_, UINT64_MAX).status == field::UpdateStatus::Ok,
       "stale exchange preparation");
  need(b.fields_.size() == s.live.fields,
       "exchange field lifecycle requires a complete trusted field inventory");
  const auto &sources = b.contract_->incoming();
  need(incoming.size() == sources.size(), "incomplete incoming alternative");
  need(sources.size() <= s.capacity.limits()[field::Budget::Batch] &&
       b.contract_->mass() <= (UINT64_C(1) << 53), "unsupported exchange declaration size or mass");
  mapping_generation_ = b.generation_;
  need(new_fields.size() <= s.capacity.limits()[field::Budget::Fields],
       "exchange field preparation exceeds capacity");
  for (const auto &f : b.fields_) {
    meter.charge(sizeof(ExchangeFieldBinding)); meter.copy_key(f.field.object);
    next_fields_.push_back(f);
  }
  meter.elements(next_fields_.size(),sizeof(int));
  field_commands_.assign(next_fields_.size(), -1);
  for (const auto &o : b.occurrences_) {
    meter.charge(1);
    if (!b.outgoing_rows_[o.handle.record]) {
      meter.charge(sizeof(OccurrenceBinding)); meter.copy_key(o.occurrence);
      next_.push_back(o);
    }
  }
  survivors_ = next_.size();
  // Definitions always precede the other commands, in declaration order. No
  // numerical start or velocity can enter this structural plan.
  for (const auto &source : sources) {
    meter.charge(1);
    const ExchangeIncoming *input = nullptr;
    for (const auto &v : incoming) if (meter.equal(v.occurrence,source.occurrence)) {
      need(!input, "duplicate incoming occurrence"); input = &v;
    }
    need(input && meter.equal(input->definition,source.definition) && input->node == source.node,
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
    meter.charge(sizeof(field::PlacementIntrinsic));
    intrinsics_.push_back({commands_.size(), commands_.size(), double(source.mass),
                           input->radius, input->geometry});
    append(command);
    meter.charge(sizeof(OccurrenceBinding)); meter.copy_key(source.occurrence);
    next_.push_key(source.occurrence, engine::RecordHandle{});
  }
  for (const auto &f : new_fields) {
    meter.charge(1);
    need(f.kind > 0 && f.kind <= 255 && f.address >= 0,
         "invalid incoming field binding");
    for (const auto &prior : next_fields_) {
      meter.charge(1);
      need(!(prior.field.kind == f.field.kind && meter.equal(prior.field.object,f.field.object)),
           "duplicate or already committed exchange field");
    }
    field::UpdateCommand c;
    c.operation = field::UpdateOp::DefineField;
    c.kind = f.kind; c.address = f.address;
    meter.charge(sizeof(ExchangeFieldBinding)); meter.copy_key(f.field.object);
    next_fields_.push_key(f.field, engine::RecordHandle{});
    meter.charge(sizeof(int));
    field_commands_.push_back(int(commands_.size()));
    append(c);
  }
  for (const auto &handle : b.outgoing_) {
    meter.charge(1);
    field::UpdateCommand c;
    c.operation = field::UpdateOp::RetireParticle;
    c.first = field::UpdateRef::existing(handle);
    append(c);
  }
  for (const auto &edge : b.edges_) {
    meter.charge(1);
    field::UpdateCommand c;
    c.operation = field::UpdateOp::RemoveParticipation;
    c.row = edge.row; c.generation = b.generation_;
    append(c);
  }
  for (std::size_t row = 0; row < s.live.contacts; ++row) {
    meter.charge(1);
    const auto &contact = s.contacts[row];
    if (b.outgoing_rows_[contact.first.record] || b.outgoing_rows_[contact.second.record]) {
      field::UpdateCommand c;
      c.operation = field::UpdateOp::RemoveContact;
      c.row = row; c.generation = b.generation_;
      append(c);
    }
  }
  // Memberships retain exact occurrence identity across both alternatives.
  // Only incoming rows are emitted; inactive catalogue rows are never live edges.
  std::size_t position = s.live.edges - b.edges_.size();
  for (std::size_t i = 0; i < sources.size(); ++i) {
    meter.charge(1);
    const auto &source = sources[i];
    const auto add = [&](GroupKind kind, const warm::ObjectKey &object,
                         std::uint64_t count, std::uint64_t ordinal) {
      meter.charge(1);
      const auto f = std::find_if(next_fields_.begin(), next_fields_.end(),
          [&](const auto &v) { meter.charge(1); return v.field.kind == kind && meter.equal(v.field.object,object); });
      need(f != next_fields_.end(), "incoming exchange field is not prepared");
      need(count && count <= source.mass, "invalid incoming exact participation");
      field::UpdateCommand c;
      c.operation = field::UpdateOp::InsertParticipation;
      c.first = field::UpdateRef::pending(int(i));
      const int command = field_commands_[f - next_fields_.begin()];
      c.second = command < 0 ? field::UpdateRef::existing(f->handle)
                             : field::UpdateRef::pending(command);
      c.exact_count = count; c.share = float(double(count) / double(source.mass));
      c.position = position++; c.ordinal = ordinal;
      append(c);
    };
    add(GroupKind::SiblingDefinition, source.definition.object, source.mass, 0);
    for (const auto &parent : source.parents) {
      meter.charge(1);
      add(GroupKind::ParentValue, parent.target.object, parent.contribution, parent.ordinal);
    }
    // Preserve the trusted committed field order used before catalogue reuse.
    for (const auto &field : b.fields_) {
      meter.charge(1);
      if (field.field.kind == GroupKind::Membership)
      for (const auto &membership : b.memberships_->contributions())
        if (meter.equal(membership.occurrence,source.occurrence) &&
            membership.field.kind == field.field.kind && meter.equal(membership.field.object,field.field.object))
          add(membership.field.kind, membership.field.object, membership.count, 0);
    }
  }
  // Only touched outgoing fields may disappear. Retained source participation,
  // including another occurrence's use, prevents retirement. Focus protection is
  // enforced by ordinary native source preflight, with no implicit focus change.
  for (std::size_t i = 0; i < next_fields_.size();) {
    meter.charge(1);
    const auto &f = next_fields_[i];
    const int command = field_commands_[i];
    bool used = false, touched = false;
    if (command < 0) {
      for (std::size_t row = 0; row < s.live.edges; ++row) {
        meter.charge(1);
        if (s.edges[row].second.record == f.handle.record) {
          touched |= b.edge_rows_[row];
          used |= !b.edge_rows_[row];
        }
      }
    }
    for (const auto &c : commands_) {
      meter.charge(1);
      if (c.operation == field::UpdateOp::InsertParticipation)
      used |= command < 0 ? c.second.command < 0 && c.second.handle.record == f.handle.record
                          : c.second.command == command;
    }
    need(command < 0 || used, "undeclared extra incoming field");
    if (touched && !used) {
      field::UpdateCommand c;
      c.operation = field::UpdateOp::RetireField;
      c.first = field::UpdateRef::existing(f.handle);
      append(c);
      meter.elements(next_fields_.size()-i,sizeof(ExchangeFieldBinding)+sizeof(int));
      next_fields_.erase(next_fields_.begin() + i);
      field_commands_.erase(field_commands_.begin() + i);
    } else ++i;
  }
  need(commands_.size() <= s.capacity.limits()[field::Budget::Batch], "exchange command backing exhausted");
  const auto entries = next_.size() + next_fields_.size();
  need(entries <= (UINT64_MAX - 16) / 16 && b.work_ <= UINT64_MAX - 16 * (entries + 1),
       "exchange coordinator work overflow");
  work_ = b.work_ + 16 * (entries + 1);
  consumed_ = false;
  return meter.result();
  } catch (const PreparationStop &stop) {
    // Partial candidate metadata remains reserved, but cannot seal or execute.
    consumed_ = true; sealed_ = false;
    return stop.result;
  }
}

HierarchySealResult ExchangeCoordinator::hierarchy_charge(const field::LiveRefreshSpec &spec) const {
  HierarchySealResult out;
  if (!seal_add(out.input_bytes,spec.node_count,sizeof(field::LiveRefreshNode)) ||
      !seal_add(out.input_bytes,spec.child_count,sizeof(int)) ||
      !seal_add(out.input_bytes,spec.contribution_count,sizeof(field::LiveRefreshContribution)) ||
      !seal_add(out.retained_bytes,nodes_.capacity(),sizeof(field::LiveRefreshNode)) ||
      !seal_add(out.retained_bytes,children_.capacity(),sizeof(int)) ||
      !seal_add(out.retained_bytes,contributions_.capacity(),sizeof(field::LiveRefreshContribution)))
    return {}; // No partial charge escapes on overflow.
  out.required_bytes = out.retained_bytes;
  if (!seal_add(out.required_bytes,out.input_bytes,1)) return {};
  out.required_work = out.input_bytes; // One copy-work unit per payload byte.
  out.status = HierarchySealStatus::Ok;
  return out;
}

HierarchySealResult ExchangeCoordinator::prepare_hierarchy(
    const field::LiveRefreshSpec &spec, HierarchySealBudget budget) {
  need(!sealed_ && !consumed_, "exchange hierarchy already sealed");
  auto charge = hierarchy_charge(spec);
  if (charge.status != HierarchySealStatus::Ok) return charge;
  if (budget.bytes < charge.required_bytes) {
    charge.status = HierarchySealStatus::ByteExhausted; return charge;
  }
  if (budget.copy_work < charge.required_work) {
    charge.status = HierarchySealStatus::WorkExhausted; return charge;
  }
  need(spec.nodes && spec.node_count && (!spec.child_count || spec.children) &&
       (!spec.contribution_count || spec.contributions), "incomplete prospective hierarchy");
  const auto &limits = bound_->h_.prepared_storage().capacity.limits();
  need(spec.node_count <= limits[field::Budget::CascadeNodes] &&
       spec.child_count <= limits[field::Budget::CascadeLinks] &&
       spec.contribution_count <= limits[field::Budget::CascadeReferences],
       "exchange hierarchy preparation exceeds capacity");
  nodes_.assign(spec.nodes, spec.nodes + spec.node_count);
  if (spec.child_count) children_.assign(spec.children, spec.children + spec.child_count);
  if (spec.contribution_count)
    contributions_.assign(spec.contributions, spec.contributions + spec.contribution_count);
  sealed_ = true;
  return charge;
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
    std::array<double, 8> outgoing;
    std::array<double, 32> balance;
    engine::readback(*h.placement_->exchange_state_, outgoing.data(), sizeof(outgoing));
    engine::readback(*h.placement_->exchange_balance_, balance.data(), sizeof(balance));
    result.readback_bytes += sizeof(outgoing) + sizeof(balance);
    // Native claims, numerical/source/hierarchy/contact publication and retirement
    // succeeded. The sealed command indices resolve the actual native handles.
    // Only fixed-size assignments/swaps remain under the same exclusive boundary.
    const auto &s = h.prepared_storage();
    if (s.failure_point == 7)
      throw std::runtime_error("injected failure before exchange mapping publication");
    for (std::size_t i = 0; i < intrinsics_.size(); ++i)
      next_[survivors_ + i].handle = s.resolved[i];
    for (std::size_t i = 0; i < next_fields_.size(); ++i)
      if (field_commands_[i] >= 0) next_fields_[i].handle = s.resolved[field_commands_[i]];
    b.occurrences_.swap(next_);
    b.fields_.swap(next_fields_);
    outgoing_report_ = outgoing;
    balance_report_ = balance;
    mapping_generation_ = result.generation;
    consumed_ = true;
    return result;
  } catch (...) {
    h.ready_ = false;
    throw;
  }
}
}  // namespace assembly::detail
