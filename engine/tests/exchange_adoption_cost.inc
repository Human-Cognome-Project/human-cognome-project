// Test-only observation of real adoption phases, followed by the exact stack
// constructor with the same pinned inputs. No production callback or copy path.
namespace adoption_binding_observation {
namespace A=allocation_lifetime;
using Phase=assembly::detail::ProjectionSetupPhase;
struct Stats { std::size_t requests{},bytes{},live{},peak{},whole_peak{}; };
struct Event { std::size_t selection{},bytes{}; bool acquire{}; };
struct Trace {
  std::array<std::size_t,65536> tags{};
  std::array<Event,32768> events{};
  std::array<Stats,16> selections{};
  assembly::detail::ProjectionSetupResult *setup{};
  Phase last{Phase::Admission};
  std::size_t count{},current{},used{},bytes{};
  bool spare{},spare_with_temporary{};
  void reset(assembly::detail::ProjectionSetupResult *s=nullptr) {
    tags.fill(0); selections.fill({}); setup=s; last=Phase::Admission;
    count=s?0:1; current=s?0:1; used=bytes=0; spare=spare_with_temporary=false;
  }
  void phase() noexcept {
    if (!setup) return;
    const auto now=setup->phase;
    if (now>=Phase::SpareInspection) {
      spare=true;
      for (std::size_t id=1;id<=count;++id) spare_with_temporary |= selections[id].live!=0;
    }
    if (now!=last) {
      current=0;
      if (now==Phase::Bindings && !spare) {
        if (++count>=selections.size() || selections[count-1].live) std::abort();
        current=count;
      }
      last=now;
    }
  }
  std::size_t slot(void *p) const noexcept {
    for (std::size_t j=0,i=A::bucket(p); j<65536; ++j,i=(i+1)%65536)
      if (A::table[i].pointer==p) return i;
    std::abort();
  }
  void event(std::size_t id,std::size_t n,bool acquire) noexcept {
    if (!id) return;
    if (used==events.size()) std::abort();
    events[used++]={id,n,acquire};
    auto &s=selections[id];
    if (acquire) { ++s.requests; s.bytes+=n; s.live+=n; s.peak=std::max(s.peak,s.live); }
    else { if (n>s.live) std::abort(); s.live-=n; }
  }
  void overlap() noexcept {
    for (std::size_t id=1; id<=count; ++id)
      if (selections[id].live) selections[id].whole_peak=std::max(selections[id].whole_peak,A::counts.live);
  }
  void acquired(void *p,std::size_t n) noexcept {
    phase(); bytes+=n; const auto index=slot(p); tags[index]=current;
    event(current,n,true); overlap();
  }
  void released(void *p,std::size_t n) noexcept {
    phase(); overlap(); const auto index=slot(p);
    event(tags[index],n,false); tags[index]=0;
  }
};
inline thread_local Trace *active{};
void acquired(void *p,std::size_t n) noexcept { active->acquired(p,n); }
void released(void *p,std::size_t n) noexcept { active->released(p,n); }
struct Observe {
  explicit Observe(Trace &t) {
    if (active || A::on_acquired || A::on_released) std::abort();
    active=&t; A::on_acquired=acquired; A::on_released=released;
  }
  ~Observe() { A::on_acquired=A::on_released=nullptr; active=nullptr; }
};
struct Select {
  Trace *prior;
  explicit Select(Trace &t):prior(active) { if (!prior) std::abort(); active=&t; }
  ~Select() { active=prior; }
};
template<class Map,class Visit> void map_requests(const Map &m,const assembly::detail::ReservationDomain *domain,Visit visit) {
  require(m.storage().get_allocator().domain==domain,"temporary outer map escaped domain");
  visit(m.storage().data(),m.storage().capacity()*sizeof(m.storage()[0]));
  for (const auto &row:m.storage()) {
    const auto &text=map_cost_uuid(row).str();
    require(text.get_allocator().domain.get()==domain,"temporary UUID escaped domain");
    visit(text.data(),warm::recipe_text_backing(text));
    const auto &key=assembly::detail::key_buffer(row);
    require(key.get_allocator().domain.get()==domain,"temporary nested key escaped domain");
    visit(key.data(),key.capacity()*sizeof(key[0]));
  }
}
bool same_events(const Trace &owner,std::size_t id,const Trace &single) {
  std::size_t j=0;
  for (std::size_t i=0; i<owner.used; ++i) if (owner.events[i].selection==id) {
    if (j==single.used || owner.events[i].bytes!=single.events[j].bytes ||
        owner.events[i].acquire!=single.events[j].acquire) return false;
    ++j;
  }
  return j==single.used;
}
}

