#pragma once

#include <memory>
#include <string>
#include <vector>

#include "engine/runtime.h"
#include "taichi/program/ndarray.h"

// A basic harness for the field model: one law, applied without exception.
//
// Every interaction is mass times mass over separation squared, evaluated
// against a group centroid rather than against another particle. A group's
// centroid always includes the subject. Parent and sibling participation are
// one edge shape: whole participation is component participation whose share
// is one and whose offset is zero.
//
// Nothing here decides what the data means. It moves state through one tick.
namespace field {

// Packed structure of arrays. Element f of item i lives at f * count + i, so
// every pass reads one contiguous run per field.
enum ParticleField {
  kPosX = 0, kPosY, kPosZ,
  kVelX, kVelY, kVelZ,
  kMass,
  kForceX, kForceY, kForceZ,
  kParticleFieldCount
};

enum GroupField {
  // Read by the force pass: the centroid as of the end of the previous tick.
  kCenX = 0, kCenY, kCenZ, kCenM,
  // Written by the centroid pass: the centroid the next tick will read.
  kAccX, kAccY, kAccZ, kAccM,
  kGroupFieldCount
};

// Participation of one particle in one group.
enum EdgeFloatField {
  // Share of the particle's total mass taking part. One for whole
  // participation, a fraction for a component.
  kShare = 0,
  // Where that share sits relative to the particle's centre. Zero for whole
  // participation; a component sits off-centre, and its pull is felt there
  // while the whole body translates in response (no rotation in the model).
  kOffX, kOffY, kOffZ,
  kEdgeFloatFieldCount
};

enum EdgeIntField { kEdgeParticle = 0, kEdgeGroup, kEdgeIntFieldCount };

// A pairwise ledgered connection between two particles of the same kind. Not
// a second force -- the one law still governs; a bond only sets how strongly
// this pair is coupled by it.
enum BondIntField { kBondA = 0, kBondB, kBondDefined, kBondIntFieldCount };
enum BondFloatField { kBondAlign = 0, kBondFloatFieldCount };

struct Parameters {
  // Calibration: two unconstrained mass-1 particles at separation 1 overlap at
  // their centroid in one time unit. Because the centroid carries the total
  // group mass and includes the subject, this is pi^2/128 rather than the
  // pairwise pi^2/16.
  float gravity{0.0771063443835106f};
  // One tick is one time unit; the brake is what makes a step that coarse
  // safe.
  float dt{1.0f};
  // Separator shell radius: the corona shell each particle carries.
  float corona{0.5f};
  // Corona separator push stiffness (a trial value, calibrate later).
  float sep_stiffness{1.0f};
  // Defined-bond invariant hold stiffness (a trial value, calibrate later).
  float hold_stiffness{1.0f};
  // Radial velocity dampening reach: the field-strength threshold M/d^2 at
  // which the reach sits. Inside the reach (M/d^2 above this threshold) the
  // dampening is inert; beyond it, outward radial velocity is bled off. A
  // trial value -- reach == sqrt(M / reach_strength), so at 1.0 the reach
  // sits at sqrt of the construct's total mass; calibrate later.
  float reach_strength{1.0f};
};

// One instance of the model's state on one runtime. Host vectors stage the
// same packed layout as the device arrays.
class Harness {
 public:
  Harness(engine::Runtime &runtime,
          int particle_count,
          int group_count,
          int edge_count,
          int bond_count = 0);

  int particle_count() const { return particles_; }
  int group_count() const { return groups_; }
  int edge_count() const { return edges_; }
  int bond_count() const { return bonds_; }

  // Host staging, in the packed layout above.
  std::vector<float> particle_data;
  std::vector<float> group_data;
  std::vector<float> edge_float_data;
  std::vector<int> edge_int_data;
  std::vector<float> bond_float_data;
  std::vector<int> bond_int_data;
  // The universal mass: the total mass of the construct, each particle
  // counted exactly once, held in the single cell every particle's
  // universal pull reads. Written on-device each tick by the clear pass;
  // not staged from the host.
  std::vector<float> universal_data;

  float &particle(int f, int i) { return particle_data[f * particles_ + i]; }
  float &group(int f, int i) { return group_data[f * groups_ + i]; }
  float &edge_float(int f, int i) { return edge_float_data[f * edges_ + i]; }
  int &edge_int(int f, int i) { return edge_int_data[f * edges_ + i]; }
  float &bond_float(int f, int i) { return bond_float_data[f * bonds_ + i]; }
  int &bond_int(int f, int i) { return bond_int_data[f * bonds_ + i]; }

  void upload();
  void download();

  // Build the centroids once from the staged state, so the first tick reads a
  // consistent field rather than zeros.
  void seed_centroids(const Parameters &p);

  // Force, integrate, determiners, then the consolidation that builds the
  // centroids the next tick will read.
  void tick(const Parameters &p);

  // Products of the primary pass, one per particle, written by the determiner
  // stage. Speed for now: the stage exists to hold the shape.
  const std::vector<float> &determined() const { return determined_; }

 private:
  void launch_clear();
  void launch_force(const Parameters &p);
  void launch_contact(const Parameters &p);
  void launch_universal(const Parameters &p);
  void launch_integrate(const Parameters &p);
  void launch_determine();
  void launch_centroid(const Parameters &p);

  engine::Runtime *runtime_;
  int particles_;
  int groups_;
  int edges_;
  int bonds_;

  std::unique_ptr<taichi::lang::Ndarray> particle_array_;
  std::unique_ptr<taichi::lang::Ndarray> group_array_;
  std::unique_ptr<taichi::lang::Ndarray> edge_float_array_;
  std::unique_ptr<taichi::lang::Ndarray> edge_int_array_;
  std::unique_ptr<taichi::lang::Ndarray> bond_float_array_;
  std::unique_ptr<taichi::lang::Ndarray> bond_int_array_;
  std::unique_ptr<taichi::lang::Ndarray> determined_array_;
  std::unique_ptr<taichi::lang::Ndarray> universal_array_;

  std::vector<float> determined_;

  engine::CompiledKernel clear_;
  engine::CompiledKernel force_;
  engine::CompiledKernel contact_;
  engine::CompiledKernel universal_;
  engine::CompiledKernel integrate_;
  engine::CompiledKernel determine_;
  engine::CompiledKernel centroid_;
  engine::CompiledKernel finalize_;
};

}  // namespace field
