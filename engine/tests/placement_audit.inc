const std::array<std::set<int>, 4> &placement_writers() {
  static const std::array<std::set<int>, 4> value{
      {{3, 4, 5}, {4, 5}, {0, 3, 4, 5}, {1}}};
  return value;
}
void placement_module_contract(Harness &h,
                               const engine::CompiledKernel &compiled,
                               const llvm::Module &module,
                               int index) {
  const auto &kernel = engine::testing::CompiledKernelAccess::kernel(compiled);
  require(!llvm::verifyModule(module, &llvm::errs()),
          "invalid emitted placement LLVM module");
  const auto sets = std::array<detail::NativeSet, 2>{
      LiveHierarchyAccess::particle_set(h), LiveHierarchyAccess::field_set(h)};
  const auto report =
      tick_audit::writes(kernel, module, placement_writers()[index], sets);
  require(report.writers == placement_writers()[index],
          "placement output coverage differs from ownership schema");
  selection_audit::Analyzer roots(sets);
  for (const auto &fn : module)
    for (const auto &bb : fn)
      for (const auto &instruction : bb) {
        if (const auto *math =
                llvm::dyn_cast<llvm::FPMathOperator>(&instruction))
          require(!math->getFastMathFlags().any(),
                  "placement arithmetic acquired fast/contract flags");
        if (const auto *call = llvm::dyn_cast<llvm::CallBase>(&instruction))
          if (const auto *callee = call->getCalledFunction())
            require(!callee->getName().contains("fmuladd") &&
                        !callee->getName().startswith("llvm.fma."),
                    "placement arithmetic contracted residual operations");
        const llvm::Value *pointer = nullptr;
        if (const auto *store = llvm::dyn_cast<llvm::StoreInst>(&instruction))
          pointer = store->getPointerOperand();
        if (const auto *atomic =
                llvm::dyn_cast<llvm::AtomicRMWInst>(&instruction)) {
          const auto *zero =
              llvm::dyn_cast<llvm::ConstantInt>(atomic->getValOperand());
          if (atomic->getOperation() != llvm::AtomicRMWInst::Or || !zero ||
              !zero->isZero())
            pointer = atomic->getPointerOperand();
        }
        if (pointer)
          require(roots.origins(pointer).empty(),
                  "placement kernel mutates committed native selection");
      }
  if (index == 3)
    tick_audit::copy(
        kernel, module,
        {{7, int(h.prepared_storage().capacity.limits()[Budget::Batch]), 2, 8,
          true}},
        true);
}
void placement_compiler(engine::Runtime &r, HostPath path) {
  const auto l = placement_limits();
  for (int bits : {52, 8}) {
    Fixture f(r, path, {{}}, {{1}}, {1}, {1}, {0}, bits, 52, false, false, true,
              false, 1024, {1}, &l);
    const auto kernels = PlacementAccess::kernels(f.h);
    const auto surfaces = PlacementAccess::surfaces(f.h);
    int controls = 0;
    for (int index = 0; index < 4; ++index) {
      const auto &compiled = *kernels[index];
      const auto &kernel =
          engine::testing::CompiledKernelAccess::kernel(compiled);
      const auto &module =
          *dynamic_cast<const taichi::lang::LLVM::CompiledKernelData &>(
               engine::testing::CompiledKernelAccess::compiled(compiled))
               .get_internal_data()
               .compiled_data.module;
      placement_module_contract(f.h, compiled, module, index);
      const auto &ctx =
          engine::testing::CompiledKernelAccess::context(compiled);
      for (int writer : placement_writers()[index]) {
        const auto pointer = ctx.array_ptrs.at(
            {writer, taichi::lang::TypeFactory::DATA_PTR_POS_IN_NDARRAY});
        bool owned = false;
        for (const auto *a : surfaces)
          if (pointer ==
              reinterpret_cast<void *>(a->get_device_allocation_ptr_as_int()))
            owned = ctx.array_bindings[writer].type == a->dtype &&
                  ctx.array_bindings[writer].rank == int(a->shape.size()) &&
                    ctx.array_bindings[writer].elements == a->get_nelement();
        require(owned, "placement writer lacks an owned frozen typed binding");
      }
      const auto reject = [&](const char *name, auto mutate) {
        auto clone = llvm::CloneModule(module);
        mutate(*clone);
        require(!llvm::verifyModule(*clone, &llvm::errs()),
                "invalid placement compiler control");
        bool refused = false;
        try {
          placement_module_contract(f.h, compiled, *clone, index);
        } catch (const std::runtime_error &) {
          refused = true;
        }
        require(refused, "placement compiler audit accepted unsafe mutation");
        ++controls;
        std::cout << "placement_compiler_negative,index=" << index
                  << ",control=" << name << " rejected PASS\n";
      };
      reject("omitted_output_surface", [&](llvm::Module &clone) {
        hierarchy_audit::Analyzer a(kernel, clone, 100);
        const int target = *placement_writers()[index].begin();
        std::vector<llvm::Instruction *> erase;
        for (auto &fn : clone)
          for (auto &bb : fn)
            for (auto &instruction : bb) {
              if (auto *s = llvm::dyn_cast<llvm::StoreInst>(&instruction))
                if (a.data_arg(s->getPointerOperand()) == target)
                  erase.push_back(s);
              if (auto *s = llvm::dyn_cast<llvm::AtomicRMWInst>(&instruction))
                if (a.data_arg(s->getPointerOperand()) == target) {
                  s->replaceAllUsesWith(
                      llvm::ConstantInt::get(s->getType(), 0));
                  erase.push_back(s);
                }
            }
        require(!erase.empty(), "missing placement omission target");
        for (auto *i : erase)
          i->eraseFromParent();
      });
      if (index == 2)
        reject("contracted_interval_math", [&](llvm::Module &clone) {
          for (auto &fn : clone)
            for (auto &bb : fn)
              for (auto &instruction : bb)
                if (auto *op =
                        llvm::dyn_cast<llvm::BinaryOperator>(&instruction))
                  if (op->getType()->isFloatingPointTy()) {
                    op->setHasAllowContract(true);
                    return;
                  }
          throw std::runtime_error(
              "missing placement interval math control target");
        });
      if (index == 3)
        reject("missing_result_publication_guard", [&](llvm::Module &clone) {
          for (auto &fn : clone)
            for (auto &bb : fn)
              if (auto *branch =
                      llvm::dyn_cast<llvm::BranchInst>(bb.getTerminator()))
                if (branch->isConditional())
                  for (bool truth : {true, false})
                    if (tick_audit::implies_enabled(branch->getCondition(),
                                                    truth, kernel, clone, 2)) {
                      branch->setCondition(llvm::ConstantInt::get(
                          llvm::Type::getInt1Ty(clone.getContext()), truth));
                      return;
                    }
          throw std::runtime_error(
              "missing placement result guard control target");
        });
      // Legal repeated writes do not change dynamic delivery. The audit must
      // not depend on compiler-specific duplication/site totals.
      auto safe = llvm::CloneModule(module);
      hierarchy_audit::Analyzer a(kernel, *safe, 100);
      llvm::StoreInst *duplicate = nullptr;
      for (auto &fn : *safe)
        for (auto &bb : fn)
          for (auto &instruction : bb)
            if (auto *s = llvm::dyn_cast<llvm::StoreInst>(&instruction))
              if (placement_writers()[index].count(
                      a.data_arg(s->getPointerOperand())))
                duplicate = s;
      require(duplicate, "missing safe placement duplicate target");
      duplicate->clone()->insertAfter(duplicate);
      placement_module_contract(f.h, compiled, *safe, index);
      if (const char *directory = std::getenv("HCP_B2_IR_DIR")) {
        std::error_code ec;
        llvm::raw_fd_ostream output(std::string(directory) + "/placement-b" +
                                        std::to_string(bits) + "-" +
                                        std::to_string(index) + ".ll",
                                    ec);
        require(!ec, "placement IR collection output");
        module.print(output, nullptr);
        std::ofstream metadata(std::string(directory) + "/placement-b" +
                               std::to_string(bits) + "-" +
                               std::to_string(index) + ".abi.txt");
        require(bool(metadata), "placement ABI collection output");
        const auto &config = r.program().compile_config();
        metadata << "actual_arch=" << taichi::arch_name(r.arch())
                 << ",kernel=" << kernel.get_name() << ",body_bits=" << bits
                 << ",strict_f64=1,whole_kernel_optimization="
                 << (index == 2 ? "local-only" : "program")
                 << ",llvm_opt=" << config.external_optimization_level
                 << ",cpu_threads=" << r.cpu_threads()
                 << ",prepared_bytes=" << compiled.prepared_bytes() << '\n';
        for (std::size_t arg = 0; arg < kernel.parameter_list.size(); ++arg) {
          metadata << "argument=" << arg << ",type="
                   << kernel.parameter_list[arg].get_dtype()->to_string();
          if (kernel.parameter_list[arg].is_array)
            metadata
                << ",data_offset="
                << kernel.args_type->get_element_offset(
                       {int(arg),
                        taichi::lang::TypeFactory::DATA_PTR_POS_IN_NDARRAY})
                << ",rank=" << ctx.array_bindings[arg].rank
                << ",elements=" << ctx.array_bindings[arg].elements;
          else
            metadata << ",scalar_offset="
                     << kernel.args_type->get_element_offset({int(arg)});
          metadata << '\n';
        }
      }
      std::cout << "placement_compiler,index=" << index
                << ",strict_f64=1,owned_writes=1,native_presence_read_only=1,"
                   "legal_duplicate=accepted PASS\n";
    }
    std::cout << "placement_compiler,modules=4,unsafe_controls=" << controls
              << ",actual_frozen_bindings=1 PASS\n";
  }
}