template<class Declarations,class Rows,class Fields,class References>
std::uint64_t adoption_validator_reuse(Fixture &f,const assembly::LetterProjection &projection,
    const Declarations &declarations,std::shared_ptr<const assembly::MembershipFields> memberships,
    const Rows &rows,const Fields &fields,const References &reference,
    adoption_binding_observation::Trace &trace,std::uint64_t fresh_work,
    std::size_t fresh_requests,std::size_t fresh_bytes) {
  using Bind=assembly::testing::ExchangeBindingAccess;
  using namespace assembly::detail;
  namespace O=adoption_binding_observation;
  namespace A=allocation_lifetime;
  const auto prior=A::counts;
  auto domain=std::make_shared<ReservationDomain>(ReservationBudget{UINT64_MAX,UINT64_MAX});
  std::weak_ptr<ReservationDomain> weak=domain;
  trace.reset();
  std::array<std::uint64_t,16> capture_work{};
  std::uint64_t base_work=0,one_pass=0,two_passes=0;
  std::size_t retries=0;
  {
    O::Select select(trace);
    Bind::scratch(f.h,domain,UINT64_MAX,[&](auto &binding,auto &meter) {
      base_work=meter.result().charged;
      const auto selected=domain->result();
      const auto compare=[&](std::size_t i) {
        reference[i].compare(binding);
        require(Bind::validate_adoption(binding,rows,fields).status==field::UpdateStatus::Ok &&
                Bind::mappings(binding).storage().empty() && Bind::fields(binding).storage().empty() &&
                Bind::validate(binding).status==field::UpdateStatus::InvalidInput,
                "scratch replay lost actual source validation or retained map ownership");
      };
      for (int pass=0;pass<2;++pass) {
        for (std::size_t step=0;step<declarations.size();++step) {
          const auto i=pass ? declarations.size()-1-step : step;
          const auto before=A::counts; const auto work_before=meter.result().charged;
          no_new([&] {
            Bind::recapture(binding,projection,declarations[i],declarations[i]->to(),memberships,rows,fields,meter);
            compare(i);
          });
          const auto charged=meter.result().charged-work_before;
          if (!pass) capture_work[i]=charged;
          else require(capture_work[i]==charged,"reverse selection order changed complete capture work");
          require(A::counts.attempts==before.attempts && A::counts.released==before.released &&
                  domain->result().live_bytes==selected.live_bytes &&
                  domain->result().charged_work==selected.charged_work,
                  "recapture resized, replaced or released a reserved capture buffer");
        }
        (pass ? two_passes : one_pass)=meter.result().charged;
      }
      // Error diagnostics may allocate exception text. They are outside the
      // successful replay ROI; the binding's five allocations stay tagged.
      trace.current=0;
      for (std::size_t i=0;i<declarations.size();++i) {
        for (const auto allowance : {UINT64_C(0),capture_work[i]-1}) {
          PreparationWork short_work(allowance);
          bool refused=false;
          try { Bind::recapture(binding,projection,declarations[i],declarations[i]->to(),memberships,rows,fields,short_work); }
          catch (const PreparationStop &stop) { refused=stop.result.status==PreparationStatus::WorkExhausted; }
          require(refused && Bind::validate_adoption(binding,rows,fields).status==field::UpdateStatus::InvalidInput,
                  "short recapture advertised previous selection as valid");
          PreparationWork exact(capture_work[i]);
          no_new([&] {
            Bind::recapture(binding,projection,declarations[i],declarations[i]->to(),memberships,rows,fields,exact);
            compare(i);
          });
          require(exact.result().charged==capture_work[i],"exact recapture retry changed work");
          ++retries;
        }
        PreparationWork wrong_role(UINT64_MAX);
        bool refused=false;
        try { Bind::recapture(binding,projection,declarations[i],declarations[i]->from(),memberships,rows,fields,wrong_role); }
        catch (const warm::Error &) { refused=true; }
        require(refused && Bind::validate_adoption(binding,rows,fields).status==field::UpdateStatus::InvalidInput,
                "wrong-role recapture retained a valid prior selection");
        PreparationWork exact(capture_work[i]);
        no_new([&] {
          Bind::recapture(binding,projection,declarations[i],declarations[i]->to(),memberships,rows,fields,exact);
          compare(i);
        });
        ++retries;
      }
      const auto &limits=f.h.prepared_storage().capacity.limits();
      const auto row_bound=exchange_map_capacity(limits[Budget::Particles],limits[Budget::Batch]);
      const auto field_bound=exchange_map_capacity(limits[Budget::Fields],std::min(limits[Budget::Fields],limits[Budget::Batch]));
      const ExchangeMap<OccurrenceBinding> too_many_rows(std::vector<OccurrenceBinding>(row_bound+1));
      const ExchangeMap<ExchangeFieldBinding> too_many_fields(std::vector<ExchangeFieldBinding>(field_bound+1));
      for (bool bad_rows : {true,false}) {
        require(Bind::validate_adoption(binding,rows,fields).status==field::UpdateStatus::Ok,
                "inventory refusal control did not start with a valid capture");
        PreparationWork rejected_work(UINT64_MAX);
        bool refused=false;
        try {
          Bind::recapture(binding,projection,declarations[0],declarations[0]->to(),memberships,
              bad_rows ? too_many_rows : rows,bad_rows ? fields : too_many_fields,rejected_work);
        } catch (const warm::Error &error) {
          refused=std::strcmp(error.what(),"exchange adoption inventory exceeds map bounds")==0;
        }
        require(refused && !rejected_work.result().charged &&
                Bind::validate_adoption(binding,rows,fields).status==field::UpdateStatus::InvalidInput,
                "pre-stage inventory refusal advertised the previous valid capture");
        PreparationWork exact(capture_work[0]);
        no_new([&] {
          Bind::recapture(binding,projection,declarations[0],declarations[0]->to(),memberships,rows,fields,exact);
          compare(0);
        });
        ++retries;
      }
      trace.current=1;
    });
  }
  const auto &scratch=trace.selections[1];
  require(scratch.requests==5 && !scratch.live && scratch.bytes==scratch.peak &&
          scratch.bytes==domain->result().charged_work && !domain->result().live_bytes &&
          fresh_requests==5*declarations.size() && fresh_bytes==declarations.size()*scratch.bytes &&
          fresh_work==one_pass+(declarations.size()-1)*base_work && two_passes==2*one_pass-base_work,
          "reused capture cost differs from real per-selection constructors");
  domain.reset();
  require(weak.expired(),"scratch validator retained its reservation domain");
  weak.reset();
  require(A::counts.live==prior.live && A::counts.blocks==prior.blocks,
          "scratch validator retained captured source or buffers");
  std::cout << "adoption_validator_recapture,selections=" << declarations.size()
    << ",fresh_reference_requests=" << fresh_requests << ",fresh_reference_requested=" << fresh_bytes
    << ",scratch_requests=" << scratch.requests << ",scratch_requested=" << scratch.bytes << ",scratch_peak=" << scratch.peak
    << ",fresh_reference_extra_requests=" << fresh_requests-scratch.requests << ",fresh_reference_extra_requested=" << fresh_bytes-scratch.bytes
    << ",base_work=" << base_work << ",fresh_work=" << fresh_work << ",scratch_one_pass_work=" << one_pass
    << ",scratch_two_pass_work=" << two_passes << ",refusal_retries=" << retries
    << ",inventory_refusals=2,full_facts_equal=1,recapture_new=0 PASS\n";
  return one_pass;
}

