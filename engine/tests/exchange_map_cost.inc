bool map_cost_handle_equal(engine::RecordHandle a, engine::RecordHandle b) {
  return a.context==b.context && a.incarnation==b.incarnation && a.address==b.address && a.record==b.record;
}
bool map_cost_row_equal(const assembly::detail::OccurrenceBinding &a,
                        const assembly::detail::OccurrenceBinding &b) {
  return a.occurrence==b.occurrence && map_cost_handle_equal(a.handle,b.handle);
}
bool map_cost_row_equal(const assembly::detail::ExchangeFieldBinding &a,
                        const assembly::detail::ExchangeFieldBinding &b) {
  return a.field.kind==b.field.kind && a.field.object==b.field.object && map_cost_handle_equal(a.handle,b.handle);
}
const warm::Uuid &map_cost_uuid(const assembly::detail::OccurrenceBinding &row) { return row.occurrence.study; }
const warm::Uuid &map_cost_uuid(const assembly::detail::ExchangeFieldBinding &row) { return row.field.object.scope; }
template<class A, class B> bool map_cost_equal(const A &a, const B &b) {
  if (a.size()!=b.size()) return false;
  for (std::size_t i=0; i<a.size(); ++i) if (!map_cost_row_equal(a[i],b[i])) return false;
  return true;
}
template<class Visit> void map_cost_pairs(const assembly::detail::ExchangeOwner &owner, Visit visit) {
  using Access=assembly::testing::ExchangeOwnerAccess;
  using Plan=assembly::testing::ExchangeCoordinatorAccess;
  const auto &plan=Access::plan(owner);
  visit(0,Access::mappings(owner),Access::fields(owner));
  visit(1,Plan::mappings(plan),Plan::fields(plan));
  visit(2,Plan::planned_mappings(plan),Plan::planned_fields(plan));
}

