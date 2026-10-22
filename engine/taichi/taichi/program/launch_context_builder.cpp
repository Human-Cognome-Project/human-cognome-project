#include <taichi/program/launch_context_builder.h>
#define TI_RUNTIME_HOST
#include <taichi/program/context.h>
#undef TI_RUNTIME_HOST
#include "fp16.h"
#include <cstring>
#include <limits>
#include <stdexcept>

namespace taichi::lang {

namespace {
template <typename T>
inline std::vector<T> concatenate_vector(const std::vector<T> &lhs,
                                         const std::vector<T> &rhs) {
  std::vector<T> result;
  result.assign(lhs.begin(), lhs.end());
  result.insert(result.end(), rhs.begin(), rhs.end());
  return result;
}
}  // namespace

LaunchContextBuilder::LaunchContextBuilder(CallableBase *kernel)
    : kernel_(kernel),
      owned_ctx_(std::make_unique<RuntimeContext>()),
      ctx_(owned_ctx_.get()),
      arg_buffer_(std::make_unique<char[]>(kernel->args_size)),
      result_buffer_(std::make_unique<char[]>(kernel->ret_size)),
      ret_type_(kernel->ret_type),
      arg_buffer_size(kernel->args_size),
      args_type(kernel->args_type),
      result_buffer_size(kernel->ret_size) {
  ctx_->result_buffer = (uint64 *)result_buffer_.get();
  ctx_->arg_buffer = arg_buffer_.get();
  array_bindings.resize(kernel->parameter_list.size());
}

LaunchContextBuilder &LaunchContextBuilder::operator=(
    LaunchContextBuilder &&other) noexcept {
  if (this == &other)
    return *this;
  // Complete the displaced binding before releasing its borrowed host buffers.
  // Default memberwise assignment would replace those buffers first.
  prepared.reset();
  kernel_ = other.kernel_;
  owned_ctx_ = std::move(other.owned_ctx_);
  ctx_ = other.ctx_;
  arg_buffer_ = std::move(other.arg_buffer_);
  result_buffer_ = std::move(other.result_buffer_);
  ret_type_ = other.ret_type_;
  arg_buffer_size = other.arg_buffer_size;
  args_type = other.args_type;
  result_buffer_size = other.result_buffer_size;
  array_runtime_sizes = std::move(other.array_runtime_sizes);
  device_allocation_type = std::move(other.device_allocation_type);
  array_ptrs = std::move(other.array_ptrs);
  argpack_ptrs = std::move(other.argpack_ptrs);
  array_bindings = std::move(other.array_bindings);
  prepared = std::move(other.prepared);
  return *this;
}

void LaunchContextBuilder::set_arg_float(const std::vector<int> &arg_id,
                                         float64 d) {
  if (prepared)
    throw std::logic_error("use prepared scalar setter on a frozen context");
  auto dt = kernel_->args_type->get_element_type(arg_id);
  TI_ASSERT_INFO(dt->is<PrimitiveType>(),
                 "Assigning scalar value to external (numpy) array argument is "
                 "not allowed.");

  PrimitiveTypeID typeId = dt->as<PrimitiveType>()->type;

  switch (typeId) {
#define PER_C_TYPE(tp, ctype)  \
  case PrimitiveTypeID::tp:    \
    set_arg(arg_id, (ctype)d); \
    break;
#include "taichi/inc/data_type_with_c_type.inc.h"
#undef PER_C_TYPE
    case PrimitiveTypeID::f16: {
      uint16 half = fp16_ieee_from_fp32_value((float32)d);
      set_arg(arg_id, half);
      break;
    }
    default:
      TI_NOT_IMPLEMENTED
  }
}

template <typename T>
void LaunchContextBuilder::set_struct_arg(std::vector<int> arg_indices, T d) {
  auto dt = kernel_->args_type->get_element_type(arg_indices);

  TI_ASSERT(dt->is<PrimitiveType>() || dt->is<PointerType>());
  if (dt->is<PointerType>()) {
    set_struct_arg_impl(arg_indices, (uint64)d);
    return;
  }
  PrimitiveTypeID typeId = dt->as<PrimitiveType>()->type;

  switch (typeId) {
#define PER_C_TYPE(tp, ctype)                   \
  case PrimitiveTypeID::tp:                     \
    set_struct_arg_impl(arg_indices, (ctype)d); \
    break;
#include "taichi/inc/data_type_with_c_type.inc.h"
#undef PER_C_TYPE
    case PrimitiveTypeID::f16: {
      uint16 half = fp16_ieee_from_fp32_value((float32)d);
      set_struct_arg_impl(arg_indices, half);
      break;
    }
    default:
      TI_NOT_IMPLEMENTED
  }
}

void LaunchContextBuilder::set_ndarray_ptrs(const std::vector<int> &arg_id,
                                            uint64 data_ptr,
                                            uint64 grad_ptr) {
  auto param_indices = arg_id;
  param_indices.push_back(TypeFactory::DATA_PTR_POS_IN_NDARRAY);
  set_struct_arg(param_indices, data_ptr);
  if (kernel_->nested_parameters[arg_id].needs_grad) {
    param_indices.back() = TypeFactory::GRAD_PTR_POS_IN_NDARRAY;
    set_struct_arg(param_indices, grad_ptr);
  }
}

void LaunchContextBuilder::set_argpack_ptr(const std::vector<int> &arg_id,
                                           uint64 data_ptr) {
  auto param_indices = arg_id;
  param_indices.push_back(TypeFactory::DATA_PTR_POS_IN_ARGPACK);
  set_struct_arg(param_indices, data_ptr);
}

template void LaunchContextBuilder::set_struct_arg(std::vector<int> arg_indices,
                                                   uint64 v);
template void LaunchContextBuilder::set_struct_arg(std::vector<int> arg_indices,
                                                   int64 v);
template void LaunchContextBuilder::set_struct_arg(std::vector<int> arg_indices,
                                                   float64 v);

void LaunchContextBuilder::set_arg_int(const std::vector<int> &arg_id,
                                       int64 d) {
  if (prepared)
    throw std::logic_error("use prepared scalar setter on a frozen context");
  auto dt = kernel_->args_type->get_element_type(arg_id);

  TI_ASSERT_INFO(dt->is<PrimitiveType>(),
                 "Assigning scalar value to external (numpy) array argument is "
                 "not allowed.");

  if (dt->is_primitive(PrimitiveTypeID::i32)) {
    set_arg(arg_id, (int32)d);
  } else if (dt->is_primitive(PrimitiveTypeID::i64)) {
    set_arg(arg_id, (int64)d);
  } else if (dt->is_primitive(PrimitiveTypeID::i8)) {
    set_arg(arg_id, (int8)d);
  } else if (dt->is_primitive(PrimitiveTypeID::i16)) {
    set_arg(arg_id, (int16)d);
  } else if (dt->is_primitive(PrimitiveTypeID::u1)) {
    set_arg(arg_id, (uint1)d);
  } else if (dt->is_primitive(PrimitiveTypeID::u8)) {
    set_arg(arg_id, (uint8)d);
  } else if (dt->is_primitive(PrimitiveTypeID::u16)) {
    set_arg(arg_id, (uint16)d);
  } else if (dt->is_primitive(PrimitiveTypeID::u32)) {
    set_arg(arg_id, (uint32)d);
  } else if (dt->is_primitive(PrimitiveTypeID::u64)) {
    set_arg(arg_id, (uint64)d);
  } else {
    TI_INFO(dt->to_string());
    TI_NOT_IMPLEMENTED
  }
}

void LaunchContextBuilder::set_arg_uint(const std::vector<int> &arg_id,
                                        uint64 d) {
  set_arg_int(arg_id, d);
}

template <>
void LaunchContextBuilder::set_arg<TypedConstant>(const std::vector<int> &i,
                                                  TypedConstant d) {
  if (is_real(d.dt)) {
    set_arg_float(i, d.val_float());
  } else {
    if (is_signed(d.dt)) {
      set_arg_int(i, d.val_int());
    } else {
      set_arg_uint(i, d.val_uint());
    }
  }
}

template <typename T>
void LaunchContextBuilder::set_struct_arg_impl(std::vector<int> arg_indices,
                                               T v) {
  int offset = args_type->get_element_offset(arg_indices);
  TI_ASSERT(offset + sizeof(T) <= arg_buffer_size);
  if (prepared)
    throw std::logic_error("cannot mutate frozen argument metadata");
  *(T *)(ctx_->arg_buffer + offset) = v;
}

template <typename T>
T LaunchContextBuilder::get_arg(const std::vector<int> &i) {
  return get_struct_arg<T>(i);
}

template <typename T>
T LaunchContextBuilder::get_struct_arg(std::vector<int> arg_indices) {
  int offset = args_type->get_element_offset(arg_indices);
  TI_ASSERT(offset + sizeof(T) <= arg_buffer_size);
  return *(T *)(ctx_->arg_buffer + offset);
}

template <typename T>
void LaunchContextBuilder::set_arg(const std::vector<int> &i, T v) {
  set_struct_arg_impl(i, v);
  set_array_device_allocation_type(i, DevAllocType::kNone);
}

template <typename T>
T LaunchContextBuilder::get_ret(int i) {
  return taichi_union_cast_with_different_sizes<T>(ctx_->result_buffer[i]);
}

#define PER_C_TYPE(type, ctype)                                            \
  template void LaunchContextBuilder::set_struct_arg_impl(                 \
      std::vector<int> arg_indices, ctype v);                              \
  template ctype LaunchContextBuilder::get_arg(const std::vector<int> &i); \
  template ctype LaunchContextBuilder::get_struct_arg(                     \
      std::vector<int> arg_indices);                                       \
  template void LaunchContextBuilder::set_arg(const std::vector<int> &i,   \
                                              ctype v);                    \
  template ctype LaunchContextBuilder::get_ret(int i);
#include "taichi/inc/data_type_with_c_type.inc.h"
PER_C_TYPE(gen, void *)  // Register void* as a valid type
#undef PER_C_TYPE

void LaunchContextBuilder::set_array_runtime_size(const std::vector<int> &i,
                                                  uint64 size) {
  array_runtime_sizes[i] = size;
}

void LaunchContextBuilder::set_array_device_allocation_type(
    const std::vector<int> &i,
    DevAllocType usage) {
  device_allocation_type[i] = usage;
}

void LaunchContextBuilder::set_arg_external_array_with_shape(
    const std::vector<int> &arg_id,
    uintptr_t ptr,
    uint64 size,
    const std::vector<int64> &shape,
    uintptr_t grad_ptr) {
  if (prepared)
    throw std::logic_error("cannot rebind a prepared external array");
  TI_ASSERT_INFO(
      kernel_->nested_parameters[arg_id].is_array,
      "Assigning external (numpy) array to scalar argument is not allowed.");

  TI_ASSERT_INFO(shape.size() <= taichi_max_num_indices,
                 "External array cannot have > {max_num_indices} indices");
  array_ptrs[concatenate_vector<int>(
      arg_id, {TypeFactory::DATA_PTR_POS_IN_NDARRAY})] = (void *)ptr;
  array_ptrs[concatenate_vector<int>(
      arg_id, {TypeFactory::GRAD_PTR_POS_IN_NDARRAY})] = (void *)grad_ptr;
  set_array_runtime_size(arg_id, size);
  set_array_device_allocation_type(arg_id, DevAllocType::kNone);
  for (uint64 i = 0; i < shape.size(); ++i) {
    set_struct_arg(concatenate_vector<int>(arg_id, {0, (int32)i}),
                   (int32)shape[i]);
  }
  const auto &parameter = kernel_->nested_parameters.at(arg_id);
  if (arg_id.size() == 1 && parameter.ptype == ParameterType::kNdarray) {
    auto type = parameter.get_dtype()
                    ->as<StructType>()
                    ->get_element_type({TypeFactory::DATA_PTR_POS_IN_NDARRAY})
                    ->as<PointerType>()
                    ->get_pointee_type();
    auto &binding = array_bindings.at(arg_id[0]);
    binding.type = type;
    binding.rank = int(shape.size());
    const auto bytes = data_type_size(binding.type);
    binding.elements = bytes > 0 && size % bytes == 0 ? size / bytes : 0;
  }
}

void LaunchContextBuilder::prepare_scalars(Prepared &state) const {
  if (prepared)
    throw std::logic_error("context is already prepared");
  state.scalars.resize(kernel_->parameter_list.size());
  for (const auto &entry : kernel_->nested_parameters) {
    if (entry.first.size() != 1)
      throw std::invalid_argument("prepared launch requires flat parameters");
    const auto &p = entry.second;
    if (p.is_argpack || p.needs_grad)
      throw std::invalid_argument("unsupported prepared argpack/gradient");
    if (p.is_array) {
      if (p.ptype != ParameterType::kNdarray || !p.element_shape.empty() ||
          p.total_dim < 1 || p.total_dim > 2)
        throw std::invalid_argument(
            "prepared arrays require flat rank-1/2 ndarrays");
      const auto *pointer =
          p.get_dtype()
              ->as<StructType>()
              ->get_element_type({TypeFactory::DATA_PTR_POS_IN_NDARRAY})
              ->as<PointerType>();
      if (!pointer->get_pointee_type()->is<PrimitiveType>())
        throw std::invalid_argument(
            "prepared ndarray element is not primitive");
      const auto &binding = array_bindings.at(entry.first[0]);
      if (binding.rank != int(p.total_dim) ||
          binding.type != DataType(pointer->get_pointee_type()))
        throw std::invalid_argument(
            "prepared array type/rank differs from signature");
      std::size_t elements = 1;
      for (int axis = 0; axis < binding.rank; ++axis) {
        const int offset =
            args_type->get_element_offset({entry.first[0], 0, axis});
        int32 dimension;
        std::memcpy(&dimension, arg_buffer_.get() + offset, sizeof(dimension));
        if (dimension <= 0 ||
            elements > std::numeric_limits<std::size_t>::max() / dimension)
          throw std::invalid_argument("invalid prepared array extent");
        elements *= dimension;
      }
      if (elements != binding.elements)
        throw std::invalid_argument(
            "prepared array extent differs from bound storage");
      continue;
    }
    auto type = p.get_dtype();
    if (type != PrimitiveType::i32 && type != PrimitiveType::i64 &&
        type != PrimitiveType::f32 && type != PrimitiveType::f64)
      throw std::invalid_argument("unsupported prepared scalar type");
    auto &slot = state.scalars.at(entry.first[0]);
    slot.offset = args_type->get_element_offset(entry.first);
    slot.type = type;
    if (slot.offset < 0 ||
        std::size_t(slot.offset) + data_type_size(type) > arg_buffer_size)
      throw std::invalid_argument("prepared scalar outside argument buffer");
  }
}

void LaunchContextBuilder::set_prepared_int(int arg_id, int64 value) {
  if (!prepared || !prepared->usable || arg_id < 0 ||
      std::size_t(arg_id) >= prepared->scalars.size())
    throw std::invalid_argument("invalid prepared scalar argument");
  const auto &slot = prepared->scalars[arg_id];
  if (slot.offset < 0)
    throw std::invalid_argument("prepared argument is an array");
  if (slot.type == PrimitiveType::i32) {
    const int32 v = static_cast<int32>(value);
    std::memcpy(arg_buffer_.get() + slot.offset, &v, sizeof(v));
  } else if (slot.type == PrimitiveType::i64) {
    std::memcpy(arg_buffer_.get() + slot.offset, &value, sizeof(value));
  } else {
    throw std::invalid_argument("prepared argument requires floating value");
  }
}

void LaunchContextBuilder::set_prepared_float(int arg_id, float64 value) {
  if (!prepared || !prepared->usable || arg_id < 0 ||
      std::size_t(arg_id) >= prepared->scalars.size())
    throw std::invalid_argument("invalid prepared scalar argument");
  const auto &slot = prepared->scalars[arg_id];
  if (slot.offset < 0)
    throw std::invalid_argument("prepared argument is an array");
  if (slot.type == PrimitiveType::f32) {
    const float32 v = static_cast<float32>(value);
    std::memcpy(arg_buffer_.get() + slot.offset, &v, sizeof(v));
  } else if (slot.type == PrimitiveType::f64) {
    std::memcpy(arg_buffer_.get() + slot.offset, &value, sizeof(value));
  } else {
    throw std::invalid_argument("prepared argument requires integer value");
  }
}

void LaunchContextBuilder::set_arg_ndarray(const std::vector<int> &arg_id,
                                           const Ndarray &arr) {
  intptr_t ptr = arr.get_device_allocation_ptr_as_int();
  TI_ASSERT_INFO(arr.shape.size() <= taichi_max_num_indices,
                 "External array cannot have > {max_num_indices} indices");
  set_arg_ndarray_impl(arg_id, ptr, arr.shape);
  if (arg_id.size() == 1)
    array_bindings.at(arg_id[0]) = {arr.dtype, int(arr.shape.size()),
                                    arr.get_nelement()};
}

void LaunchContextBuilder::set_arg_argpack(const std::vector<int> &arg_id,
                                           const ArgPack &argpack) {
  argpack_ptrs[arg_id] = &argpack;
  if (arg_id.size() == 1) {
    // Only set ptr to arg buffer if this argpack is not nested
    set_argpack_ptr(arg_id, argpack.get_device_allocation_ptr_as_int());
  }
  // TODO: Consider renaming this method to `set_device_allocation_type`
  set_array_device_allocation_type(arg_id, DevAllocType::kArgPack);
}

void LaunchContextBuilder::set_arg_ndarray_with_grad(
    const std::vector<int> &arg_id,
    const Ndarray &arr,
    const Ndarray &arr_grad) {
  intptr_t ptr = arr.get_device_allocation_ptr_as_int();
  intptr_t ptr_grad = arr_grad.get_device_allocation_ptr_as_int();
  TI_ASSERT_INFO(arr.shape.size() <= taichi_max_num_indices,
                 "External array cannot have > {max_num_indices} indices");
  set_arg_ndarray_impl(arg_id, ptr, arr.shape, ptr_grad);
}

void LaunchContextBuilder::set_arg_texture(const std::vector<int> &arg_id,
                                           const Texture &tex) {
  intptr_t ptr = tex.get_device_allocation_ptr_as_int();
  set_arg_texture_impl(arg_id, ptr);
}

void LaunchContextBuilder::set_arg_rw_texture(const std::vector<int> &arg_id,
                                              const Texture &tex) {
  intptr_t ptr = tex.get_device_allocation_ptr_as_int();
  set_arg_rw_texture_impl(arg_id, ptr, tex.get_size());
}

RuntimeContext &LaunchContextBuilder::get_context() {
  return *ctx_;
}

void LaunchContextBuilder::set_arg_texture_impl(const std::vector<int> &arg_id,
                                                intptr_t alloc_ptr) {
  array_ptrs[arg_id] = (void *)alloc_ptr;
  set_array_device_allocation_type(arg_id, DevAllocType::kTexture);
}

void LaunchContextBuilder::set_arg_rw_texture_impl(
    const std::vector<int> &arg_id,
    intptr_t alloc_ptr,
    const std::array<int, 3> &shape) {
  array_ptrs[arg_id] = (void *)alloc_ptr;
  set_array_device_allocation_type(arg_id, DevAllocType::kRWTexture);
  TI_ASSERT(shape.size() <= taichi_max_num_indices);
  for (int i = 0; i < shape.size(); ++i) {
    set_struct_arg(concatenate_vector<int>(arg_id, {0, i}), shape[i]);
  }
}

void LaunchContextBuilder::set_arg_ndarray_impl(const std::vector<int> &arg_id,
                                                intptr_t devalloc_ptr,
                                                const std::vector<int> &shape,
                                                intptr_t devalloc_ptr_grad) {
  if (prepared)
    throw std::logic_error("cannot rebind a prepared array");
  // Set array ptr
  array_ptrs[concatenate_vector<int>(
      arg_id, {TypeFactory::DATA_PTR_POS_IN_NDARRAY})] = (void *)devalloc_ptr;
  if (devalloc_ptr != 0) {
    array_ptrs[concatenate_vector<int>(
        arg_id, {TypeFactory::GRAD_PTR_POS_IN_NDARRAY})] =
        (void *)devalloc_ptr_grad;
  }
  // Set device allocation type and runtime size
  set_array_device_allocation_type(arg_id, DevAllocType::kNdarray);
  TI_ASSERT(shape.size() <= taichi_max_num_indices);
  size_t total_size = 1;
  for (int i = 0; i < shape.size(); i++) {
    set_struct_arg(concatenate_vector<int>(arg_id, {0, (int32)i}),
                   (int32)shape[i]);
    total_size *= shape[i];
  }
  set_array_runtime_size(arg_id, total_size);
}

void LaunchContextBuilder::set_arg_matrix(int arg_id, const Matrix &matrix) {
  int type_size = data_type_size(matrix.dtype());
  for (uint32_t i = 0; i < matrix.length(); i++) {
    switch (type_size) {
      case 1:
        set_struct_arg_impl({arg_id, (int32)i},
                            taichi_union_cast_with_different_sizes<int8>(
                                reinterpret_cast<uint8_t *>(matrix.data())[i]));
        break;
      case 2:
        set_struct_arg_impl(
            {arg_id, (int32)i},
            taichi_union_cast_with_different_sizes<int16>(
                reinterpret_cast<uint16_t *>(matrix.data())[i]));
        break;
      case 4:
        set_struct_arg_impl(
            {arg_id, (int32)i},
            taichi_union_cast_with_different_sizes<int32>(
                reinterpret_cast<uint32_t *>(matrix.data())[i]));
        break;
      case 8:
        set_struct_arg_impl(
            {arg_id, (int32)i},
            taichi_union_cast_with_different_sizes<int64>(
                reinterpret_cast<uint64_t *>(matrix.data())[i]));
        break;
      default:
        TI_ERROR("Unsupported type size {}", type_size);
    }
  }
}

TypedConstant LaunchContextBuilder::fetch_ret(const std::vector<int> &index) {
  const Type *dt = ret_type_->get_element_type(index);
  int offset = ret_type_->get_element_offset(index);
  return fetch_ret_impl(offset, dt);
}

float64 LaunchContextBuilder::get_struct_ret_float(
    const std::vector<int> &index) {
  return fetch_ret(index).val_float();
}

int64 LaunchContextBuilder::get_struct_ret_int(const std::vector<int> &index) {
  return fetch_ret(index).val_int();
}

uint64 LaunchContextBuilder::get_struct_ret_uint(
    const std::vector<int> &index) {
  return fetch_ret(index).val_uint();
}

TypedConstant LaunchContextBuilder::fetch_ret_impl(int offset, const Type *dt) {
  TI_ASSERT(dt->is<PrimitiveType>());
  auto primitive_type = dt->as<PrimitiveType>();
  char *ptr = result_buffer_.get() + offset;
  switch (primitive_type->type) {
#define PER_C_TYPE(type, ctype) \
  case PrimitiveTypeID::type:   \
    return TypedConstant(*(ctype *)ptr);
#include "taichi/inc/data_type_with_c_type.inc.h"
#undef PER_C_TYPE
    case PrimitiveTypeID::f16: {
      // first fetch the data as u16, and then convert it to f32
      uint16 half = *(uint16 *)ptr;
      return TypedConstant(fp16_ieee_to_fp32_value(half));
    }
    default:
      TI_NOT_IMPLEMENTED
  }
}

}  // namespace taichi::lang
