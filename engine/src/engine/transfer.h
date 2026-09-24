#pragma once

#include <cstddef>

#include "taichi/program/ndarray.h"

namespace engine {

// Bulk host/device transfer for one engine ndarray, in one call rather than
// one element at a time.
void upload(taichi::lang::Ndarray &array, const void *src, std::size_t bytes);
void readback(taichi::lang::Ndarray &array, void *dst, std::size_t bytes);

}  // namespace engine