void exchange_adoption_binding_cost(Fixture &f,const assembly::LetterProjection &projection,
    const std::vector<assembly::detail::OccurrenceBinding> &rows,
    const std::vector<assembly::detail::ExchangeFieldBinding> &fields) {
  using Owner=assembly::testing::ExchangeOwnerAccess;
  using Bind=assembly::testing::ExchangeBindingAccess;
  using namespace assembly::detail;
  namespace O=adoption_binding_observation;
  namespace A=allocation_lifetime;
  auto real=std::make_unique<O::Trace>(), single=std::make_unique<O::Trace>(), reused=std::make_unique<O::Trace>();
  auto reference=std::make_unique<std::array<validator_reuse_observation::Snapshot,16>>();
  for (auto &snapshot:*reference) snapshot.reserve(f.h);
  ProjectionSetupResult setup;
  real->reset(&setup);
  const TickSnapshot before(f);
  struct Measurement {
    O::Stats actual;
    A::Counts alive;
    std::size_t map_bytes{},map_blocks{},facts{},bodies{},outgoing{},edges{};
    std::uint64_t work{},validation{};
    warm::Exposure source{};
  };
  std::array<Measurement,16> measurements{};
  A::Counts whole,finished;
  ReservationResult selected;
  std::uint64_t retained_map_bytes=0,candidate_requested=0,candidate_blocks=0;
  std::uint64_t temporary_bytes=0,temporary_requests=0,temporary_work=0,owner_requested=0;
  std::uint64_t fresh_bytes=0,fresh_requests=0,fresh_work=0;
  std::size_t selection_count=0;
  std::weak_ptr<ReservationDomain> weak;
  exchange_parent_observation::Cost parent_cost;
  membership_study_observation::Cost membership_study_cost;
  membership_target_observation::Cost membership_target_cost;
  std::array<projection_definition_observation::Cost,2> projection_definition_cost;
  std::array<projection_study_observation::Cost,2> projection_study_cost;
  {
    O::Observe observe(*real);
    A::begin();
    try {
      {
        auto owner=Owner::adopt(f.h,projection,rows,fields,UINT64_MAX,setup);
        require(owner && setup.phase==ProjectionSetupPhase::Complete,"actual adoption did not complete");
        whole=A::counts;
        owner_requested=real->bytes;
        selected=Owner::reservations(*owner);
        weak=Owner::reservation_domain(*owner);
        selection_count=projection.recipe().selections.size();
        require(real->count==1 && !real->spare_with_temporary && real->selections[1].requests==5 &&
                    !real->selections[1].live,"shared adoption buffers survived into spare/coordinator preparation");
        temporary_bytes=real->selections[1].bytes;
        temporary_requests=real->selections[1].requests;
        require(selected.live_bytes && selected.charged_work<=real->bytes,
                "selected requests exceed whole-owner observation");
        const auto declarations=Owner::declarations(*owner);
        const auto memberships=Owner::memberships(*owner);
        const auto &source=Owner::projection(*owner);
        no_new([&] {
          const auto &spare=Owner::spare_projection(*owner);
          projection_definition_observation::independent(source,spare);
          projection_study_observation::independent(source,spare);
          std::size_t view=0;
          for(const auto *p:{&source,&spare})
            projection_definition_cost[view++]=projection_definition_observation::inspect(*p,
                Owner::reservation_domain(*owner).get(),[&](const void *pointer,std::size_t bytes) {
                  require(A::table[real->slot(const_cast<void *>(pointer))].bytes==bytes,
                          "projected definition pin differs from its actual owner allocation");
                });
          view=0;
          for(const auto *p:{&source,&spare})
            projection_study_cost[view++]=projection_study_observation::inspect(*p,
                Owner::reservation_domain(*owner).get(),[&](const void *pointer,std::size_t bytes) {
                  require(A::table[real->slot(const_cast<void *>(pointer))].bytes==bytes,
                          "projection study attribution differs from its actual owner allocation");
                });
          for (const auto &declaration:declarations)
            parent_cost.add(exchange_parent_observation::inspect_pinned<Owner>(source,*declaration));
          membership_study_cost=membership_study_observation::inspect(*memberships,source,Owner::catalogue_rows(*memberships),
              Owner::reservation_domain(*owner).get(),[&](const void *pointer,std::size_t bytes) {
                require(A::table[real->slot(const_cast<void *>(pointer))].bytes==bytes,
                        "catalogue UUID attribution differs from an actual owner request");
              });
          membership_target_cost=membership_target_observation::inspect(*memberships,
              assembly::testing::MembershipFieldsAccess::recipe(*memberships),source,Owner::catalogue_rows(*memberships),
              Owner::reservation_domain(*owner).get(),[&](const void *pointer,std::size_t bytes) {
                require(A::table[real->slot(const_cast<void *>(pointer))].bytes==bytes,
                        "catalogue target pin attribution differs from an actual owner request");
              });
        });
        retained_map_bytes=Owner::resources(*owner).maps.charged_bytes();
        map_cost_pairs(*owner,[&](auto role,const auto &r,const auto &g) {
          if (role!=2) return;
          const auto add=[&](const void *p,std::size_t n) {
            if (!n) return;
            require(A::table[real->slot(const_cast<void *>(p))].bytes==n,
                    "candidate visitor differs from real request");
            candidate_requested+=n; ++candidate_blocks;
          };
          const auto *domain=Owner::reservation_domain(*owner).get();
          O::map_requests(r,domain,add); O::map_requests(g,domain,add);
        });
        const auto candidate_backing=Owner::map_backing(*owner);
        for (std::size_t i=0; i<declarations.size(); ++i) {
          const auto &declaration=declarations[i];
          auto domain=std::make_shared<ReservationDomain>(ReservationBudget{UINT64_MAX,UINT64_MAX});
          single->reset();
          A::Counts alive;
          const auto baseline=A::counts;
          std::size_t map_bytes=0,map_blocks=0,fact_bytes=0;
          std::size_t bodies=0,outgoing=0,edges=0;
          std::uint64_t work=0,validation=0;
          {
            O::Select select(*single);
            const auto result=Bind::adoption(f.h,source,declaration,memberships,
              Owner::mappings(*owner),Owner::fields(*owner),domain,UINT64_MAX,
              [&](const auto &b) { no_new([&] {
                alive=A::counts;
                alive.live-=baseline.live; alive.blocks-=baseline.blocks;
                alive.peak=single->selections[1].peak;
                require(Bind::validate_adoption(b,Owner::mappings(*owner),Owner::fields(*owner)).status==field::UpdateStatus::Ok &&
                            Bind::generation(b)==f.h.prepared_storage().generation &&
                            map_cost_equal(Owner::mappings(*owner),rows) && map_cost_equal(Owner::fields(*owner),fields) &&
                            Bind::mappings(b).storage().empty() && Bind::fields(b).storage().empty() &&
                            Bind::validate(b).status==field::UpdateStatus::InvalidInput,
                        "temporary constructor changed source associations");
                const auto add=[&](const void *p,std::size_t n) {
                  if (!n) return;
                  const auto index=single->slot(const_cast<void *>(p));
                  require(A::table[index].bytes==n,"temporary map visitor differs from actual request");
                  map_bytes+=n; ++map_blocks;
                };
                O::map_requests(Bind::mappings(b),domain.get(),add);
                O::map_requests(Bind::fields(b),domain.get(),add);
                const auto &native=f.h.prepared_storage();
                const auto same=[](const auto &a,const auto &c) {
                  return a.context==c.context && a.record==c.record && a.address==c.address && a.incarnation==c.incarnation;
                };
                const auto &masses=Bind::masses(b);
                const auto &captured=Bind::edges(b);
                const auto &handles=Bind::outgoing_view(b);
                bodies=masses.size(); outgoing=handles.size(); edges=captured.size();
                require(bodies==rows.size() && outgoing==declaration->outgoing().size(),"temporary lost inventory or outgoing role");
                for (std::size_t j=0; j<rows.size(); ++j) {
                  const auto h=rows[j].handle;
                  require(same(h,native.particle_headers[h.record]) && masses[j]==native.source_masses[h.record],
                          "temporary captured mass or full source handle differs");
                }
                for (std::size_t j=0; j<handles.size(); ++j) {
                  const auto row=std::find_if(rows.begin(),rows.end(),[&](const auto &r) {return r.occurrence==declaration->outgoing()[j].occurrence;});
                  require(row!=rows.end() && same(handles[j],row->handle) && Bind::outgoing_mask(b)[row->handle.record],
                          "temporary outgoing source selection differs");
                }
                for (const auto &e:captured) {
                  const auto &live=native.edges[e.row];
                  require(same(e.header.first,live.first) && same(e.header.second,live.second) &&
                              e.header.ordinal==live.ordinal && e.header.share==live.share &&
                              e.count==native.source_counts[e.row] && Bind::edge_mask(b)[e.row],
                          "temporary captured ledger fact differs");
                }
                require(Bind::outgoing_mask(b).size()==native.particle_headers.size() &&
                            Bind::edge_mask(b).size()==native.live.edges,"temporary masks lost source spans");
                for (std::size_t row=0; row<native.particle_headers.size(); ++row) {
                  const auto present=std::any_of(handles.begin(),handles.end(),[&](const auto &h) {return std::size_t(h.record)==row;});
                  require(Bind::outgoing_mask(b)[row]==present,"temporary outgoing mask changed another occurrence");
                }
                for (std::size_t row=0; row<native.live.edges; ++row) {
                  const auto count=std::count_if(captured.begin(),captured.end(),[&](const auto &e) {return e.row==row;});
                  const auto incident=Bind::outgoing_mask(b)[native.edges[row].first.record];
                  require(count<=1 && bool(count)==incident && Bind::edge_mask(b)[row]==incident,
                          "temporary captured ledger omitted or duplicated outgoing participation");
                }
                validation=Bind::work(b);
                (*reference)[i].capture(b);
                require(validation==16*(rows.size()+fields.size()+edges+native.live.edges+1),
                        "temporary validation work omitted live inventory");
                require(alive.live==domain->result().live_bytes && alive.live>=map_bytes &&
                            alive.blocks==map_blocks+5,
                        "temporary retained backing is not maps plus five capture buffers");
                fact_bytes=alive.live-map_bytes;
              }); });
            work=result.charged;
          }
          const auto &actual=real->selections[1], &isolated=single->selections[1];
          require(A::counts.live==baseline.live && A::counts.blocks==baseline.blocks && !domain->result().live_bytes &&
                      A::counts.attempts-baseline.attempts==actual.requests &&
                      isolated.bytes==actual.bytes && isolated.peak==actual.peak &&
                      O::same_events(*real,1,*single),
                  "fresh reference differs from actual shared buffer acquisition/release trace");
          fresh_bytes+=isolated.bytes; fresh_requests+=isolated.requests; fresh_work+=work;
          measurements[i]={isolated,alive,map_bytes,map_blocks,fact_bytes,bodies,outgoing,edges,work,validation,declaration->from()};
          require(Owner::resources(*owner).maps.charged_bytes()==retained_map_bytes &&
                      Owner::map_backing(*owner)==candidate_backing,
                  "temporary validation inspection changed retained current/candidate backing");
          // The private constructor uses the shared setup allowance, unwinds
          // partial capture, and can retry against the same unmodified source.
          for (const auto allowance : {UINT64_C(0),work-1}) {
            bool refused=false;
            try { Bind::adoption(f.h,source,declaration,memberships,Owner::mappings(*owner),
                Owner::fields(*owner),domain,allowance,[](const auto &) {}); }
            catch (const PreparationStop &stop) { refused=stop.result.status==PreparationStatus::WorkExhausted; }
            require(refused && !domain->result().live_bytes,"short adoption capture leaked or exceeded setup work");
          }
          const auto retried=Bind::adoption(f.h,source,declaration,memberships,Owner::mappings(*owner),
              Owner::fields(*owner),domain,work,[&](const auto &b) {
                require(Bind::validate_adoption(b,Owner::mappings(*owner),Owner::fields(*owner)).status==field::UpdateStatus::Ok,
                        "exact adoption capture retry lost current source");
              });
          require(retried.charged==work && !domain->result().live_bytes,"exact adoption capture work changed");
          if (i==0) {
            const auto &limits=f.h.prepared_storage().capacity.limits();
            const auto bound=exchange_map_capacity(limits[field::Budget::Fields],
                std::min(limits[field::Budget::Fields],limits[field::Budget::Batch]));
            const std::vector<ExchangeFieldBinding> oversized(bound+1,fields.front());
            const ExchangeMap<ExchangeFieldBinding> too_many(oversized);
            bool refused=false;
            try { Bind::adoption(f.h,source,declaration,memberships,Owner::mappings(*owner),too_many,
                domain,UINT64_MAX,[](const auto &) {}); }
            catch (const warm::Error &error) {
              refused=std::strcmp(error.what(),"exchange adoption inventory exceeds map bounds")==0;
            }
            require(refused && !domain->result().live_bytes,"adoption lost independent field reservation bound");
          }
        }
        temporary_work=adoption_validator_reuse(f,source,declarations,memberships,Owner::mappings(*owner),Owner::fields(*owner),
            *reference,*reused,fresh_work,fresh_requests,fresh_bytes);
        require(Owner::map_backing(*owner)==candidate_backing &&
                map_cost_equal(Owner::mappings(*owner),rows) && map_cost_equal(Owner::fields(*owner),fields),
                "scratch replay changed actual current/candidate owner maps");
      }
      require(weak.expired(),"temporary inspection retained the owner domain");
      weak.reset();
      finished=A::end();
    } catch (...) { if (A::active) A::end(); throw; }
  }
  require(!finished.live && !finished.blocks && finished.acquired==finished.released,
          "adoption inspection leaked owner, alias or constructor requests");
  require(temporary_bytes<=owner_requested-whole.live && temporary_work<=setup.work.charged,
          "temporary costs exceed whole-owner construction or setup work");
  for (std::size_t i=0; i<selection_count; ++i) {
    const auto &m=measurements[i];
    std::cout << "exchange_adoption_fresh_reference_cost,selections=" << selection_count << ",selection=" << i
              << ",source=" << (m.source==warm::Exposure::Aggregate?"aggregate":"parts")
              << ",requests=" << m.actual.requests << ",requested=" << m.actual.bytes << ",peak=" << m.actual.peak
              << ",constructor_live=" << m.alive.live
              << ",owned_map_requests=" << m.map_blocks << ",owned_map_bytes=" << m.map_bytes
              << ",capture_buffer_bytes=" << m.facts << ",constructor_work=" << m.work
              << ",released_before_constructor_return=" << m.actual.bytes-m.alive.live
              << ",validation_work=" << m.validation << ",bodies=" << m.bodies << ",outgoing=" << m.outgoing
              << ",captured_edges=" << m.edges << ",stack_object_bytes=" << sizeof(ExchangeBindings)
              << ",scratch_map_headers=0,map_argument_pointer_bytes=" << 2*sizeof(void *)
              << ",trace_equal=1,after_destroy_requested=0 PASS\n";
  }
  require(before==TickSnapshot(f),"adoption binding inspection changed native execution");
  for(std::size_t view=0;view<projection_definition_cost.size();++view) {
    const auto &cost=projection_definition_cost[view];
    std::cout << "projection_definition_owner,selections=" << selection_count << ",view=" << view
              << ",rows=" << cost.rows << ",active=" << cost.active << ",unique_definitions=" << cost.unique_definitions
              << ",requests=" << cost.requests << ",requested=" << cost.bytes
              << ",pin_capacity_bytes=" << cost.pin_capacity_bytes << ",index_capacity_bytes=" << cost.index_capacity_bytes
              << ",outer_bytes=" << cost.outer_bytes
              << ",lookup_work=" << cost.lookup_work
              << ",lookup_allocations=0,recipe_shared=1,recipe_definition_shared=1,after_destroy_requested=0 PASS\n";
  }
  for(std::size_t view=0;view<projection_study_cost.size();++view) {
    const auto &cost=projection_study_cost[view];
    std::cout << "projection_study_owner,selections=" << selection_count << ",view=" << view << ",rows=" << cost.rows
              << ",capacity=" << cost.capacity << ",row_uuid_requests=" << cost.requests << ",row_uuid_requested=" << cost.bytes
              << ",header_uuid_requested=" << cost.header_bytes << ",uuid_member_capacity_bytes=" << cost.uuid_capacity_bytes
              << ",outer_requested=" << cost.outer_bytes << ",byte_owner_uuid_requests=" << cost.byte_owner_requests
              << ",byte_owner_uuid_requested=" << cost.byte_owner_bytes << ",header_comparison_work=" << cost.comparison_work
              << ",source_validation_work=" << cost.source_work
              << ",comparison_allocations=0,independent_views=1,after_destroy_requested=0,runtime_saving=0 PASS\n";
  }
  std::cout << "membership_study_owner,selections=" << selection_count << ",rows=" << membership_study_cost.rows
            << ",capacity=" << membership_study_cost.capacity << ",row_uuid_requests=" << membership_study_cost.requests
            << ",row_uuid_requested=" << membership_study_cost.bytes << ",header_uuid_requested=" << membership_study_cost.header_bytes
            << ",uuid_member_capacity_bytes=" << membership_study_cost.uuid_member_bytes
            << ",outer_requested=" << membership_study_cost.outer_bytes
            << ",header_comparison_work=" << membership_study_cost.comparison_work
            << ",comparison_allocations=0,duplicate_row_study_backing=0,after_destroy_requested=0 PASS\n";
  std::cout << "membership_target_owner,selections=" << selection_count << ",rows=" << membership_target_cost.rows
            << ",unique_targets=" << membership_target_cost.unique_targets << ",requests=" << membership_target_cost.requests
            << ",requested=" << membership_target_cost.bytes << ",unique_requests=" << membership_target_cost.unique_requests
            << ",unique_requested=" << membership_target_cost.unique_bytes
            << ",pin_capacity_bytes=" << membership_target_cost.pin_capacity_bytes
            << ",table_capacity=" << membership_target_cost.table_capacity
            << ",index_capacity_bytes=" << membership_target_cost.index_capacity_bytes
            << ",lookup_work=" << membership_target_cost.lookup_work
            << ",lookup_allocations=0,after_destroy_requested=0,target_table_owned=1 PASS\n";
  std::cout << "exchange_parent_owner,selections=" << selection_count
            << ",sources=" << parent_cost.sources << ",parents=" << parent_cost.parents
            << ",parent_vector_requested=" << parent_cost.vector_bytes
            << ",parent_pin_requested=" << parent_cost.nested_bytes
            << ",parent_requests=" << parent_cost.requests << ",parent_requested=" << parent_cost.bytes()
            << ",repeated_sources=" << parent_cost.repeated_sources << ",lookup_work=" << parent_cost.lookup_work
            << ",lookup_allocations=0,private_parent_backing=0,after_destroy_requested=0 PASS\n";
  std::cout << "exchange_adoption_binding_total,selections=" << selection_count
            << ",temporary_requests=" << temporary_requests << ",temporary_requested=" << temporary_bytes
            << ",temporary_work=" << temporary_work << ",owner_setup_work=" << setup.work.charged
            << ",owner_requests=" << whole.attempts << ",owner_requested=" << owner_requested
            << ",owner_retained=" << whole.live << ",owner_peak=" << whole.peak
            << ",domain_peak=" << selected.peak_bytes << ",current_candidate_map_census=" << retained_map_bytes
            << ",candidate_map_requested=" << candidate_requested << ",candidate_map_requests=" << candidate_blocks
            << ",other_setup_released=" << owner_requested-whole.live-temporary_bytes
            << ",temporary_whole_overlap_peak=" << real->selections[1].whole_peak
            << ",released_before_spare=1,candidate_unchanged=1,native_unchanged=1 PASS\n";
}
