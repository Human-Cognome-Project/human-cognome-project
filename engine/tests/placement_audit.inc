bool rebase_mode_condition(const llvm::Value *v, bool truth,
                           const taichi::lang::Kernel &kernel,
                           const llvm::Module &module, int mode) {
  if (const auto *cmp = llvm::dyn_cast<llvm::ICmpInst>(v))
    for (int side = 0; side < 2; ++side)
      if (const auto *constant = llvm::dyn_cast<llvm::ConstantInt>(cmp->getOperand(side)))
        if (constant->getSExtValue() == mode &&
            tick_audit::scalar(cmp->getOperand(!side), kernel, module, 11))
          return cmp->getPredicate() == (truth ? llvm::ICmpInst::ICMP_EQ : llvm::ICmpInst::ICMP_NE);
  if (const auto *op = llvm::dyn_cast<llvm::BinaryOperator>(v))
    if ((truth && op->getOpcode() == llvm::Instruction::And) ||
        (!truth && op->getOpcode() == llvm::Instruction::Or))
      return rebase_mode_condition(op->getOperand(0), truth, kernel, module, mode) ||
             rebase_mode_condition(op->getOperand(1), truth, kernel, module, mode);
  return false;
}
bool rebase_mode_guard(const llvm::Instruction &write, const taichi::lang::Kernel &kernel,
                       const llvm::Module &module, int mode) {
  llvm::DominatorTree dom(*const_cast<llvm::Function *>(write.getFunction()));
  for (const auto &block : *write.getFunction()) {
    if (const auto *dispatch = llvm::dyn_cast<llvm::SwitchInst>(block.getTerminator()))
      if (tick_audit::scalar(dispatch->getCondition(), kernel, module, 11))
        for (const auto &choice : dispatch->cases())
          if (choice.getCaseValue()->getSExtValue() == mode) {
            const auto *destination = choice.getCaseSuccessor();
            bool unique = destination != dispatch->getDefaultDest();
            for (const auto &other : dispatch->cases())
              if (other.getCaseValue()->getSExtValue() != mode && other.getCaseSuccessor() == destination)
                unique = false;
            if (unique && dom.dominates(llvm::BasicBlockEdge(&block, destination), write.getParent())) return true;
          }
    if (const auto *branch = llvm::dyn_cast<llvm::BranchInst>(block.getTerminator()))
      if (branch->isConditional())
        for (int side = 0; side < 2; ++side)
          if (dom.dominates(llvm::BasicBlockEdge(&block, branch->getSuccessor(side)), write.getParent()) &&
              rebase_mode_condition(branch->getCondition(), side == 0, kernel, module, mode))
            return true;
  }
  return false;
}
const std::array<std::set<int>, 15> &placement_writers() {
  static const std::array<std::set<int>, 15> value{
      {{3, 4, 5}, {4, 5}, {0, 3, 4, 5}, {1}, {2},
       {0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14},
       {3, 4, 5}, {1, 2}, {5, 6, 7}, {1, 2, 3, 4, 5, 6, 7},
       {0, 1, 2}, {1, 3, 5, 7, 9, 11}, {0, 3, 4, 5}, {1}, {0, 1, 2, 3, 4, 5, 7}}};
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
  hierarchy_audit::Analyzer arguments(kernel, module, 100);
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
        if (pointer && !roots.origins(pointer).empty())
          require((index == 11 || index == 13) && tick_audit::guarded(
                      instruction, kernel, module, index == 11 ? 12 : 4),
                  "placement kernel mutates native selection without publication");
        // Guard model-array/native-selection writes, as for contact publication.
        // Compiler task closures and runtime lock bookkeeping are not model
        // outputs. Activation call sites are independently guarded below.
        const bool model_write = pointer &&
            (arguments.data_arg(pointer) >= 0 || !roots.origins(pointer).empty());
        if (index == 14 && model_write && arguments.data_arg(pointer) != 7 &&
            !rebase_mode_guard(instruction, kernel, module, 2)) {
          llvm::errs() << "rebase_unguarded,arg=" << arguments.data_arg(pointer) << ",write=" << instruction << '\n';
          for (const auto &block : *instruction.getFunction())
            llvm::errs() << *block.getTerminator() << '\n';
        }
        // Status is private scratch: LLVM may preserve its value with a store
        // outside check mode. Every live coordinate writer still requires mode 2.
        if (index == 14 && model_write && arguments.data_arg(pointer) != 7)
          require(rebase_mode_guard(instruction, kernel, module, 2),
                  "rebase publication mode guard missing");
        if (index == 13 && pointer && !model_write)
          if (const auto *atomic = llvm::dyn_cast<llvm::AtomicRMWInst>(&instruction))
            if (atomic->getOperation() == llvm::AtomicRMWInst::Xchg)
              llvm::errs() << "focal_runtime_bookkeeping,function="
                           << instruction.getFunction()->getName()
                           << ",root=" << *llvm::getUnderlyingObject(pointer) << '\n';
        if (index == 13 && model_write &&
            !tick_audit::guarded(instruction, kernel, module, 4)) {
          llvm::errs() << "focal_unguarded_store,arg=" << arguments.data_arg(pointer)
                       << ",native_origins=" << roots.origins(pointer).size()
                       << ",instruction=" << instruction << '\n';
        }
        if (index == 13 && model_write)
          require(tick_audit::guarded(instruction, kernel, module, 4),
                  "focal publication lacks enable guard");
        if (index == 11 || index == 13)
          if (const auto *call = llvm::dyn_cast<llvm::CallBase>(&instruction))
            if (const auto *callee = call->getCalledFunction())
              if (callee->getName().contains("activate"))
                require(tick_audit::guarded(instruction, kernel, module, index == 11 ? 12 : 4),
                        "admission native activation lacks publication guard");
      }
  if (index == 3)
    tick_audit::copy(
        kernel, module,
        {{7, int(h.prepared_storage().capacity.command_capacity()), 2, 8,
          true}},
        true);
  if (index == 11) {
    const int p = std::max(h.particle_capacity(), 1),
              c = std::max(h.contact_capacity(), 1);
    tick_audit::copy(kernel, module,
        {{2, c, 2, 4, false}, {1, c, 1, 4, false}, {1, c, 1, 4, false},
         {4, p, 2, 4, false}, {2, c, 2, 4, false}, {1, 16, 1, 8, false}}, true);
  }
}
void placement_compiler(engine::Runtime &r, HostPath path) {
  auto l = placement_limits();
  l[Budget::BootstrapCommands] = 128;
  l[Budget::ScratchBytes] = detail::PreparedStorage::required_scratch(
      l, false, NumericalProfile::trial()) + 256 * 1024 * 1024;
  for (int bits : {52, 8}) {
    Fixture f(r, path, {{}}, {{1}}, {1}, {1}, {0}, bits, 52, false, false, true,
              false, 1024, {1}, &l);
    const auto kernels = PlacementAccess::kernels(f.h);
    const auto surfaces = PlacementAccess::surfaces(f.h);
    int controls = 0;
    for (int index = 0; index < 15; ++index) {
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
        bool owned = index == 14 && PlacementAccess::rebase_payload_binding(f.h, writer, pointer) &&
                     ctx.array_bindings[writer].type == taichi::lang::PrimitiveType::f64 &&
                     ctx.array_bindings[writer].rank == 2 &&
                     ctx.array_bindings[writer].elements ==
                         (writer ? kGroupFieldCount * f.h.field_capacity() : kParticleFieldCount * f.h.particle_capacity());
        for (const auto *a : surfaces)
          if (a && pointer ==
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
      if (index == 2 || index == 12 || index == 14)
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
      if (index == 3 || index == 11 || index == 13)
        reject("missing_result_publication_guard", [&](llvm::Module &clone) {
          for (auto &fn : clone)
            for (auto &bb : fn)
              if (auto *branch =
                      llvm::dyn_cast<llvm::BranchInst>(bb.getTerminator()))
                if (branch->isConditional())
                  for (bool truth : {true, false})
                    if (tick_audit::implies_enabled(branch->getCondition(),
                                                    truth, kernel, clone,
                                                    index == 3 ? 2 : index == 11 ? 12 : 4)) {
                      branch->setCondition(llvm::ConstantInt::get(
                          llvm::Type::getInt1Ty(clone.getContext()), truth));
                      return;
                    }
          throw std::runtime_error(
              "missing placement result guard control target");
        });
      if (index == 14)
        for (int mode : {2})
          reject("missing_commit_guard", [&](llvm::Module &clone) {
            for (auto &fn : clone)
              for (auto &bb : fn) {
                if (auto *dispatch = llvm::dyn_cast<llvm::SwitchInst>(bb.getTerminator()))
                  if (tick_audit::scalar(dispatch->getCondition(), kernel, clone, 11))
                    for (const auto &choice : dispatch->cases())
                      if (choice.getCaseValue()->getSExtValue() == mode) {
                        dispatch->setCondition(llvm::ConstantInt::get(dispatch->getCondition()->getType(), mode));
                        return;
                      }
                if (auto *branch = llvm::dyn_cast<llvm::BranchInst>(bb.getTerminator()))
                  if (branch->isConditional())
                    for (bool truth : {true, false})
                      if (rebase_mode_condition(branch->getCondition(), truth, kernel, clone, mode)) {
                        branch->setCondition(llvm::ConstantInt::get(llvm::Type::getInt1Ty(clone.getContext()), truth));
                        return;
                      }
              }
            throw std::runtime_error("missing rebase mode control target");
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
                 << (index == 2 || index == 12 ? "local-only" : "program")
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
                << ",strict_f64=1,owned_writes=1,native_activity_publication="
                << (index == 11 || index == 13) << ","
                   "legal_duplicate=accepted PASS\n";
    }
    std::cout << "placement_compiler,modules=15,unsafe_controls=" << controls
              << ",actual_frozen_bindings=1 PASS\n";
  }
}
