#pragma once
#include "assembly/letter_projection.h"
#include "assembly/reservation_allocator.h"
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
  explicit ExchangeMap(std::shared_ptr<ReservationDomain> domain)
      : domain_(std::move(domain)), slots_(ReservationAllocator<T>(domain_.get())) {}
  ExchangeMap(const std::vector<T> &rows, std::shared_ptr<ReservationDomain> domain = {})
      : ExchangeMap(std::move(domain)) {
    slots_.resize(rows.size());
    for (std::size_t i=0; i<rows.size(); ++i) {
      initialize_key(slots_[i],domain_);
      slots_[i]=rows[i];
    }
    size_=rows.size();
  }
  // Ordinary snapshots detach, including inactive paths. Explicit setup copies
  // attach before allocating nested capacity, never after detached copying.
  ExchangeMap(const ExchangeMap &other) : ExchangeMap(other,{}) {}
  ExchangeMap(const ExchangeMap &other, std::shared_ptr<ReservationDomain> domain)
      : ExchangeMap(std::move(domain)) {
    slots_.resize(other.slots_.size());
    for (std::size_t i=0; i<slots_.size(); ++i) {
      initialize_key(slots_[i],domain_);
      nested(slots_[i]).reserve(nested(other.slots_[i]).capacity());
      slots_[i]=other.slots_[i];
    }
    size_=other.size_;
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
    const auto previous=slots_.size();
    slots_.resize(count);
    for (std::size_t i=previous; i<count; ++i) initialize_key(slots_[i],domain_);
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
  void erase(typename ReservedVector<T>::iterator where) {
    // Rotate whole slots, preserving both ordering and all nested reservations.
    std::rotate(where,where+1,end());
    --size_;
  }
  void swap(ExchangeMap &other) noexcept {
    slots_.swap(other.slots_); domain_.swap(other.domain_); std::swap(size_,other.size_);
  }
  const auto &storage() const { return slots_; }
 private:
  static auto &nested(T &row) { return key_buffer(row); }
  static const auto &nested(const T &row) { return key_buffer(row); }
  // Declared first so even empty maps keep the raw outer allocator alive until
  // after slot destruction. Swapping stores swaps this lifetime as well.
  std::shared_ptr<ReservationDomain> domain_;
  ReservedVector<T> slots_;
  std::size_t size_{};
};
}
