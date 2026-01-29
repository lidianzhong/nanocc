
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>

#include "nanocc/backend/CodeGen.h"

#include "koopa.h"

size_t ProgramCodeGen::CalcTypeSize(koopa_raw_type_t ty) {
  switch (ty->tag) {
  case KOOPA_RTT_INT32:
  case KOOPA_RTT_POINTER:
    return 4;
  case KOOPA_RTT_UNIT:
    return 0;
  case KOOPA_RTT_ARRAY:
    return ty->data.array.len * CalcTypeSize(ty->data.array.base);
  default:
    std::cerr << "Unsupported type in global initializer: " << ty->tag
              << std::endl;
    assert(false);
    return 0;
  }
}

void ProgramCodeGen::EmitInitializer(const koopa_raw_value_t &init) {
  switch (init->kind.tag) {
  case KOOPA_RVT_ZERO_INIT: {
    size_t size = CalcTypeSize(init->ty);
    std::cout << "  .zero " << size << std::endl;
    break;
  }
  case KOOPA_RVT_INTEGER:
    std::cout << "  .word " << init->kind.data.integer.value << std::endl;
    break;
  case KOOPA_RVT_AGGREGATE: {
    const auto &agg = init->kind.data.aggregate;
    for (size_t i = 0; i < agg.elems.len; ++i) {
      auto elem = reinterpret_cast<koopa_raw_value_t>(agg.elems.buffer[i]);
      EmitInitializer(elem);
    }
    break;
  }
  default:
    std::cerr << "Unsupported global initializer tag: " << init->kind.tag
              << std::endl;
    assert(false);
  }
}

void ProgramCodeGen::EmitGlobalAlloc(const koopa_raw_value_t &value) {
  assert(value->kind.tag == KOOPA_RVT_GLOBAL_ALLOC);

  std::string name = value->name;
  assert(name.length() > 0 && name[0] == '@');
  name = name.substr(1);

  std::cout << "  .globl " << name << std::endl;
  std::cout << name << ":" << std::endl;
  EmitInitializer(value->kind.data.global_alloc.init);
}

void ProgramCodeGen::EmitDataSection(const koopa_raw_slice_t &values) {
  if (values.len == 0)
    return;

  std::cout << "  .data" << std::endl;
  for (size_t i = 0; i < values.len; ++i) {
    auto val = reinterpret_cast<koopa_raw_value_t>(values.buffer[i]);
    EmitGlobalAlloc(val);
  }
  std::cout << std::endl;
}

void ProgramCodeGen::Emit(const koopa_raw_program_t &program) {
  EmitDataSection(program.values);
  EmitTextSection();

  const koopa_raw_slice_t &funcs = program.funcs;
  for (size_t i = 0; i < funcs.len; ++i) {
    koopa_raw_function_t func =
        reinterpret_cast<koopa_raw_function_t>(funcs.buffer[i]);
    FunctionCodeGen func_gen;
    func_gen.EmitFunction(func);
  }
}

void ProgramCodeGen::EmitTextSection() { std::cout << "  .text" << std::endl; }

void FunctionCodeGen::EmitFunction(const koopa_raw_function_t &func) {
  if (func->bbs.len == 0) {
    // 函数声明，无需生成代码
    return;
  }

  func_ = func;

  std::string name = func_->name;
  assert(name.length() > 0 && name[0] == '@');

  name = name.substr(1);
  std::cout << "  .globl " << name << std::endl;
  std::cout << name << ":" << std::endl;

  AllocateStackSpace();
  EmitPrologue();

  // 使用 func_->params 来处理函数参数，确保与 AllocateStackSpace 一致
  for (size_t i = 0; i < func_->params.len; ++i) {
    koopa_raw_value_t param = (koopa_raw_value_t)func_->params.buffer[i];
    size_t offset = GetStackOffset(param);
    if (i < 8) {
      SafeStore("a" + std::to_string(i), offset);
    } else {
      size_t stack_size = stack_frame_.GetStackSize();
      size_t src_offset = stack_size + (i - 8) * 4;
      SafeLoad("t0", src_offset);
      SafeStore("t0", offset);
    }
  }

  EmitSlice(func_->bbs);

  EmitEpilogue();
}

