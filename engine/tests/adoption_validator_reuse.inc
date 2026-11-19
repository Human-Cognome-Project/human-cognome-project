// Inspection-only reuse of an independent binding, never the real owner's
// adoption validator. Reference storage is provisioned before allocation ROI.
namespace validator_reuse_observation {
using Bind=assembly::testing::ExchangeBindingAccess;
struct Snapshot {
  struct Edge { std::size_t row; field::detail::RelationshipHeader header; std::uint64_t count; };
  std::vector<std::uint64_t> masses;
  std::vector<engine::RecordHandle> outgoing;
  std::vector<Edge> edges;
  std::vector<unsigned char> outgoing_mask,edge_mask;
  std::size_t bodies{},handles{},facts{},body_bits{},edge_bits{};
  std::uint64_t generation{},work{};
  const assembly::LetterExchangeContract *declaration{};
  const assembly::MembershipFields *catalogue{};
  void reserve(Harness &h) {
    const auto &s=h.prepared_storage();
    const auto P=s.particle_headers.size(), E=s.capacity.limits()[Budget::Participation];
    masses.resize(P); outgoing.resize(P); edges.resize(E); outgoing_mask.resize(P); edge_mask.resize(E);
  }
  static bool same(engine::RecordHandle a,engine::RecordHandle b) {
    return a.context==b.context && a.incarnation==b.incarnation && a.address==b.address && a.record==b.record;
  }
  template<class Binding> void capture(const Binding &b) {
    bodies=Bind::masses(b).size(); handles=Bind::outgoing_view(b).size(); facts=Bind::edges(b).size();
    body_bits=Bind::outgoing_mask(b).size(); edge_bits=Bind::edge_mask(b).size();
    require(bodies<=masses.size() && handles<=outgoing.size() && facts<=edges.size() &&
            body_bits<=outgoing_mask.size() && edge_bits<=edge_mask.size(),"snapshot capacity exhausted");
    for (std::size_t i=0;i<bodies;++i) masses[i]=Bind::masses(b)[i];
    for (std::size_t i=0;i<handles;++i) outgoing[i]=Bind::outgoing_view(b)[i];
    for (std::size_t i=0;i<facts;++i) {
      const auto &e=Bind::edges(b)[i]; edges[i]={e.row,e.header,e.count};
    }
    for (std::size_t i=0;i<body_bits;++i) outgoing_mask[i]=Bind::outgoing_mask(b)[i];
    for (std::size_t i=0;i<edge_bits;++i) edge_mask[i]=Bind::edge_mask(b)[i];
    generation=Bind::generation(b); work=Bind::work(b);
    declaration=Bind::declaration(b).get(); catalogue=Bind::catalogue(b).get();
  }
  template<class Binding> void compare(const Binding &b) const {
    require(bodies==Bind::masses(b).size() && handles==Bind::outgoing_view(b).size() && facts==Bind::edges(b).size() &&
            body_bits==Bind::outgoing_mask(b).size() && edge_bits==Bind::edge_mask(b).size() &&
            generation==Bind::generation(b) && work==Bind::work(b) && declaration==Bind::declaration(b).get() &&
            catalogue==Bind::catalogue(b).get(),"reused binding retained another capture identity/shape");
    for (std::size_t i=0;i<bodies;++i) require(masses[i]==Bind::masses(b)[i],"reused exact mass differs");
    for (std::size_t i=0;i<handles;++i) require(same(outgoing[i],Bind::outgoing_view(b)[i]),"reused full outgoing handle differs");
    for (std::size_t i=0;i<facts;++i) {
      const auto &a=edges[i]; const auto &e=Bind::edges(b)[i];
      require(a.row==e.row && same(a.header.first,e.header.first) && same(a.header.second,e.header.second) &&
              a.header.ordinal==e.header.ordinal && !std::memcmp(&a.header.share,&e.header.share,sizeof(float)) && a.count==e.count,
              "reused captured source-row/endpoint/ordinal/share/count differs");
    }
    for (std::size_t i=0;i<body_bits;++i) require(outgoing_mask[i]==Bind::outgoing_mask(b)[i],"reused outgoing mask kept stale bits");
    for (std::size_t i=0;i<edge_bits;++i) require(edge_mask[i]==Bind::edge_mask(b)[i],"reused edge mask kept stale bits");
  }
};
}
