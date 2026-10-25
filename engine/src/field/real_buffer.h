#pragma once
#include <vector>

#include "engine/transfer.h"

namespace field::detail {
// One populated typed vector, selected at preparation. The inactive vector is
// empty; legacy references keep their existing C++ source interface.
class RealBuffer {
 public:
  void configure(bool wide) { wide_ = wide; }
  bool wide() const { return wide_; }
  std::vector<float> &legacy() { return floats_; }
  const std::vector<float> &legacy() const { return floats_; }
  std::vector<double> &doubles() { return doubles_; }
  const std::vector<double> &doubles() const { return doubles_; }
  void assign(std::size_t n, double value) {
    if (wide_)
      doubles_.assign(n, value);
    else
      floats_.assign(n, float(value));
  }
  void resize(std::size_t n) {
    if (wide_)
      doubles_.resize(n);
    else
      floats_.resize(n);
  }
  std::size_t size() const { return wide_ ? doubles_.size() : floats_.size(); }
  std::size_t element_bytes() const {
    return wide_ ? sizeof(double) : sizeof(float);
  }
  std::size_t bytes() const { return size() * element_bytes(); }
  void *data() {
    return wide_ ? static_cast<void *>(doubles_.data())
                 : static_cast<void *>(floats_.data());
  }
  const void *data() const {
    return wide_ ? static_cast<const void *>(doubles_.data())
                 : static_cast<const void *>(floats_.data());
  }
  void *data_at(std::size_t i) {
    return static_cast<unsigned char *>(data()) + i * element_bytes();
  }
  double operator[](std::size_t i) const {
    return wide_ ? doubles_[i] : floats_[i];
  }
  struct Reference {
    RealBuffer &buffer;
    std::size_t index;
    operator double() const {
      return static_cast<const RealBuffer &>(buffer)[index];
    }
    Reference &operator=(double value) {
      if (buffer.wide_)
        buffer.doubles_[index] = value;
      else
        buffer.floats_[index] = float(value);
      return *this;
    }
    Reference &operator=(const Reference &r) { return *this = double(r); }
  };
  Reference operator[](std::size_t i) { return {*this, i}; }
  void upload(taichi::lang::Ndarray &a) const {
    engine::upload(a, data(), bytes());
  }
  void readback(taichi::lang::Ndarray &a) {
    engine::readback(a, data(), bytes());
  }

 private:
  bool wide_{};
  std::vector<float> floats_;
  std::vector<double> doubles_;
};
}  // namespace field::detail