void FunctionCodeGen::EmitSlice(const koopa_raw_slice_t &slice) {
  for (size_t i = 0; i < slice.len; ++i) {
    auto ptr = slice.buffer[i];
    switch (slice.kind) {
    case KOOPA_RSIK_FUNCTION:
      EmitFunction(reinterpret_cast<koopa_raw_function_t>(ptr));
      break;
    case KOOPA_RSIK_BASIC_BLOCK:
      EmitBasicBlock(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
      break;
    case KOOPA_RSIK_VALUE:
      EmitValue(reinterpret_cast<koopa_raw_value_t>(ptr));
      break;
    default:
      assert(false);
      break;
    }
  }
}

void FunctionCodeGen::EmitPrologue() {
  int size = static_cast<int>(stack_frame_.GetStackSize());
  if (size >= -2048 && size <= 2047) {
    std::cout << "  addi sp, sp, " << -size << std::endl;
  } else {
    std::cout << "  li t0, " << -size << std::endl;
    std::cout << "  add sp, sp, t0" << std::endl;
  }

  if (stack_frame_.HasCall()) {
    int ra_offset = size - 4;
    SafeStore("ra", ra_offset);
  }
}

void FunctionCodeGen::EmitEpilogue() {
  std::string label_name = std::string(func_->name).substr(1) + "_epilogue";
  std::cout << label_name << ":" << std::endl;

  int size = static_cast<int>(stack_frame_.GetStackSize());
  if (stack_frame_.HasCall()) {
    int ra_offset = size - 4;
    SafeLoad("ra", ra_offset);
  }

  if (size >= -2048 && size <= 2047) {
    std::cout << "  addi sp, sp, " << size << std::endl;
  } else {
    std::cout << "  li t0, " << size << std::endl;
    std::cout << "  add sp, sp, t0" << std::endl;
  }

  std::cout << "  ret" << std::endl;
}

void FunctionCodeGen::EmitBasicBlock(const koopa_raw_basic_block_t &bb) {
  std::string label_name = bb->name;
  label_name = label_name.substr(1);
  // 函数入口的 Block 不需要再添加 label 了，已经有 main 入口了
  if (label_name != "entry")
    std::cout << label_name << ':' << std::endl;
  EmitSlice(bb->insts);
}

void FunctionCodeGen::EmitValue(const koopa_raw_value_t &value) {
  const auto &kind = value->kind;
  switch (kind.tag) {
  case KOOPA_RVT_RETURN: {
    if (kind.data.ret.value) {
      LoadReg("a0", kind.data.ret.value);
    }

    std::string label_name = std::string(func_->name).substr(1) + "_epilogue";
    std::cout << "  j " << label_name << std::endl;

    break;
  }
  case KOOPA_RVT_INTEGER:
    // 整数常量不生成代码
    break;
  case KOOPA_RVT_BINARY: {
    const auto &binary = kind.data.binary;
    size_t res_offset = GetStackOffset(value);

    // 加载左操作数到 t0
    LoadReg("t0", binary.lhs);

    // 加载右操作数到 t1
    LoadReg("t1", binary.rhs);

    switch (binary.op) {
    case KOOPA_RBO_NOT_EQ:
      std::cout << "  sub t0, t0, t1" << std::endl;
      std::cout << "  snez t0, t0" << std::endl;
      break;
    case KOOPA_RBO_EQ:
      std::cout << "  sub t0, t0, t1" << std::endl;
      std::cout << "  seqz t0, t0" << std::endl;
      break;
    case KOOPA_RBO_GT:
      std::cout << "  sgt t0, t0, t1" << std::endl;
      break;
    case KOOPA_RBO_LT:
      std::cout << "  slt t0, t0, t1" << std::endl;
      break;
    case KOOPA_RBO_GE:
      std::cout << "  slt t0, t0, t1" << std::endl;
      std::cout << "  xori t0, t0, 1" << std::endl;
      break;
    case KOOPA_RBO_LE:
      std::cout << "  sgt t0, t0, t1" << std::endl;
      std::cout << "  xori t0, t0, 1" << std::endl;
      break;
    case KOOPA_RBO_ADD:
      std::cout << "  add t0, t0, t1" << std::endl;
      break;
    case KOOPA_RBO_SUB:
      std::cout << "  sub t0, t0, t1" << std::endl;
      break;
    case KOOPA_RBO_MUL:
      std::cout << "  mul t0, t0, t1" << std::endl;
      break;
    case KOOPA_RBO_DIV:
      std::cout << "  div t0, t0, t1" << std::endl;
      break;
    case KOOPA_RBO_MOD:
      std::cout << "  rem t0, t0, t1" << std::endl;
      break;
    case KOOPA_RBO_AND:
      std::cout << "  and t0, t0, t1" << std::endl;
      break;
    case KOOPA_RBO_OR:
      std::cout << "  or t0, t0, t1" << std::endl;
      break;
    case KOOPA_RBO_XOR:
      std::cout << "  xor t0, t0, t1" << std::endl;
      break;
    case KOOPA_RBO_SHL:
      std::cout << "  sll t0, t0, t1" << std::endl;
      break;
    case KOOPA_RBO_SHR:
      std::cout << "  srl t0, t0, t1" << std::endl;
      break;
    case KOOPA_RBO_SAR:
      std::cout << "  sra t0, t0, t1" << std::endl;
      break;
    default:
      std::cerr << "Unsupported binary operation: " << binary.op << std::endl;
      assert(false);
    }

    // 将结果存回栈
    SafeStore("t0", res_offset);
    break;
  }
  case KOOPA_RVT_ALLOC:
    // 分配指令不生成代码
    break;
  case KOOPA_RVT_LOAD: {
    const auto &load = kind.data.load;
    size_t res_offset = GetStackOffset(value);

    LoadReg("t0", load.src);
    std::cout << "  lw t0, 0(t0)" << std::endl;

    SafeStore("t0", res_offset);
    break;
  }
  case KOOPA_RVT_STORE: {
    const auto &store = kind.data.store;
    LoadReg("t0", store.value);
    LoadReg("t1", store.dest);
    std::cout << "  sw t0, 0(t1)" << std::endl;
    break;
  }
  case KOOPA_RVT_BRANCH: {
    const auto &branch = kind.data.branch;
    std::string true_label = std::string(branch.true_bb->name).substr(1);
    std::string false_label = std::string(branch.false_bb->name).substr(1);

    // 加载条件
    LoadReg("t0", branch.cond);

    // 条件为 false 时跳到 false 分支
    std::cout << "  beqz t0, " << false_label << "_args" << std::endl;

    // 条件为 true：传递 true_args 并跳转
    EmitBlockArgs(branch.true_bb, branch.true_args);
    std::cout << "  j " << true_label << std::endl;

    // false 分支的参数传递
    std::cout << false_label << "_args:" << std::endl;
    EmitBlockArgs(branch.false_bb, branch.false_args);
    std::cout << "  j " << false_label << std::endl;
    break;
  }
  case KOOPA_RVT_JUMP: {
    const auto &jump = kind.data.jump;
    std::string jump_label = std::string(jump.target->name).substr(1);
    EmitBlockArgs(jump.target, jump.args);
    std::cout << "  j " << jump_label << std::endl;
    break;
  }

  case KOOPA_RVT_CALL: {
    const auto &call = kind.data.call;
    for (size_t i = 0; i < call.args.len; ++i) {
      koopa_raw_value_t arg = (koopa_raw_value_t)call.args.buffer[i];
      LoadReg("t0", arg);

      if (i < 8) {
        std::cout << "  mv a" << i << ", t0" << std::endl;
      } else {
        int slot_offset = (i - 8) * 4;
        SafeStore("t0", slot_offset);
      }
    }

    std::cout << "  call " << (call.callee->name + 1) << std::endl;

    if (value->ty->tag != KOOPA_RTT_UNIT) {
      size_t offset = GetStackOffset(value);
      SafeStore("a0", offset);
    }
    break;
  }
  case KOOPA_RVT_GET_PTR: {
    const auto &gp = kind.data.get_ptr;
    size_t res_offset = GetStackOffset(value);

    // 加载基地址
    LoadReg("t0", gp.src);

    // 计算偏移
    if (gp.index->kind.tag == KOOPA_RVT_INTEGER) {
      // 保持优化，不通过 LoadReg + slli
      int32_t index = gp.index->kind.data.integer.value;

      // Calculate type size of pointee
      size_t type_size =
          ProgramCodeGen::CalcTypeSize(gp.src->ty->data.pointer.base);
      int32_t offset = index * type_size;
      std::cout << "  li t1, " << offset << std::endl;
    } else {
      LoadReg("t1", gp.index);

      size_t type_size =
          ProgramCodeGen::CalcTypeSize(gp.src->ty->data.pointer.base);
      // For size 4 use shift, otherwise mul. 简单起见，这里总是 mul
      // 或者假设为 4. 之前代码假设为 4. 但对于 GetPtr, src is pointer.
      // gp.src->ty is pointer to T. GetPtr result is pointer to T. Offset is
      // index * sizeof(T). 之前假定 4 字节. 对于 int* 来说是正确的.
      // 对于数组指针比如 int(*)[10], sizeof(T) = 40.

      if (type_size == 4) {
        std::cout << "  slli t1, t1, 2" << std::endl;
      } else {
        std::cout << "  li t2, " << type_size << std::endl;
        std::cout << "  mul t1, t1, t2" << std::endl;
      }
    }

    // 计算最终地址
    std::cout << "  add t0, t0, t1" << std::endl;

    // 存储结果地址
    SafeStore("t0", res_offset);
    break;
  }

  case KOOPA_RVT_GET_ELEM_PTR: {
    const auto &gep = kind.data.get_elem_ptr;
    size_t res_offset = GetStackOffset(value);

    // 加载基地址
    LoadReg("t0", gep.src);

    // 计算偏移
    auto src_ty = gep.src->ty;
    // src_ty 是 pointer to array/struct/etc.
    // getelemptr drills down.
    // T = src_ty->base.
    // Ensure T is array?
    // According to Koopa spec: GEP on pointer to array/struct.
    // Result is pointer to Element.
    // Element size = sizeof(T.element).

    // Example: [4096 x i32]*. GEP 0. Element type is i32, size 4.
    // Wait. GEP on *Array.
    // The size of element is type size of (Array's element).
    // src_ty->data.pointer.base must be ARRAY.
    assert(src_ty->tag == KOOPA_RTT_POINTER);
    auto base_ty = src_ty->data.pointer.base;
    assert(base_ty->tag == KOOPA_RTT_ARRAY);

    size_t elem_size = ProgramCodeGen::CalcTypeSize(base_ty->data.array.base);

    if (gep.index->kind.tag == KOOPA_RVT_INTEGER) {
      int32_t index = gep.index->kind.data.integer.value;
      int32_t offset = index * elem_size;
      std::cout << "  li t1, " << offset << std::endl;
    } else {
      LoadReg("t1", gep.index);
      if (elem_size == 4) {
        std::cout << "  slli t1, t1, 2" << std::endl;
      } else {
        std::cout << "  li t2, " << elem_size << std::endl;
        std::cout << "  mul t1, t1, t2" << std::endl;
      }
    }

    // 计算最终地址
    std::cout << "  add t0, t0, t1" << std::endl;

    // 存储结果地址
    SafeStore("t0", res_offset);
    break;
  }

  default:
    std::cerr << "Error: Unsupported value kind: " << kind.tag << std::endl;
    assert(false);
    break;
  }
  std::cout << std::endl;
}

/**
  * 分配栈帧空间
  1. ra 寄存器
  2. 局部变量 (包括块参数和有返回值的指令)
  3. 传参空间
*/
void FunctionCodeGen::AllocateStackSpace() {
  int max_args = 0;
  bool has_call = false;

  koopa_raw_slice_t bbs = func_->bbs;
  for (size_t i = 0; i < bbs.len; ++i) {
    koopa_raw_basic_block_t bb = (koopa_raw_basic_block_t)bbs.buffer[i];
    koopa_raw_slice_t insts = bb->insts;
    for (size_t j = 0; j < insts.len; ++j) {
      koopa_raw_value_t inst = (koopa_raw_value_t)insts.buffer[j];
      if (inst->kind.tag == KOOPA_RVT_CALL) {
        has_call = true;
        int args_len = inst->kind.data.call.args.len;
        if (args_len > max_args)
          max_args = args_len;
      }
    }
  }

  stack_frame_.Init(max_args, has_call);

  // 为函数参数分配空间 (参数通常是 i32 或指针，大小为 4)
  for (size_t i = 0; i < func_->params.len; ++i) {
    koopa_raw_value_t param = (koopa_raw_value_t)func_->params.buffer[i];
    stack_frame_.AllocSlot(param, 4);
  }

  for (size_t i = 0; i < bbs.len; ++i) {
    koopa_raw_basic_block_t bb = (koopa_raw_basic_block_t)bbs.buffer[i];
    koopa_raw_slice_t params = bb->params;
    // 为块参数分配空间
    for (size_t j = 0; j < params.len; ++j) {
      koopa_raw_value_t param = (koopa_raw_value_t)params.buffer[j];
      stack_frame_.AllocSlot(param, 4);
    }
    // 为有返回值的指令分配空间
    koopa_raw_slice_t insts = bb->insts;
    for (size_t j = 0; j < insts.len; ++j) {
      koopa_raw_value_t inst = (koopa_raw_value_t)insts.buffer[j];
      if (inst->ty->tag != KOOPA_RTT_UNIT) {
        if (inst->kind.tag == KOOPA_RVT_ALLOC) {
          // Alloc 指令不仅返回指针，还要在栈上分配它所指向类型的大小
          size_t size =
              ProgramCodeGen::CalcTypeSize(inst->ty->data.pointer.base);
          stack_frame_.AllocSlot(inst, size);
        } else {
          stack_frame_.AllocSlot(inst, 4);
        }
      }
    }
  }
  stack_frame_.Finalize();
}

size_t FunctionCodeGen::GetStackOffset(koopa_raw_value_t val) {
  // 不应为立即数分配栈空间
  assert(val->kind.tag != KOOPA_RVT_INTEGER);
  return stack_frame_.GetOffset(val);
}

void FunctionCodeGen::LoadReg(const std::string &reg, koopa_raw_value_t val) {
  if (val->kind.tag == KOOPA_RVT_INTEGER) {
    std::cout << "  li " << reg << ", " << val->kind.data.integer.value
              << std::endl;
    return;
  }

  if (val->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
    std::string name = val->name;
    name = name.substr(1);
    std::cout << "  la " << reg << ", " << name << std::endl;
    return;
  }

  size_t offset = GetStackOffset(val);

  // 如果是 Alloc 指令，Offset 是分配的空间的基址，值就是 Address (sp + offset)
  if (val->kind.tag == KOOPA_RVT_ALLOC) {
    if (static_cast<int32_t>(offset) >= -2048 &&
        static_cast<int32_t>(offset) <= 2047) {
      std::cout << "  addi " << reg << ", sp, " << offset << std::endl;
    } else {
      std::cout << "  li " << "t6" << ", " << offset << std::endl;
      std::cout << "  add " << reg << ", sp, " << "t6" << std::endl;
    }
  } else {
    // 其他情况（参数、临时变量），栈槽里存的是值，需要 Load
    SafeLoad(reg, offset);
  }
}

void FunctionCodeGen::EmitBlockArgs(koopa_raw_basic_block_t bb,
                                    koopa_raw_slice_t args) {
  for (size_t i = 0; i < args.len; ++i) {
    koopa_raw_value_t arg = (koopa_raw_value_t)args.buffer[i];
    koopa_raw_value_t param = (koopa_raw_value_t)bb->params.buffer[i];
    size_t param_offset = GetStackOffset(param);

    LoadReg("t1", arg);
    SafeStore("t1", param_offset);
  }
}

void FunctionCodeGen::SafeLoad(const std::string &reg, int offset) {
  if (offset >= -2048 && offset <= 2047) {
    std::cout << "  lw " << reg << ", " << offset << "(sp)" << std::endl;
  } else {
    std::cout << "  li t6, " << offset << std::endl;
    std::cout << "  add t6, sp, t6" << std::endl;
    std::cout << "  lw " << reg << ", (t6)" << std::endl;
  }
}

void FunctionCodeGen::SafeStore(const std::string &reg, int offset) {
  if (offset >= -2048 && offset <= 2047) {
    std::cout << "  sw " << reg << ", " << offset << "(sp)" << std::endl;
  } else {
    std::cout << "  li t6, " << offset << std::endl;
    std::cout << "  add t6, sp, t6" << std::endl;
    std::cout << "  sw " << reg << ", (t6)" << std::endl;
  }
}