// This independent observer owns scratch outside the owner-adoption window.
// No production maps or publication protocol are changed by the inspection.
void exchange_binding_map_cost(Fixture &f, const assembly::LetterProjection &projection,
    const std::vector<assembly::detail::OccurrenceBinding> &rows,
    const std::vector<assembly::detail::ExchangeFieldBinding> &fields) {
  using Access=assembly::testing::ExchangeOwnerAccess;
  using Plan=assembly::testing::ExchangeCoordinatorAccess;
  using namespace assembly::detail;
  namespace A=allocation_lifetime;
  struct Buffer { const void *data; std::size_t bytes; };
  const auto &limits=f.h.prepared_storage().capacity.limits();
  const auto row_slots=exchange_map_capacity(limits[field::Budget::Particles],limits[field::Budget::Batch]);
  const auto field_slots=exchange_map_capacity(limits[field::Budget::Fields],
      std::min(limits[field::Budget::Fields],limits[field::Budget::Batch]));
  std::array<std::vector<Buffer>,3> buffers;
  for (auto &family : buffers) family.reserve(2+2*(row_slots+field_slots));
  auto caller=std::make_unique<assembly::LetterProjection>(projection);
  std::optional<std::vector<OccurrenceBinding>> caller_rows(rows);
  std::optional<std::vector<ExchangeFieldBinding>> caller_fields(fields);
  // Comparator controls change every full-handle component and complete
  // semantic key independently; a record index alone is not an association.
  auto changed_rows=rows;
  for (int component=0; component<4; ++component) {
    changed_rows=rows;
    auto &h=changed_rows.front().handle;
    if (component==0) ++h.context;
    else if (component==1) ++h.incarnation;
    else if (component==2) ++h.address;
    else ++h.record;
    require(!map_cost_equal(changed_rows,rows),"map comparison ignored a native handle component");
  }
  changed_rows=rows; changed_rows.front().occurrence.study=warm::new_uuid();
  require(!map_cost_equal(changed_rows,rows),"map comparison ignored the study UUID");
  changed_rows=rows; ++changed_rows.front().occurrence.source.entry;
  require(!map_cost_equal(changed_rows,rows),"map comparison ignored the selected entry");
  changed_rows=rows; changed_rows.front().occurrence.source.path.push_back(19);
  require(!map_cost_equal(changed_rows,rows),"map comparison ignored the complete occurrence path");
  auto changed_fields=fields;
  changed_fields.front().field.kind=changed_fields.front().field.kind==assembly::GroupKind::Membership
      ? assembly::GroupKind::ParentValue : assembly::GroupKind::Membership;
  require(!map_cost_equal(changed_fields,fields),"map comparison ignored field kind");
  changed_fields=fields; changed_fields.front().field.object.scope=warm::new_uuid();
  require(!map_cost_equal(changed_fields,fields),"map comparison ignored field scope");
  changed_fields=fields;
  changed_fields.front().field.object.address=*codec::decode_token_id("Az.0z");
  require(!map_cost_equal(changed_fields,fields),"map comparison ignored the full field address");
  const auto entry=projection.recipe().selections.front().entry;
  const auto target=projection.exposure(entry)==warm::Exposure::Aggregate ? warm::Exposure::Parts : warm::Exposure::Aggregate;
  const assembly::OccurrenceKey key{projection.study(),{entry,{}}};
  const TickSnapshot before(f);
  std::array<std::uint64_t,3> requested{}, blocks{};
  A::Counts retained, released;
  std::uint64_t stage_work=0, binding_request=0;
  A::begin();
  try {
    auto owner=Access::adopt(f.h,*caller,*caller_rows,*caller_fields);
    retained=A::counts;
    binding_request=observed_shared_request(Plan::binding(Access::plan(*owner)));
    require(binding_request==sizeof(ExchangeBindings),"binding headers were not embedded in its actual object request");
    caller.reset(); caller_rows.reset(); caller_fields.reset();
    const auto *domain=Access::reservation_domain(*owner).get();
    const auto add=[&](std::size_t role,const void *data,std::size_t bytes,const auto *selected) {
      require(selected==domain,"map backing escaped selected owner domain");
      if (!bytes) return;
      require(buffers[role].size()<buffers[role].capacity(),"map observer scratch was not reserved");
      const auto index=A::bucket(const_cast<void *>(data));
      const A::Entry *found=nullptr;
      for (std::size_t j=0; j<65536; ++j) {
        const auto &candidate=A::table[(index+j)%65536];
        if (!candidate.used) break;
        if (candidate.pointer==data) { found=&candidate; break; }
      }
      require(found && found->bytes==bytes,"map buffer differs from independent actual request");
      for (const auto &family : buffers) for (const auto &other : family)
        require(other.data!=data,"map request aliased another slot or owner/binding/candidate family");
      buffers[role].push_back({data,bytes}); requested[role]+=bytes; ++blocks[role];
    };
    const auto inspect=[&](std::size_t role,const auto &map) {
      const auto &slots=map.storage();
      add(role,slots.data(),slots.capacity()*sizeof(slots[0]),slots.get_allocator().domain);
      for (const auto &row : slots) {
        const auto &id=map_cost_uuid(row);
        add(role,id.str().data(),warm::recipe_text_backing(id.str()),id.str().get_allocator().domain.get());
        const auto &nested=key_buffer(row);
        add(role,nested.data(),nested.capacity()*sizeof(nested[0]),nested.get_allocator().domain.get());
      }
    };
    no_new([&] {
      map_cost_pairs(*owner,[&](auto role,const auto &r,const auto &g) {
        require(r.storage().size()==row_slots && g.storage().size()==field_slots,
                "map reservation omitted constructed inactive slots");
        require(role==0 ? map_cost_equal(r,rows) && map_cost_equal(g,fields) : r.size()==0 && g.size()==0,
                "map adoption/source-death changed active inventory or made reserved slots live");
        inspect(role,r); inspect(role,g);
      });
    });
    require(requested[0] && requested[0]==requested[1] && requested[1]==requested[2] &&
                blocks[0]==blocks[1] && blocks[1]==blocks[2],
            "map families differ in reserved request footprint");
    no_new([&] {
      require(Access::resources(*owner).maps.charged_bytes()==requested[0]+requested[1]+requested[2],
              "map census differs from independent actual backing or counts embedded headers again");
    });
    {
      const auto selected=Access::declaration(*owner,key,target);
      const auto memberships=Access::memberships(*owner);
      no_new([&] {
        stage_work=Plan::stage_binding(Access::plan(*owner),selected,memberships,
            Access::mappings(*owner),Access::fields(*owner),target).charged;
        require(map_cost_equal(Plan::mappings(Access::plan(*owner)),Access::mappings(*owner)) &&
                    map_cost_equal(Plan::fields(Access::plan(*owner)),Access::fields(*owner)),
                "staging did not retain complete ordered logical/native associations");
        Plan::capture(Access::plan(*owner),Access::projection(*owner));
        require(Plan::binding_status(Access::plan(*owner))==UpdateStatus::Ok &&
                    Plan::planned_mappings(Access::plan(*owner)).size()==0 &&
                    Plan::planned_fields(Access::plan(*owner)).size()==0 && stage_work,
                "source capture changed candidate maps or did not validate committed source");
        map_cost_pairs(*owner,[&](auto role,const auto &r,const auto &g) {
          std::size_t i=0;
          const auto unchanged=[&](const auto &map) {
            const auto check=[&](const void *data,std::size_t bytes) {
              if (!bytes) return;
              require(i<buffers[role].size() && buffers[role][i].data==data && buffers[role][i].bytes==bytes,
                      "source capture replaced reserved map backing");
              ++i;
            };
            check(map.storage().data(),map.storage().capacity()*sizeof(map.storage()[0]));
            for (const auto &row : map.storage()) {
              const auto &id=map_cost_uuid(row);
              check(id.str().data(),warm::recipe_text_backing(id.str()));
              const auto &nested=key_buffer(row);
              check(nested.data(),nested.capacity()*sizeof(nested[0]));
            }
          };
          unchanged(r); unchanged(g);
          require(i==buffers[role].size(),"source capture lost map requests");
        });
      });
    }
    require(A::counts.attempts==retained.attempts && A::counts.live==retained.live,
            "map staging/capture changed C++ allocation lifetime");
    const auto bootstrap=Access::bootstrap_tracker(*owner).result().live_bytes;
    std::weak_ptr<ReservationDomain> weak=Access::reservation_domain(*owner);
    owner.reset();
    require(weak.expired() && A::counts.live==bootstrap && A::counts.blocks==2,
            "map owner teardown retained selected backing or domain");
    weak.reset();
    released=A::end();
  } catch (...) { if (A::active) A::end(); throw; }
  require(!released.live && !released.blocks && released.acquired==released.released && before==TickSnapshot(f),
          "map inspection leaked requests or changed native state");
  std::cout << "exchange_binding_map_cost,selections=" << projection.recipe().selections.size()
            << ",construction_calls=" << retained.attempts << ",owner_retained=" << retained.live
            << ",owner_peak=" << retained.peak << ",row_slots=" << row_slots << ",field_slots=" << field_slots
            << ",owner_map_blocks=" << blocks[0] << ",binding_map_blocks=" << blocks[1]
            << ",candidate_map_blocks=" << blocks[2] << ",owner_map_requested=" << requested[0]
            << ",binding_map_requested=" << requested[1] << ",candidate_map_requested=" << requested[2]
            << ",map_pair_headers=" << sizeof(ExchangeMap<OccurrenceBinding>)+sizeof(ExchangeMap<ExchangeFieldBinding>)
            << ",binding_object_bytes=" << sizeof(ExchangeBindings) << ",binding_object_request=" << binding_request
            << ",stage_work=" << stage_work
            << ",stage_capture_cpp_new=0,after_destroy_requested=" << released.live << " PASS\n";
}

