#pragma once
#include "assembly/letter_projection.h"
#include <algorithm>

namespace assembly::detail {
inline const auto &key_buffer(const OccurrenceKey &v) { return v.source.path; }
inline const auto &key_buffer(const FieldKey &v) { return v.object.address; }
// Private identity staging storage. Logical removal must not destroy nested
// key backing. Every constructed slot survives until teardown; publication
// swaps entire stores. Reservation is a setup operation, never an append path.
template<class T> class ExchangeMap {
 public:
  ExchangeMap() = default;
  ExchangeMap(const std::vector<T> &rows) : slots_(rows), size_(rows.size()) {}
  ExchangeMap(const ExchangeMap &other) : slots_(other.slots_), size_(other.size_) {
    for (std::size_t i = 0; i < slots_.size(); ++i)
      nested(slots_[i]).reserve(nested(other.slots_[i]).capacity());
  }
  ExchangeMap &operator=(const ExchangeMap &other) {
    if (this != &other) {
      if (other.size() > slots_.size()) throw warm::Error("exchange map capacity exhausted");
      clear();
      for (const auto &row : other) push_back(row);
    }
    return *this;
  }
  void reserve(std::size_t count, std::size_t extent) {
    if (count < size_) throw warm::Error("exchange map reservation below live size");
    slots_.resize(count);
    for (auto &row : slots_) nested(row).reserve(extent);
  }
  auto begin() { return slots_.begin(); }
  auto begin() const { return slots_.begin(); }
  auto end() { return begin() + size_; }
  auto end() const { return begin() + size_; }
  auto &operator[](std::size_t i) { return slots_[i]; }
  const auto &operator[](std::size_t i) const { return slots_[i]; }
  auto &back() { return slots_[size_-1]; }
  const auto &back() const { return slots_[size_-1]; }
  std::size_t size() const { return size_; }
  void clear() { size_ = 0; }
  void push_back(const T &row) {
    if (size_ == slots_.size() || nested(row).size() > nested(slots_[size_]).capacity())
      throw warm::Error("exchange map identity reservation exceeded");
    slots_[size_] = row;
    ++size_;
  }
  template<class Key, class Handle> void push_key(const Key &key, const Handle &handle) {
    if (size_ == slots_.size() || key_buffer(key).size() > nested(slots_[size_]).capacity())
      throw warm::Error("exchange map identity reservation exceeded");
    set_key(slots_[size_],key);
    slots_[size_].handle = handle;
    ++size_;
  }
  void erase(typename std::vector<T>::iterator where) {
    // Rotate whole slots, preserving both ordering and all nested reservations.
    std::rotate(where,where+1,end());
    --size_;
  }
  void swap(ExchangeMap &other) noexcept {
    slots_.swap(other.slots_); std::swap(size_,other.size_);
  }
  const auto &storage() const { return slots_; }
 private:
  static auto &nested(T &row) { return key_buffer(row); }
  static const auto &nested(const T &row) { return key_buffer(row); }
  std::vector<T> slots_;
  std::size_t size_{};
};
}
