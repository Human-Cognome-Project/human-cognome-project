#pragma once

#include <string>
#include <memory>
#include <optional>
#include <algorithm>

#include "taichi/rhi/arch.h"
#include "taichi/program/compiled_kernel_borrow.h"

namespace taichi::lang {

class KernelLaunchHandle {
 public:
  void set_launch_id(int id) {
    launch_id_ = id;
  }

  int get_launch_id() const {
    return launch_id_;
  }

 private:
  int launch_id_{-1};
};

class CompiledKernelDataFile {
 public:
  static constexpr char kHeadStr[] = "TIC";
  static constexpr std::size_t kHeadSize = std::size(kHeadStr);
  static constexpr std::size_t kHashSize = 64;
  enum class Err {
    kNoError,
    kNotTicFile,
    kCorruptedFile,
    kOutOfMemory,
    kIOStreamError,
  };

  Err dump(std::ostream &os);
  Err load(std::istream &is);

  CompiledKernelDataFile() {
    std::copy(kHeadStr, kHeadStr + kHeadSize, head_);
  }

  void set_arch(Arch arch) {
    arch_ = arch;
  }

  void set_metadata(std::string metadata) {
    metadata_ = std::move(metadata);
  }

  void set_src_code(std::string src) {
    src_code_ = std::move(src);
  }

  const Arch &arch() const {
    return arch_;
  }

  const std::string &metadata() const {
    return metadata_;
  }

  const std::string &src_code() const {
    return src_code_;
  }

 private:
  bool update_hash();

  char head_[kHeadSize];
  Arch arch_;
  std::string metadata_;
  std::string src_code_;
  std::string hash_;
};

class CompiledKernelData {
 public:
  enum class Err {
    kNoError = 0,
    kNotTicFile,
    kCorruptedFile,
    kParseMetadataFailed,
    kParseSrcCodeFailed,
    kArchNotMatched,
    kSerMetadataFailed,
    kSerSrcCodeFailed,
    kIOStreamError,
    kOutOfMemory,
    kTiWithoutLLVM,
    kTiWithoutSpirv,
    kCompiledKernelDataBroken,
    kUnknown,
  };

  CompiledKernelData() = default;
  CompiledKernelData(const CompiledKernelData &) = delete;
  CompiledKernelData &operator=(const CompiledKernelData &) = delete;
  virtual ~CompiledKernelData() {
    if (borrow_state_) { borrow_state_->data = nullptr; borrow_state_->owner = nullptr; }
  }

  // Non-lazy metadata only. Unmanaged/cloned objects remain unavailable and
  // untracked; raw native clients remain permanently marked on owned records.
  CompiledBorrowObservation borrow_observation() const noexcept {
    return CompiledKernelBorrow::observe(borrow_state_);
  }

  virtual Arch arch() const = 0;

  Err load(std::istream &is);
  Err dump(std::ostream &os) const;

  virtual std::unique_ptr<CompiledKernelData> clone() const = 0;

  virtual Err debug_print(std::ostream &os) const {
    return dump(os);
  }

  virtual Err check() const {
    return Err::kNoError;
  }

  void set_handle(const KernelLaunchHandle &handle) const {
    kernel_launch_handle_ = handle;
  }

  const std::optional<KernelLaunchHandle> &get_handle() const {
    return kernel_launch_handle_;
  }

  static std::unique_ptr<CompiledKernelData> load(std::istream &is, Err *p_err);

  static std::string get_err_msg(Err err);

 protected:
  virtual Err load_impl(const CompiledKernelDataFile &file) = 0;
  virtual Err dump_impl(CompiledKernelDataFile &file) const = 0;

 private:
  friend class KernelCompilationManager;
  const std::shared_ptr<CompiledKernelBorrow::State> &borrow_state(const void *owner) const {
    if (!borrow_state_)
      borrow_state_ = std::make_shared<CompiledKernelBorrow::State>(this, owner);
    if (borrow_state_->owner != owner || borrow_state_->data != this)
      throw std::logic_error("compiled-data borrow owner mismatch");
    return borrow_state_;
  }
  mutable std::shared_ptr<CompiledKernelBorrow::State> borrow_state_;
  using Creator = std::unique_ptr<CompiledKernelData>();
  static Creator *const llvm_creator;
  static Creator *const spriv_creator;

  static std::unique_ptr<CompiledKernelData> create(Arch arch, Err &err);

  mutable std::optional<KernelLaunchHandle> kernel_launch_handle_;
};

}  // namespace taichi::lang