// Value snapshots are test scratch, constructed outside execution/allocation
// guards. They preserve full keys/handles independently of the rotating stores.
struct MapRotationSnapshot {
  std::array<const void *,6> stores;
  assembly::detail::ExchangeMap<assembly::detail::OccurrenceBinding> source_rows;
  assembly::detail::ExchangeMap<assembly::detail::ExchangeFieldBinding> source_fields;
  std::uint64_t generation;
};
MapRotationSnapshot map_rotation_before(const assembly::detail::ExchangeOwner &owner) {
  using Access=assembly::testing::ExchangeOwnerAccess;
  using Plan=assembly::testing::ExchangeCoordinatorAccess;
  std::array<const void *,6> stores;
  no_new([&] {
    require(map_cost_equal(Access::mappings(owner),Plan::mappings(Access::plan(owner))) &&
                map_cost_equal(Access::fields(owner),Plan::fields(Access::plan(owner))),
            "prepared binding source differs from complete current owner maps");
    map_cost_pairs(owner,[&](auto role,const auto &r,const auto &g) {
      stores[2*role]=r.storage().data(); stores[2*role+1]=g.storage().data();
    });
    for (std::size_t i=0; i<stores.size(); ++i) for (std::size_t j=0; j<i; ++j)
      require(stores[i] && stores[i]!=stores[j],"publication map stores are not independent");
  });
  return {stores,Access::mappings(owner),Access::fields(owner),Access::generation(owner)};
}
void map_rotation_after(const assembly::detail::ExchangeOwner &owner,
    const field::detail::PreparedStorage &native, const MapRotationSnapshot &before) {
  using Access=assembly::testing::ExchangeOwnerAccess;
  using Plan=assembly::testing::ExchangeCoordinatorAccess;
  no_new([&] {
    map_cost_pairs(owner,[&](auto role,const auto &r,const auto &g) {
      const auto prior=role==0 ? 2 : (role==1 ? 0 : 1);
      require(r.storage().data()==before.stores[2*prior] && g.storage().data()==before.stores[2*prior+1],
              "publication did not rotate candidate to binding to owner and old current stores back");
      if (role) require(map_cost_equal(r,before.source_rows) && map_cost_equal(g,before.source_fields),
                        "retired binding/candidate maps lost exact previous source associations");
    });
    require(Access::generation(owner)==before.generation+1 && native.generation==Access::generation(owner) &&
                Plan::binding_status(Access::plan(owner))==UpdateStatus::Stale && !Access::pending(owner),
            "publication reused old source authority or separated native/owner generation");
    const auto claims=[&](const auto &map,const auto &headers) {
      for (const auto &row : map)
        require(row.handle.record>=0 && std::size_t(row.handle.record)<headers.size() &&
                    map_cost_handle_equal(row.handle,headers[row.handle.record]),
                "published semantic map lost the actual complete native claim");
    };
    claims(Access::mappings(owner),native.particle_headers);
    claims(Access::fields(owner),native.field_headers);
  });
}
