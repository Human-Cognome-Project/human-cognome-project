# Engine C++ API reference copies

Verbatim copies from the engine checkout, kept here so the calling
conventions can be read without opening the engine tree. They are reference
material, not part of the build.

| File | Origin | Status |
|---|---|---|
| `ir_builder_test.cpp` | `tests/cpp/ir/ir_builder_test.cpp` | Current. The authority for how a kernel is built, compiled and launched. |
| `llvm_capacity_config_test.cpp` | `tests/cpp/program/llvm_capacity_config_test.cpp` | Current. Shows the startup capacity contract and its rejection cases. |
| `run_snode.cpp` | `cpp_examples/run_snode.cpp` | **Stale.** Compiles, then aborts at run time in `type.cpp get_element_type`. The examples are excluded from the engine build (`TI_BUILD_EXAMPLES=OFF`), so they have drifted from the current API. Read it for the shape of SNode tree construction only. |

## What current usage looks like

Build the body with `IRBuilder`, then:

```cpp
auto kernel = std::make_unique<Kernel>(program, builder.extract_ir(), name);
kernel->insert_ndarray_param(dtype, /*total_dim=*/1);  // or insert_scalar_param
kernel->finalize_params();
kernel->finalize_rets();
const CompiledKernelData &compiled = program.compile_kernel(
    program.compile_config(), program.get_device_caps(), *kernel);
LaunchContextBuilder ctx = kernel->make_launch_context();
ctx.set_arg_ndarray({0}, array);
program.launch_kernel(compiled, ctx);
```

The older `program.current_ast_builder()` and direct `Kernel::operator()`
spellings in the stale examples no longer apply.
