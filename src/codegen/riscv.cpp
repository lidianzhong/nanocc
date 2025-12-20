#include <cassert>
#include <iostream>
#include <map>
#include <string>

#include "codegen/riscv.h"

// 寄存器分配状态
// 使用 map 记录每个 Value 对应的寄存器
static std::map<koopa_raw_value_t, std::string> val_regs;
// 当前分配到的临时寄存器索引 (t0-t6)
static int cur_reg_idx = 0;

// 分配一个新的临时寄存器
std::string AllocReg() {
  std::string reg = "t" + std::to_string(cur_reg_idx);
  cur_reg_idx = (cur_reg_idx + 1) % 7; // 简单循环分配 t0-t6
  return reg;
}

// 获取值所在的寄存器
// 如果是整数常量，则分配寄存器并加载
// 如果是已计算的值，则返回其寄存器
std::string GetReg(koopa_raw_value_t val) {
  if (val->kind.tag == KOOPA_RVT_INTEGER) {
    // 整数常量 0 直接使用x0寄存器
    int32_t int_val = val->kind.data.integer.value;
    if (int_val == 0) {
      return "x0";
    }
    std::string reg = AllocReg();
    std::cout << "  li " << reg << ", " << int_val << std::endl;
    return reg;
  }
  if (val_regs.find(val) != val_regs.end()) {
    return val_regs[val];
  }
  // 应该不会发生，除非 IR 有问题或未处理的类型
  std::cerr << "Error: Value not found in registers!" << std::endl;
  // assert(false);
  return "t0"; // Fallback
}

void Visit(const koopa_raw_program_t &program) {
  // 声明 .text 段
  std::cout << "  .text" << std::endl;

  // 访问所有全局变量
  Visit(program.values);

  // 访问所有函数
  Visit(program.funcs);
}

void Visit(const koopa_raw_slice_t &slice) {
  for (size_t i = 0; i < slice.len; ++i) {
    auto ptr = slice.buffer[i];
    switch (slice.kind) {
    case KOOPA_RSIK_FUNCTION:
      Visit(reinterpret_cast<koopa_raw_function_t>(ptr));
      break;
    case KOOPA_RSIK_BASIC_BLOCK:
      Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
      break;
    case KOOPA_RSIK_VALUE:
      Visit(reinterpret_cast<koopa_raw_value_t>(ptr));
      break;
    default:
      assert(false);
    }
  }
}

void Visit(const koopa_raw_function_t &func) {
  // 清除寄存器状态
  val_regs.clear();
  cur_reg_idx = 0;

  // 声明全局符号
  std::string name = func->name;
  if (name.length() > 0 && name[0] == '@') {
    name = name.substr(1);
  }

  std::cout << "  .globl " << name << std::endl;
  std::cout << name << ":" << std::endl;

  // 访问函数的基本块
  Visit(func->bbs);
}

void Visit(const koopa_raw_basic_block_t &bb) { Visit(bb->insts); }

void Visit(const koopa_raw_value_t &value) {
  const auto &kind = value->kind;
  switch (kind.tag) {
  case KOOPA_RVT_RETURN: {
    koopa_raw_value_t ret_val = kind.data.ret.value;
    if (ret_val) {
      std::string reg = GetReg(ret_val);
      std::cout << "  mv a0, " << reg << std::endl;
    }
    std::cout << "  ret" << std::endl;
    break;
  }
  case KOOPA_RVT_INTEGER:
    // 整数作为操作数在 GetReg 中处理，作为指令出现时忽略
    break;
  case KOOPA_RVT_BINARY: {
    const auto &binary = kind.data.binary;
    std::string lhs = GetReg(binary.lhs);
    std::string rhs = GetReg(binary.rhs);

    std::string rd = "";

    // 如果有一个操作数是非零常量，结果寄存器可以直接使用另一个操作数的寄存器
    if (binary.rhs->kind.tag == KOOPA_RVT_INTEGER &&
        binary.rhs->kind.data.integer.value != 0) {
      rd = rhs;
    } else if (binary.lhs->kind.tag == KOOPA_RVT_INTEGER &&
               binary.lhs->kind.data.integer.value != 0) {
      rd = lhs;
    }

    if (rd.empty()) {
      rd = AllocReg();
    }

    val_regs[value] = rd;

    switch (binary.op) {
    case KOOPA_RBO_NOT_EQ:
      std::cout << "  xor " << rd << ", " << lhs << ", " << rhs << std::endl;
      std::cout << "  snez " << rd << ", " << rd << std::endl;
      break;
    case KOOPA_RBO_EQ:
      std::cout << "  xor " << rd << ", " << lhs << ", " << rhs << std::endl;
      std::cout << "  seqz " << rd << ", " << rd << std::endl;
      break;

    case KOOPA_RBO_GT:
      std::cout << "  slt " << rd << ", " << rhs << ", " << lhs << std::endl;
      break;

    case KOOPA_RBO_LT:
      std::cout << "  slt " << rd << ", " << lhs << ", " << rhs << std::endl;
      break;
    case KOOPA_RBO_GE:
      std::cout << "  slt " << rd << ", " << lhs << ", " << rhs << std::endl;
      std::cout << "  seqz " << rd << ", " << rd << std::endl;
      break;
    case KOOPA_RBO_LE:
      std::cout << "  slt " << rd << ", " << rhs << ", " << lhs << std::endl;
      std::cout << "  seqz " << rd << ", " << rd << std::endl;
      break;
    case KOOPA_RBO_ADD: {
      std::cout << "  add " << rd << ", " << lhs << ", " << rhs << std::endl;
      break;
    }
    case KOOPA_RBO_SUB: {
      std::cout << "  sub " << rd << ", " << lhs << ", " << rhs << std::endl;
      break;
    }
    case KOOPA_RBO_MUL: {
      std::cout << "  mul " << rd << ", " << lhs << ", " << rhs << std::endl;
      break;
    }
    case KOOPA_RBO_DIV:
      std::cout << "  div " << rd << ", " << lhs << ", " << rhs << std::endl;
      break;
    case KOOPA_RBO_MOD:
      std::cout << "  rem " << rd << ", " << lhs << ", " << rhs << std::endl;
      break;
    case KOOPA_RBO_AND:
      std::cout << "  and " << rd << ", " << lhs << ", " << rhs << std::endl;
      break;
    case KOOPA_RBO_OR:
      std::cout << "  or " << rd << ", " << lhs << ", " << rhs << std::endl;
      break;

    default:
      std::cerr << "Unsupported binary operation: " << binary.op << std::endl;
      assert(false);
    }
    break;
  }
  default:
    // 其他类型暂时忽略
    break;
  }
}

void Visit(const koopa_raw_integer_t &integer) {
  // Do nothing
}

void Visit(const koopa_raw_binary_t &binary) {
  // Do nothing
}
