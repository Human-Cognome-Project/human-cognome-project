#pragma once

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace engine {
// Synchronous borrowed view: its vector owner must remain alive and unchanged
// until the enclosing upload/launch + synchronize returns. Never an async
// lease. Only scalar f32/f64/i32 arrays cross this boundary, never C++ object
// bytes.
template <class T>
class PackedView {
 public:
  PackedView(std::vector<T> &storage, std::vector<int> shape,
             std::uint64_t generation)
      : data_(storage.data()),
        shape_(std::move(shape)),
        generation_(generation) {
    static_assert(
        (std::is_same<T, float>::value && sizeof(T) == 4) ||
            (std::is_same<T, double>::value && sizeof(T) == 8) ||
            (std::is_same<T, int>::value && sizeof(T) == 4),
        "packed ABI requires f32/f64/i32");
    std::size_t n = 1;
    if (shape_.empty() || !generation_)
      throw std::invalid_argument("packed schema");
    for (int axis : shape_) {
      if (axis < 1 ||
          n > std::numeric_limits<std::size_t>::max() / std::size_t(axis))
        throw std::invalid_argument("packed shape overflow");
      n *= std::size_t(axis);
    }
    if (n != storage.size() ||
        n > std::numeric_limits<std::size_t>::max() / sizeof(T) ||
        reinterpret_cast<std::uintptr_t>(data_) % alignof(T))
      throw std::invalid_argument("packed bounds/alignment");
    count_ = n;
  }
  T *data() const { return data_; }
  std::size_t bytes() const { return count_ * sizeof(T); }
  const std::vector<int> &shape() const { return shape_; }
  std::uint64_t generation() const { return generation_; }
  T &at(std::size_t row, std::size_t item) const {
    if (shape_.size() != 2 || row >= std::size_t(shape_[0]) ||
        item >= std::size_t(shape_[1]))
      throw std::out_of_range("packed index");
    return data_[row * std::size_t(shape_[1]) + item];
  }

 private:
  T *data_;
  std::vector<int> shape_;
  std::size_t count_;
  std::uint64_t generation_;
};
}  // namespace engine
