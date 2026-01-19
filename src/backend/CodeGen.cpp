
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>

#include "backend/CodeGen.h"

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
      if (offset < 2048)
        std::cout << "  sw a" << i << ", " << offset << "(sp)" << std::endl;
      else {
        std::cout << "  li t0, " << offset << std::endl;
        std::cout << "  add t0, sp, t0" << std::endl;
        std::cout << "  sw a" << i << ", (t0)" << std::endl;
      }
    } else {
      size_t stack_size = stack_frame_.GetStackSize();
      size_t src_offset = stack_size + (i - 8) * 4;
      if (src_offset < 2048) {
        std::cout << "  lw t0, " << src_offset << "(sp)" << std::endl;
      } else {
        std::cout << "  li t0, " << src_offset << std::endl;
        std::cout << "  add t0, sp, t0" << std::endl;
        std::cout << "  lw t0, (t0)" << std::endl;
      }
      if (offset < 2048) {
        std::cout << "  sw t0, " << offset << "(sp)" << std::endl;
      } else {
        std::cout << "  li t1, " << offset << std::endl;
        std::cout << "  add t1, sp, t1" << std::endl;
        std::cout << "  sw t0, (t1)" << std::endl;
      }
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
    if (ra_offset >= -2048 && ra_offset <= 2047) {
      std::cout << "  sw ra, " << ra_offset << "(sp)" << std::endl;
    } else {
      std::cout << "  li t0, " << ra_offset << std::endl;
      std::cout << "  add t0, sp, t0" << std::endl;
      std::cout << "  sw ra, (t0)" << std::endl;
    }
  }
}

