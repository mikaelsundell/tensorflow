/* Copyright 2015 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

// Suite of datatypes to represent data-parallel kernel objects (code entities).
// Kernel is the untyped variant, whereas TypedKernel takes a type signature
// to do some template-based helper generation and give compile-time type
// checking for kernel launch parameters.
//
// Users typically don't see KernelBase, they see typed kernels, analogous to a
// typed function pointer. TypedKernels express their argument types via
// template parameters like so:
//
//  TypedKernel<DeviceMemory<int>*, int>
//
// Which expresses a data parallel kernel signature for:
//
//  void(int*, int);
//
// And for a const memory region:
//
//  TypedKernel<const DeviceMemory<int>&, int>
//
// Corresponds to a data parallel kernel signature for:
//
//  void(const int*, int)
//
// Note that kernels always have a void return type, so results typically must
// be memcpy'ied from device memory to the host.
//
// Also note that a scalar integer residing in device memory and an array of
// integers residing in device memory have the same signature: DeviceMemory<T>.
// However, in the future, checks may be added for additional safety that arrays
// of minimum sizes are passed when those minimum sizes are contractually
// expected by the kernel.
//
// For user-defined types whose definitions are appropriately shared between the
// host code doing the launching and the kernel code being launched, the user
// defined types are similarly permitted to be expressed as residing in device
// memory:
//
//  TypedKernel<DeviceMemory<MyUserDefinedStructure>>
//
// And, when the alignment and padding are agreed upon, POD types will also be
// able to be passed by value; for example, it is a common idiom to specify a
// bunch of options simultaneously with a structure:
//
//  TypedKernel<MyOptionsStructurePassedByValue, DeviceMemory<float>>
//
// Which corresponds to a data parallel kernel signature like:
//
//  void(MyOptionsStructurePassedByValue value, float *result);
//
// Users typically won't need to type out the TypedKernel signature in full, it
// will be typedef'd by automatically generated code; for example, see
// stream_executor::executor_sample::VecReduceAddKernel.

#ifndef XLA_STREAM_EXECUTOR_KERNEL_H_
#define XLA_STREAM_EXECUTOR_KERNEL_H_

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

#include "absl/log/check.h"
#include "absl/meta/type_traits.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "xla/stream_executor/device_memory.h"
#include "tsl/platform/statusor.h"

namespace stream_executor {

class StreamExecutor;

namespace internal {
class KernelInterface;
}  // namespace internal

//===----------------------------------------------------------------------===//
// Kernel cache config
//===----------------------------------------------------------------------===//

// This enum represents potential configurations of L1/shared memory when
// running a particular kernel. These values represent user preference, and
// the runtime is not required to respect these choices.
enum class KernelCacheConfig {
  // Indicates no preference for device L1/shared memory configuration.
  kNoPreference,

  // Indicates a preference for more shared memory than L1 cache.
  kPreferShared,

  // Indicates a preference for more L1 cache than shared memory.
  kPreferL1,

  // Indicates a preference for equal amounts of L1 cache and shared memory.
  kPreferEqual,
};

//===----------------------------------------------------------------------===//
// Kernel metadata
//===----------------------------------------------------------------------===//

// KernelMetadata holds runtime-queryable attributes of a loaded kernel, such as
// registers allocated, shared memory used, etc.
// Not all platforms support reporting of all information, so each accessor
// returns false if the associated field is not populated in the underlying
// platform.
class KernelMetadata {
 public:
  KernelMetadata() = default;

  // Returns the number of registers used per thread executing this kernel.
  std::optional<int64_t> registers_per_thread() const;

  // Returns the amount of [static] shared memory used per block executing this
  // kernel. Note that dynamic shared memory allocations are not (and can not)
  // be reported here (since they're not specified until kernel launch time).
  std::optional<int64_t> shared_memory_bytes() const;

  void set_registers_per_thread(int registers_per_thread);
  void set_shared_memory_bytes(int shared_memory_bytes);

 private:
  std::optional<int64_t> registers_per_thread_;
  std::optional<int64_t> shared_memory_bytes_;
};

//===----------------------------------------------------------------------===//
// Kernel
//===----------------------------------------------------------------------===//

// A data-parallel kernel (code entity) for launching via the StreamExecutor,
// analogous to a void* device function pointer. See TypedKernel for the typed
// variant.
//
// Thread-compatible.
class KernelBase {
 public:
  KernelBase(KernelBase &&from);

  // Constructs an "empty" (not-yet-loaded) kernel instance.
  //
  // parent is the StreamExecutor that will be responsible for loading the
  // implementation of this kernel. It must not be null.
  explicit KernelBase(StreamExecutor *parent);

  // Test-only constructor that can take a mock KernelInterface implementation.
  KernelBase(StreamExecutor *parent, internal::KernelInterface *implementation);

  // Releases resources associated with the kernel instance (i.e.
  // platform-specific implementation).
  ~KernelBase();

  // Returns the number of parameters that this kernel accepts. (Arity refers to
  // nullary, unary, ...).
  unsigned Arity() const;

  // Returns the StreamExecutor that represents the platform this kernel
  // executes upon.
  StreamExecutor *parent() const { return parent_; }

  // Returns a const pointer to the (opaque) platform-dependent implementation.
  const internal::KernelInterface *implementation() const {
    return implementation_.get();
  }

  // Returns a non-const pointer to the (opaque) platform-dependent
  // implementation.
  internal::KernelInterface *implementation() { return implementation_.get(); }

  void set_metadata(const KernelMetadata &metadata) { metadata_ = metadata; }

  const KernelMetadata &metadata() const { return metadata_; }

  // Sets the preferred cache configuration for a kernel. This is just a
  // suggestion to the runtime, and may not be honored during execution.
  void SetPreferredCacheConfig(KernelCacheConfig config);

  // Gets the preferred cache configuration for a kernel.
  KernelCacheConfig GetPreferredCacheConfig() const;

  void set_name(absl::string_view name);
  const std::string &name() const { return name_; }
  const std::string &demangled_name() const { return demangled_name_; }

 private:
  // The StreamExecutor that loads this kernel object.
  StreamExecutor *parent_;

  // Implementation delegated to for platform-specific functionality.
  std::unique_ptr<internal::KernelInterface> implementation_;

  std::string name_;
  std::string demangled_name_;

  KernelMetadata metadata_;

  KernelBase(const KernelBase &) = delete;
  void operator=(const KernelBase &) = delete;
};

//===----------------------------------------------------------------------===//
// Typed kernel
//===----------------------------------------------------------------------===//

// Typed variant of KernelBase, like a typed device function pointer.
template <typename... Params>
class TypedKernel : public KernelBase {
 public:
  static constexpr size_t kNumberOfParameters = sizeof...(Params);

  explicit TypedKernel(StreamExecutor *parent) : KernelBase(parent) {}
};

//===----------------------------------------------------------------------===//
// Kernel arguments
//===----------------------------------------------------------------------===//

// A virtual base class for passing kernel arguments to a stream executor APIs.
class KernelArgsArrayBase {
 public:
  template <typename T>
  using IsKernelArgs =
      std::enable_if_t<std::is_base_of<KernelArgsArrayBase, T>::value>;

  enum class Kind { kPackedArray };

  virtual ~KernelArgsArrayBase() = default;

  // Gets the number of arguments added so far, including shared memory
  // arguments.
  virtual size_t number_of_arguments() const = 0;

  // Gets the total number of shared memory bytes added so far.
  virtual uint64_t number_of_shared_bytes() const = 0;

  virtual Kind kind() const = 0;
};

// A small LLVM-style RTTI library for casting kernel arguments.
template <class T, KernelArgsArrayBase::IsKernelArgs<T> * = nullptr>
const T *Cast(const KernelArgsArrayBase *args) {
  CHECK(T::classof(args)) << "Invalid arguments casting to a destination type: "
                          << typeid(T).name();
  CHECK(args != nullptr) << "Casted arguments must be not null";
  return static_cast<const T *>(args);
}

template <class T, KernelArgsArrayBase::IsKernelArgs<T> * = nullptr>
const T *DynCast(const KernelArgsArrayBase *args) {
  CHECK(args != nullptr) << "Casted arguments must be not null";
  return T::classof(args) ? static_cast<const T *>(args) : nullptr;
}

template <class T, KernelArgsArrayBase::IsKernelArgs<T> * = nullptr>
const T *DynCastOrNull(const KernelArgsArrayBase *args) {
  return args && T::classof(args) ? static_cast<const T *>(args) : nullptr;
}

//===----------------------------------------------------------------------===//
// Kernel arguments packed array
//===----------------------------------------------------------------------===//

// A virtual base class for passing kernel arguments packed into a storage so
// that we have stable addresses for all arguments. This is a low level API for
// passing arguments in a platform-specific way that relies on the knowledge of
// the ABI of the underlying platform.
//
// For example `cuLaunchKernel` accepts arguments as `void** kernelParams`, and
// packed array base guarantees that `argument_addresses` are compatible with
// the CUDA APIs.
//
// See: https://docs.nvidia.com/cuda/cuda-driver-api/group__CUDA__EXEC.html
class KernelArgsPackedArrayBase : public KernelArgsArrayBase {
 public:
  // Gets the list of argument addresses.
  virtual absl::Span<const void *const> argument_addresses() const = 0;

  static bool classof(const KernelArgsArrayBase *args) {
    return args->kind() == Kind::kPackedArray;
  }

  Kind kind() const final { return Kind::kPackedArray; }
};

//===----------------------------------------------------------------------===//
// Kernel arguments packing for device memory and POD args
//===----------------------------------------------------------------------===//

// KernelArgsPackedArray is optimized for packing DeviceMemoryBase pointers
// and POD arguments (i.e. scalars) when the number and type of arguments are
// not known at compile time.

namespace internal {

// An empty storage for packing just the device memory arguments, that are
// stored directly in the `KernelArgsPackedArray`.
class EmptyArgs {};

// A storage for POD generic arguments that are smaller than `size` and require
// alignment smaller or equal to `alignment`.
template <size_t capacity, size_t size = 8,
          size_t alignment = alignof(std::max_align_t)>
class PodArgs {
 protected:
  template <typename T>
  const std::byte *add_pod_argument(const T &arg) {
    static_assert(
        std::is_pod_v<T> && sizeof(T) <= size & alignof(T) <= alignment,
        "Type is not compatible with POD arguments storage");

    assert(num_args_ < capacity && "pod args overflow");
    std::byte *arg_storage = args_storage_[num_args_++].storage;
    std::memcpy(arg_storage, &arg, sizeof(T));

    return arg_storage;
  }

 private:
  struct Arg {
    alignas(alignment) std::byte storage[size];
  };

  size_t num_args_ = 0;
  std::array<Arg, capacity> args_storage_;
};

template <typename ArgsStorage>
static constexpr bool is_pod_args_v = false;

template <size_t capacity, size_t size, size_t alignment>
static constexpr bool is_pod_args_v<PodArgs<capacity, size, alignment>> = true;

}  // namespace internal

// An array of arguments for a kernel call.
//
// The template parameter `num_args` is the maximum number of arguments which
// can be stored in the array.
template <size_t num_args, typename ArgsStorage = internal::PodArgs<num_args>>
class KernelArgsPackedArray : public KernelArgsPackedArrayBase, ArgsStorage {
 public:
  KernelArgsPackedArray() = default;

  // KernelArgsPackedArray is not copyable or movable because argument addresses
  // point to inline storage that can't be moved.
  KernelArgsPackedArray(const KernelArgsPackedArray &) = delete;
  KernelArgsPackedArray &operator=(const KernelArgsPackedArray &) = delete;

  // Adds an argument to the list.
  template <typename T>
  void add_argument(const T &arg) {
    if constexpr (internal::is_pod_args_v<ArgsStorage>) {
      argument_addresses_[number_of_argument_addresses_++] =
          ArgsStorage::add_pod_argument(arg);
    } else {
      static_assert(false, "Arguments storage is not supported");
    }
  }

  // Adds a device memory argument to the list.
  void add_device_memory_argument(const DeviceMemoryBase &arg) {
    const void **copy_ptr =
        &device_memory_opaque_pointers_[number_of_argument_addresses_];
    *copy_ptr = arg.opaque();
    argument_addresses_[number_of_argument_addresses_] = copy_ptr;
    ++number_of_argument_addresses_;
  }

  // Adds a shared memory argument to the list.
  //
  // The only significant information about a shared argument is its size, so
  // that is the only parameter in this function.
  void add_shared_bytes(size_t number_of_bytes) {
    total_shared_memory_bytes_ += number_of_bytes;
  }

  // Gets the number of arguments added so far, including shared memory
  // arguments.
  size_t number_of_arguments() const final {
    return number_of_argument_addresses_ + (total_shared_memory_bytes_ > 0);
  }

  // Gets the total number of shared memory bytes added so far.
  uint64_t number_of_shared_bytes() const final {
    return total_shared_memory_bytes_;
  }

  // Gets the list of argument addresses.
  absl::Span<const void *const> argument_addresses() const final {
    return absl::Span<const void *const>(argument_addresses_.data(),
                                         number_of_argument_addresses_);
  }

 private:
  // A place to store copies of opaque pointers from device memory arguments.
  std::array<const void *, num_args> device_memory_opaque_pointers_;

  // Addresses for non-shared-memory arguments.
  std::array<const void *, num_args> argument_addresses_;

  // Total of all shared memory sizes.
  size_t total_shared_memory_bytes_ = 0;

  // Number of significant entries in argument_addresses_.
  size_t number_of_argument_addresses_ = 0;
};

namespace internal {
template <int n>
std::unique_ptr<KernelArgsPackedArrayBase> PackKernelArgs(
    absl::Span<const DeviceMemoryBase> args, uint32_t shared_mem_bytes) {
  auto packed = std::make_unique<KernelArgsPackedArray<n, EmptyArgs>>();
  for (const DeviceMemoryBase &buf : args) {
    packed->add_device_memory_argument(buf);
  }
  if (shared_mem_bytes > 0) {
    packed->add_shared_bytes(shared_mem_bytes);
  }
  return packed;
}
}  // namespace internal

inline tsl::StatusOr<std::unique_ptr<KernelArgsPackedArrayBase>> PackKernelArgs(
    absl::Span<const DeviceMemoryBase> args, uint32_t shared_mem_bytes) {
  static constexpr int kKernelArgsLimit = 1024;

  if (args.size() > kKernelArgsLimit)
    return absl::InvalidArgumentError(absl::StrCat(
        "Can't pack device memory arguments array of size ", args.size(),
        " which is larger than the maximum supported size of ",
        kKernelArgsLimit));

  // Specialize kernel arguments array for small sizes to allocate a smaller
  // chunk of memory and hopefully hit a small allocations cache.
  if (args.size() <= 4) {
    return internal::PackKernelArgs<4>(args, shared_mem_bytes);
  } else if (args.size() <= 8) {
    return internal::PackKernelArgs<8>(args, shared_mem_bytes);
  } else if (args.size() <= 16) {
    return internal::PackKernelArgs<16>(args, shared_mem_bytes);
  } else if (args.size() <= 32) {
    return internal::PackKernelArgs<32>(args, shared_mem_bytes);
  } else if (args.size() <= 64) {
    return internal::PackKernelArgs<64>(args, shared_mem_bytes);
  } else if (args.size() <= 256) {
    return internal::PackKernelArgs<256>(args, shared_mem_bytes);
  } else if (args.size() <= 512) {
    return internal::PackKernelArgs<512>(args, shared_mem_bytes);
  }

  return internal::PackKernelArgs<kKernelArgsLimit>(args, shared_mem_bytes);
}

inline tsl::StatusOr<std::unique_ptr<KernelArgsPackedArrayBase>> PackKernelArgs(
    absl::Span<const DeviceMemoryBase> args, const KernelMetadata &metadata) {
  return PackKernelArgs(args, metadata.shared_memory_bytes().value_or(0));
}

//===----------------------------------------------------------------------===//
// Kernel arguments packing for statically know argument types
//===----------------------------------------------------------------------===//

// KernelArgsPackedTuple is optimized for packing arguments when their types are
// known at compile time, and somewhat similar to `std::tuple` but with a few
// special rules for passing device memory arguments.

namespace internal {

// PackedArgType template specialization defines what storage type we'll be
// using for each kernel argument type:
//
//   (1) We always strip references and store a copy of an argument.
//   (2) We do not support pointer arguments, as we should not be passing a
//       pointers to host memory to device kernels.
//   (3) DeviceMemory passed as an opaque `void*` pointer.
//   (4) We have a special case for passing pointers to DeviceMemory where we
//       also pass it as an opaque device pointer.
template <typename T>
struct PackedArgType {
  static_assert(!std::is_pointer_v<T>, "cannot pass raw pointer to the device");
  using Type = T;
};

template <>
struct PackedArgType<DeviceMemoryBase> {
  using Type = const void *;
};

template <typename T>
struct PackedArgType<DeviceMemory<T>> {
  using Type = typename PackedArgType<DeviceMemoryBase>::Type;
};

template <>
struct PackedArgType<DeviceMemoryBase *> {
  using Type = typename PackedArgType<DeviceMemoryBase>::Type;
};

template <>
struct PackedArgType<const DeviceMemoryBase *> {
  using Type = typename PackedArgType<DeviceMemoryBase>::Type;
};

template <typename T>
struct PackedArgType<DeviceMemory<T> *> {
  using Type = typename PackedArgType<DeviceMemoryBase>::Type;
};

template <typename T>
struct PackedArgType<const DeviceMemory<T> *> {
  using Type = typename PackedArgType<DeviceMemoryBase>::Type;
};

// Overload set for packing kernel arguments. This overload set matches
// supported kernel arguments types defined by `PackedArgType`.
template <typename T>
T PackArg(const T &arg) {
  return arg;
}

inline const void *PackArg(const DeviceMemoryBase &arg) { return arg.opaque(); }
inline const void *PackArg(const DeviceMemoryBase *arg) {
  return PackArg(*arg);
}

template <typename T>
const void *PackArg(const DeviceMemory<T> &arg) {
  return arg.opaque();
}

template <typename T>
const void *PackArg(const DeviceMemory<T> *arg) {
  return PackArg(*arg);
}

}  // namespace internal

template <typename... Args>
class KernelArgsPackedTuple : public KernelArgsPackedArrayBase {
 public:
  static constexpr size_t kSize = sizeof...(Args);

  using Storage = std::tuple<
      typename internal::PackedArgType<absl::remove_cvref_t<Args>>::Type...>;

  explicit KernelArgsPackedTuple(size_t shared_memory_bytes, Args... args)
      : shared_memory_bytes_(shared_memory_bytes),
        storage_(internal::PackArg(std::forward<Args>(args))...) {
    InitializeArgumentAddresses(std::make_index_sequence<kSize>{});
  }

  // KernelArgsPackedTuple is not copyable or movable because argument addresses
  // point to inline storage that can't be moved.
  KernelArgsPackedTuple(const KernelArgsPackedTuple &) = delete;
  KernelArgsPackedTuple &operator=(const KernelArgsPackedTuple &) = delete;

  size_t number_of_arguments() const final {
    return kSize + (shared_memory_bytes_ > 0);
  }

  uint64_t number_of_shared_bytes() const final { return shared_memory_bytes_; }

  absl::Span<const void *const> argument_addresses() const final {
    return absl::Span<const void *const>(argument_addresses_.data(), kSize);
  }

  // Compile time check that KernelArgsPackedTuple is compatible with
  // `OtherArgs`: after stripping const and reference all types match.
  template <typename... OtherArgs>
  static void CheckCompatibleStaticAssert() {
    static constexpr size_t kOtherSize = sizeof...(OtherArgs);
    static_assert(kSize == kOtherSize, "length of arguments packs must match");

    using StrippedArgs = std::tuple<absl::remove_cvref_t<Args>...>;
    using StrippedOtherArgs = std::tuple<absl::remove_cvref_t<OtherArgs>...>;
    static_assert(std::is_same_v<StrippedArgs, StrippedOtherArgs>,
                  "arguments types do not match");
  }

 private:
  template <size_t... Is>
  void InitializeArgumentAddresses(std::index_sequence<Is...>) {
    ((argument_addresses_[Is] = &std::get<Is>(storage_)), ...);
  }

  size_t shared_memory_bytes_ = 0;

  // Storage for packed kernel arguments.
  Storage storage_;

  // Pointers into `storage_`.
  std::array<const void *, kSize> argument_addresses_;
};

// Packs the given arguments into a KernelArgsPackedTuple with compile-time type
// checks.
template <typename... Params, typename... Args>
std::unique_ptr<KernelArgsPackedArrayBase> PackKernelArgs(
    const TypedKernel<Params...> &kernel, const Args &...args) {
  using PackedParams = KernelArgsPackedTuple<Params...>;
  using PackedArgs = KernelArgsPackedTuple<Args...>;

  PackedParams::template CheckCompatibleStaticAssert<Args...>();

  int64_t shmem_bytes = kernel.metadata().shared_memory_bytes().value_or(0);
  return std::make_unique<PackedArgs>(shmem_bytes, args...);
}

}  // namespace stream_executor

#endif  // XLA_STREAM_EXECUTOR_KERNEL_H_
