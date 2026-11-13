// Bounded packet diagnostic experiment. No production force, brake or activity
// state is modified. Input rows are caller-supplied, not a graph enumeration.
#include "engine/runtime.h"
#include "engine/transfer.h"
#include "field/numeric_ir.h"
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

using namespace taichi::lang;
namespace {
constexpr int columns = 17, packets = 2, max_cases = 64;
using Row = std::array<double, columns>;
void require(bool x, const char *why) { if (!x) throw std::runtime_error(why); }
void near(double a, double b) {
  require(std::isfinite(a) && std::isfinite(b) &&
              std::abs(a-b) <= 1e-12 * (1+std::abs(b)), "native reference mismatch");
}

engine::CompiledKernel build(engine::Runtime &runtime, int n) {
  IRBuilder b;
  auto *in = b.create_ndarray_arg_load({0}, PrimitiveType::f64, 2, 0);
  auto *out = b.create_ndarray_arg_load({1}, PrimitiveType::f64, 2, 0);
  auto *loop = b.create_range_for(b.get_int32(0), b.get_int32(n), false, runtime.cpu_threads());
  {
    auto scope = b.get_loop_guard(loop);
    auto *row = b.get_loop_index(loop);
    const auto load = [&](int col) -> Stmt * {
      return b.create_global_load(b.create_external_ptr(in, {b.get_int32(col), row}));
    };
    const auto var = [&](DataType type, Stmt *initial) {
      auto *v = b.create_local_var(type); b.create_local_store(v, initial); return v;
    };
    auto *bad = var(PrimitiveType::i32, b.get_int32(0));
    auto *contact_bad = var(PrimitiveType::i32, b.get_int32(0));
    auto *coincident = var(PrimitiveType::i32, b.get_int32(0));
    auto *weight = var(PrimitiveType::f64, b.get_float64(0));
    std::array<AllocaStmt *, 3> force{}, contact{};
    for (int k=0;k<3;++k) {
      force[k]=var(PrimitiveType::f64,b.get_float64(0));
      contact[k]=var(PrimitiveType::f64,b.get_float64(0));
    }
    const auto fail = [&](AllocaStmt *flag, Stmt *condition) {
      b.create_local_store(flag, b.create_logical_or(b.create_local_load(flag), condition));
    };
    // Native normal-or-zero contract, including bit-level input subnormals.
    const auto checked = [&](Stmt *v, AllocaStmt *flag) -> Stmt * {
      auto *ok=field::detail::supported_real(b,v);
      fail(flag,b.create_logical_not(ok));
      return b.create_select(ok,v,b.get_float64(0));
    };
    const auto mul = [&](Stmt *a, Stmt *c) -> Stmt * {
      auto *v=b.create_mul(a,c);
      fail(bad,b.create_logical_and(b.create_logical_and(
          b.create_cmp_ne(a,b.get_float64(0)),b.create_cmp_ne(c,b.get_float64(0))),
          b.create_cmp_eq(v,b.get_float64(0))));
      return checked(v,bad);
    };
    const auto add = [&](Stmt *a, Stmt *c, AllocaStmt *flag) -> Stmt * {
      auto *v=b.create_add(a,c);
      fail(flag,b.create_logical_and(b.create_cmp_eq(v,b.get_float64(0)),
                                    b.create_cmp_ne(a,b.create_neg(c))));
      return checked(v,flag);
    };
    const auto divide = [&](Stmt *a, Stmt *c) -> Stmt * {
      auto *v=b.create_div(a,c);
      fail(bad,b.create_logical_and(b.create_cmp_ne(a,b.get_float64(0)),
                                  b.create_cmp_eq(v,b.get_float64(0))));
      return checked(v,bad);
    };
    std::array<Stmt *,3> position{};
    for(int k=0;k<3;++k) position[k]=checked(load(14+k),bad);
    for(int j=0;j<packets;++j) {
      std::array<Stmt *,3> d{};
      for(int k=0;k<3;++k) {
        d[k]=checked(load(7*j+k),bad);
        auto *c=checked(load(7*j+4+k),contact_bad);
        b.create_local_store(contact[k],add(b.create_local_load(contact[k]),c,contact_bad));
      }
      auto *coupling=checked(load(7*j+3),bad);
      fail(bad,b.create_cmp_lt(coupling,b.get_float64(0)));
      auto active=b.get_if_guard(b.create_if(b.create_cmp_gt(coupling,b.get_float64(0))),true);
      auto *zero=b.create_logical_and(b.create_cmp_eq(d[0],b.get_float64(0)),
          b.create_logical_and(b.create_cmp_eq(d[1],b.get_float64(0)),
                              b.create_cmp_eq(d[2],b.get_float64(0))));
      {
        auto exact=b.get_if_guard(b.create_if(zero),true);
        b.create_local_store(coincident,b.create_add(b.create_local_load(coincident),b.get_int32(1)));
      }
      {
        auto separated=b.get_if_guard(b.create_if(b.create_logical_not(zero)),true);
        Stmt *d2=b.get_float64(0);
        for(auto *v:d) d2=add(d2,mul(v,v),bad);
        auto *positive=b.create_cmp_gt(d2,b.get_float64(0));
        fail(bad,b.create_logical_not(positive));
        auto *safe=b.create_select(positive,d2,b.get_float64(1));
        auto *distance=checked(b.create_sqrt(safe),bad);
        auto *cube=mul(safe,distance);
        auto *cube_positive=b.create_cmp_gt(cube,b.get_float64(0));
        fail(bad,b.create_logical_not(cube_positive));
        auto *alpha=divide(coupling,b.create_select(cube_positive,cube,b.get_float64(1)));
        b.create_local_store(weight,add(b.create_local_load(weight),alpha,bad));
        for(int k=0;k<3;++k)
          b.create_local_store(force[k],add(b.create_local_load(force[k]),mul(alpha,d[k]),bad));
      }
    }
    auto *has=b.create_cmp_gt(b.create_local_load(weight),b.get_float64(0));
    std::array<Stmt *,3> relative{}, common{};
    for(int k=0;k<3;++k) {
      relative[k]=divide(b.create_local_load(force[k]),b.create_select(has,b.create_local_load(weight),b.get_float64(1)));
      common[k]=add(position[k],relative[k],bad);
    }
    // Precedence is independent of encounter order: unsupported > coincidence
    // > supported > absent. Contacts have a separate validity flag.
    auto *field_ok=b.create_logical_not(b.create_local_load(bad));
    auto *contact_ok=b.create_logical_not(b.create_local_load(contact_bad));
    auto *status=b.create_select(b.create_logical_or(b.create_local_load(bad),b.create_local_load(contact_bad)),b.get_int32(3),
        b.create_select(b.create_cmp_gt(b.create_local_load(coincident),b.get_int32(0)),b.get_int32(2),
                        b.create_select(has,b.get_int32(1),b.get_int32(0))));
    auto *valid=b.create_cmp_eq(status,b.get_int32(1));
    const auto put=[&](int col,Stmt *value) {
      b.create_global_store(b.create_external_ptr(out,{b.get_int32(col),row}),b.create_cast(value,PrimitiveType::f64));
    };
    put(0,status); put(1,b.create_select(field_ok,b.get_int32(1),b.get_int32(0)));
    put(2,b.create_select(contact_ok,b.get_int32(1),b.get_int32(0)));
    put(3,b.create_local_load(coincident));
    for(int k=0;k<3;++k) {
      put(4+k,b.create_select(field_ok,b.create_local_load(force[k]),b.get_float64(0)));
      put(7+k,b.create_select(contact_ok,b.create_local_load(contact[k]),b.get_float64(0)));
      put(10+k,b.create_select(valid,relative[k],b.get_float64(0)));
      put(13+k,b.create_select(valid,common[k],b.get_float64(0)));
    }
    put(16,b.get_int32(packets));
  }
  return engine::CompiledKernel(runtime,"intent_packet_probe",b.extract_ir(),
      {engine::Param::array(PrimitiveType::f64,2),engine::Param::array(PrimitiveType::f64,2)},
      engine::MathMode::Strict,engine::Optimization::Local);
}

void run(int threads) {
  std::vector<Row> rows;
  const auto add=[&](double dx,double k,double dx2=0,double k2=0) {
    Row r{}; r[0]=dx;r[3]=k;r[7]=dx2;r[10]=k2;rows.push_back(r);
  };
  add(2,8); add(2,8,-1,1); add(1,1,-1,1); add(0,0); add(0,1,1,1);
  add(1,1); add(2,4); // equal force, different intent
  add(0,1,0,std::numeric_limits<double>::infinity());
  rows.push_back(rows.back()); for(int k=0;k<7;++k) std::swap(rows.back()[k],rows.back()[7+k]);
  add(0,1,1,-1); rows.push_back(rows.back()); for(int k=0;k<7;++k) std::swap(rows.back()[k],rows.back()[7+k]);
  add(1e200,1); add(1e-200,1); // squared-distance overflow/underflow
  add(1e100,1e-100); // coefficient underflow despite normal inputs
  add(1,1);rows.back()[4]=-3; // independent contact force
  add(2,8);rows.back()[14]=10;add(2,8);rows.back()[14]=11; // virtual drift
  add(2,8);rows.back()[14]=138;rows.back()[15]=-64; // translated common frame
  add(1,std::numeric_limits<double>::denorm_min());
  add(1,std::numeric_limits<double>::quiet_NaN(),0,1);
  rows.push_back(rows.back());for(int k=0;k<7;++k) std::swap(rows.back()[k],rows.back()[7+k]);
  add(1,1);rows.back()[4]=std::numeric_limits<double>::infinity();
  add(2,8,0,1);rows.back()[8]=1; // angled XY contributions
  add(0,8);rows.back()[2]=2; // independent Z component
  add(0,1); // lone coincidence is not absence
  add(1,1e308,1,1e308); // normalizer accumulation overflow
  add(1,1);rows.back()[4]=1e308;rows.back()[11]=1e308; // contact overflow
  add(1,1);rows.back()[14]=std::numeric_limits<double>::quiet_NaN();
  add(1,1);rows.back()[4]=std::numeric_limits<double>::min();
  rows.back()[11]=-std::nextafter(std::numeric_limits<double>::min(),1.0); // contact sum underflow
  require(rows.size()<=max_cases,"packet budget exceeded");
  const int n=static_cast<int>(rows.size());
  std::vector<double> input(columns*n),output(columns*n);
  for(int i=0;i<n;++i)for(int k=0;k<columns;++k)input[k*n+i]=rows[i][k];
  engine::InstanceSettings settings;settings.cpu_threads=threads;settings.read_environment=false;
  engine::Runtime runtime(settings);
  require(!runtime.on_cuda(),"CPU qualification requested");
  Ndarray in(&runtime.program(),PrimitiveType::f64,{columns,n}),out(&runtime.program(),PrimitiveType::f64,{columns,n});
  auto kernel=build(runtime,n);
  engine::upload(in,input.data(),input.size()*sizeof(double));
  auto invocation=kernel.invocation(true);
  auto &ctx=invocation.get();ctx.set_arg_ndarray({0},in);ctx.set_arg_ndarray({1},out);
  kernel.prepare(ctx);
  for(int repeat=0;repeat<2;++repeat) {
    kernel.launch(ctx);runtime.synchronize();
    engine::readback(out,output.data(),output.size()*sizeof(double));
    const auto value=[&](int i,int col){return output[col*n+i];};
    for(int i=0;i<n;++i) {
      near(value(i,16),2);
      for(int k=0;k<columns;++k)require(std::isfinite(value(i,k)),"nonfinite output escaped status");
    }
    near(value(0,0),1);near(value(0,4),2);near(value(0,10),2);
    near(value(1,10),0.5);near(value(2,0),1);near(value(2,10),0);near(value(3,0),0);
    near(value(4,0),2);near(value(4,1),1);near(value(4,4),1);near(value(4,10),0);
    near(value(5,4),value(6,4));near(value(5,10),1);near(value(6,10),2);
    for(int i:{7,8,9,10,11,12,13,18,19,20}){near(value(i,0),3);near(value(i,1),0);}
    near(value(14,4),1);near(value(14,7),-3);near(value(14,10),1);
    near(value(15,13),12);near(value(16,13),13);near(value(17,13),140);near(value(17,14),-64);
    near(value(21,0),3);near(value(21,1),1);near(value(21,2),0);near(value(21,4),1);
    near(value(22,4),2);near(value(22,5),1);near(value(22,10),1);near(value(22,11),0.5);
    near(value(23,6),2);near(value(23,12),2);near(value(24,0),2);near(value(24,4),0);
    near(value(25,0),3);near(value(25,1),0);
    near(value(26,0),3);near(value(26,1),1);near(value(26,2),0);
    near(value(27,0),3);near(value(27,1),0);
    near(value(28,0),3);near(value(28,2),0);
  }
  std::cout<<"intent_native PASS threads="<<threads<<" cases="<<n<<" packet_visits="<<2*n
           <<" ndarray_payload_bytes="<<2*columns*n*sizeof(double)
           <<" host_capacity_bytes="<<(input.capacity()+output.capacity())*sizeof(double)+rows.capacity()*sizeof(Row)
           <<" prepared_context_bytes="<<kernel.prepared_bytes()<<'\n';
}
}
int main(int argc,char **argv){try{int threads=argc>1?std::stoi(argv[1]):1;require(threads==1||threads==2,"workers");run(threads);return 0;}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