void FunctionCodeGen::EmitEpilogue() {
  std::string label_name = std::string(func_->name).substr(1) + "_epilogue";
  std::cout << label_name << ":" << std::endl;

  int size = static_cast<int>(stack_frame_.GetStackSize());
  if (stack_frame_.HasCall()) {
    int ra_offset = size - 4;
    if (ra_offset >= -2048 && ra_offset <= 2047) {
      std::cout << "  lw ra, " << ra_offset << "(sp)" << std::endl;
    } else {
      std::cout << "  li t0, " << ra_offset << std::endl;
      std::cout << "  add t0, sp, t0" << std::endl;
      std::cout << "  lw ra, (t0)" << std::endl;
    }
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
      switch (kind.data.ret.value->kind.tag) {
      case KOOPA_RVT_INTEGER:
        // 整数常量直接放到 a0
        std::cout << "  li a0, " << kind.data.ret.value->kind.data.integer.value
                  << std::endl;
        break;
      default: {
        // 其他值从栈中加载到 a0
        size_t ret_offset = GetStackOffset(kind.data.ret.value);
        std::cout << "  lw a0, " << ret_offset << "(sp)" << std::endl;
      }
      }
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
    if (binary.lhs->kind.tag == KOOPA_RVT_INTEGER) {
      std::cout << "  li t0, " << binary.lhs->kind.data.integer.value
                << std::endl;
    } else {
      size_t lhs_offset = GetStackOffset(binary.lhs);
      std::cout << "  lw t0, " << lhs_offset << "(sp)" << std::endl;
    }

    // 加载右操作数到 t1
    if (binary.rhs->kind.tag == KOOPA_RVT_INTEGER) {
      std::cout << "  li t1, " << binary.rhs->kind.data.integer.value
                << std::endl;
    } else {
      size_t rhs_offset = GetStackOffset(binary.rhs);
      std::cout << "  lw t1, " << rhs_offset << "(sp)" << std::endl;
    }

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
    std::cout << "  sw t0, " << res_offset << "(sp)" << std::endl;
    break;
  }
  case KOOPA_RVT_ALLOC:
    // 分配指令不生成代码
    break;
  case KOOPA_RVT_LOAD: {
    const auto &load = kind.data.load;
    size_t res_offset = GetStackOffset(value);

    if (load.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
      std::string name = load.src->name;
      assert(name.length() > 0 && name[0] == '@');
      name = name.substr(1);
      std::cout << "  la t0, " << name << std::endl;
      std::cout << "  lw t0, 0(t0)" << std::endl;
    } else {
      size_t src_offset = GetStackOffset(load.src);
      std::cout << "  lw t0, " << src_offset << "(sp)" << std::endl;
    }
    std::cout << "  sw t0, " << res_offset << "(sp)" << std::endl;
    break;
  }
  case KOOPA_RVT_STORE: {
    // "store src_imm/src_offset, dest_offset"
    const auto &store = kind.data.store;
    switch (store.value->kind.tag) {
    case KOOPA_RVT_INTEGER: {
      int32_t src_imm = store.value->kind.data.integer.value;
      std::cout << "  li t0, " << src_imm << std::endl;
      break;
    }
    default:
      // 除了立即数，其余都放在栈上
      size_t src_offset = GetStackOffset(store.value);
      std::cout << "  lw t0, " << src_offset << "(sp)" << std::endl;
      break;
    }

    if (store.dest->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
      std::string name = store.dest->name;
      assert(name.length() > 0 && name[0] == '@');
      name = name.substr(1);
      std::cout << "  la t1, " << name << std::endl;
      std::cout << "  sw t0, 0(t1)" << std::endl;
    } else {
      size_t dest_offset = GetStackOffset(store.dest);
      std::cout << "  sw t0, " << dest_offset << "(sp)" << std::endl;
    }
    break;
  }
  case KOOPA_RVT_BRANCH: {
    const auto &branch = kind.data.branch;
    std::string true_label = std::string(branch.true_bb->name).substr(1);
    std::string false_label = std::string(branch.false_bb->name).substr(1);

    // 加载条件
    if (branch.cond->kind.tag == KOOPA_RVT_INTEGER) {
      std::cout << "  li t0, " << branch.cond->kind.data.integer.value
                << std::endl;
    } else {
      size_t cond_offset = GetStackOffset(branch.cond);
      std::cout << "  lw t0, " << cond_offset << "(sp)" << std::endl;
    }

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
      if (arg->kind.tag == KOOPA_RVT_INTEGER) {
        std::cout << "  li t0, " << arg->kind.data.integer.value << std::endl;
      } else {
        size_t offset = GetStackOffset(arg);
        if (offset < 2048) {
          std::cout << "  lw t0, " << offset << "(sp)" << std::endl;
        } else {
          std::cout << "  li t1, " << offset << std::endl;
          std::cout << "  add t1, sp, t1" << std::endl;
          std::cout << "  lw t0, (t1)" << std::endl;
        }
      }

      if (i < 8) {
        std::cout << "  mv a" << i << ", t0" << std::endl;
      } else {
        int slot_offset = (i - 8) * 4;
        if (slot_offset < 2048) {
          std::cout << "  sw t0, " << slot_offset << "(sp)" << std::endl;
        } else {
          std::cout << "  li t1, " << slot_offset << std::endl;
          std::cout << "  add t1, sp, t1" << std::endl;
          std::cout << "  sw t0, (t1)" << std::endl;
        }
      }
    }

    std::cout << "  call " << (call.callee->name + 1) << std::endl;

    if (value->ty->tag != KOOPA_RTT_UNIT) {
      size_t offset = GetStackOffset(value);
      if (offset < 2048) {
        std::cout << "  sw a0, " << offset << "(sp)" << std::endl;
      } else {
        std::cout << "  li t1, " << offset << std::endl;
        std::cout << "  add t1, sp, t1" << std::endl;
        std::cout << "  sw a0, (t1)" << std::endl;
      }
    }
    break;
  }
  case KOOPA_RVT_GET_ELEM_PTR: {
    const auto &gep = kind.data.get_elem_ptr;
    size_t res_offset = GetStackOffset(value);

    // 加载基地址
    if (gep.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
      std::string name = gep.src->name;
      assert(name.length() > 0 && name[0] == '@');
      name = name.substr(1);
      std::cout << "  la t0, " << name << std::endl;
    } else {
      size_t src_offset = GetStackOffset(gep.src);
      std::cout << "  lw t0, " << src_offset << "(sp)" << std::endl;
    }

    // 计算偏移
    if (gep.index->kind.tag == KOOPA_RVT_INTEGER) {
      int32_t index = gep.index->kind.data.integer.value;
      int32_t offset = index * 4; // 假设元素大小为 4 字节
      std::cout << "  li t1, " << offset << std::endl;
    } else {
      size_t index_offset = GetStackOffset(gep.index);
      std::cout << "  lw t1, " << index_offset << "(sp)" << std::endl;
      std::cout << "  slli t1, t1, 2" << std::endl; // 假设元素大小为 4 字节
    }

    // 计算最终地址
    std::cout << "  add t0, t0, t1" << std::endl;

    // 存储结果地址
    std::cout << "  sw t0, " << res_offset << "(sp)" << std::endl;
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

  // 为函数参数分配空间
  for (size_t i = 0; i < func_->params.len; ++i) {
    koopa_raw_value_t param = (koopa_raw_value_t)func_->params.buffer[i];
    stack_frame_.AllocSlot(param);
  }

  for (size_t i = 0; i < bbs.len; ++i) {
    koopa_raw_basic_block_t bb = (koopa_raw_basic_block_t)bbs.buffer[i];
    koopa_raw_slice_t params = bb->params;
    // 为块参数分配空间
    for (size_t j = 0; j < params.len; ++j) {
      koopa_raw_value_t param = (koopa_raw_value_t)params.buffer[j];
      stack_frame_.AllocSlot(param);
    }
    // 为有返回值的指令分配空间
    koopa_raw_slice_t insts = bb->insts;
    for (size_t j = 0; j < insts.len; ++j) {
      koopa_raw_value_t inst = (koopa_raw_value_t)insts.buffer[j];
      if (inst->ty->tag != KOOPA_RTT_UNIT) {
        stack_frame_.AllocSlot(inst);
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

void FunctionCodeGen::EmitBlockArgs(koopa_raw_basic_block_t bb,
                                    koopa_raw_slice_t args) {
  for (size_t i = 0; i < args.len; ++i) {
    koopa_raw_value_t arg = (koopa_raw_value_t)args.buffer[i];
    koopa_raw_value_t param = (koopa_raw_value_t)bb->params.buffer[i];
    size_t param_offset = GetStackOffset(param);

    if (arg->kind.tag == KOOPA_RVT_INTEGER) {
      std::cout << "  li t1, " << arg->kind.data.integer.value << std::endl;
    } else {
      size_t arg_offset = GetStackOffset(arg);
      std::cout << "  lw t1, " << arg_offset << "(sp)" << std::endl;
    }
    std::cout << "  sw t1, " << param_offset << "(sp)" << std::endl;
  }
}
