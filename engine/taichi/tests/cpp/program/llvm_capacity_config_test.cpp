#include "gtest/gtest.h"
#include "taichi/program/compile_config.h"

#include <cstdlib>
#include <string>

namespace taichi::lang {
namespace {
class ScopedEnvironment {
 public:
  explicit ScopedEnvironment(const char *name) : name_(name) {
    if (const char *value = std::getenv(name)) {
      present_ = true;
      value_ = value;
    }
  }
  ~ScopedEnvironment() {
    set(present_ ? value_.c_str() : nullptr);
  }
  void set(const char *value) {
#ifdef _WIN32
    _putenv_s(name_, value ? value : "");
#else
    if (value)
      setenv(name_, value, 1);
    else
      unsetenv(name_);
#endif
  }

 private:
  const char *name_;
  bool present_{false};
  std::string value_;
};
}  // namespace

TEST(LlvmCapacityConfig, NativeLoaderEnvironment) {
  ScopedEnvironment nodes("TI_LLVM_SNODE_CAPACITY");
  ScopedEnvironment trees("TI_LLVM_SNODE_TREE_CAPACITY");
  nodes.set("4096");
  trees.set("768");
  CompileConfig config;
  // Construction must not interfere with Python's explicit init overrides.
  EXPECT_EQ(config.llvm_snode_capacity, 1024);
  EXPECT_EQ(config.llvm_snode_tree_capacity, 512);
  config.apply_llvm_runtime_environment();
  EXPECT_EQ(config.llvm_snode_capacity, 4096);
  EXPECT_EQ(config.llvm_snode_tree_capacity, 768);
  nodes.set("");
  trees.set(nullptr);
  config.apply_llvm_runtime_environment();
  EXPECT_EQ(config.llvm_snode_capacity, 4096);
  EXPECT_EQ(config.llvm_snode_tree_capacity, 768);
  for (const char *invalid :
       {"0", "-1", " 2", "2 ", "2x", "1.5", "99999999999999999999999999999"}) {
    nodes.set("8192");
    trees.set(invalid);
    EXPECT_ANY_THROW(config.apply_llvm_runtime_environment());
    EXPECT_EQ(config.llvm_snode_capacity, 4096);
    EXPECT_EQ(config.llvm_snode_tree_capacity, 768);
  }
}
}  // namespace taichi::lang
