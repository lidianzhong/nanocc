#pragma once

#include "koopa.h"

#include <memory>
#include <string>
#include <variant>
#include <vector>

enum class Opcode {
  // Arithmetic
  Add,
  Sub,
  Mul,
  Div,
  Mod,
  // Comparison
  Lt,
  Gt,
  Le,
  Ge,
  Eq,
  Ne,
  // Logical
  And,
  Or,
  // Memory
  Alloc, // allocate local variable
  Load,  // load from memory
  Store, // store to memory
  // Control flow
  Br,  // conditional branch
  Jmp, // unconditional jump
  Ret, // return
  // Function
  Call // function call
};

enum class ValueKind {
  Immediate, // immediate value
  Register,  // register
  Address,   // address
};

struct BasicBlock;
struct Function;

struct Value {
  ValueKind kind;
  std::string reg_or_addr; // reg or addr
  int32_t imm;             // imm

  bool isImmediate() const { return kind == ValueKind::Immediate; }
  bool isRegister() const { return kind == ValueKind::Register; }
  bool isAddress() const { return kind == ValueKind::Address; }

  std::string toString() const {
    if (kind == ValueKind::Immediate)
      return std::to_string(imm);
    return reg_or_addr;
  }

  static Value Imm(int32_t imm) { return {ValueKind::Immediate, "", imm}; }
  static Value Reg(std::string reg_or_addr) {
    return {ValueKind::Register, reg_or_addr, 0};
  }
  static Value Addr(std::string reg_or_addr) {
    return {ValueKind::Address, reg_or_addr, 0};
  }
};

struct BranchTarget {
  BasicBlock *target;
  std::vector<Value> args;

  BranchTarget(BasicBlock *bb, std::vector<Value> arguments)
      : target(bb), args(std::move(arguments)) {}
};

using Operand = std::variant<Value, BasicBlock *, BranchTarget>;

struct Instruction {
  Opcode op;
  std::vector<Operand> args;

  template <typename... Args> Instruction(Opcode op, Args &&...args) : op(op) {
    (this->args.emplace_back(std::forward<Args>(args)), ...);
  }
};

struct BasicBlock {
  Function *func;
  std::string name;
  std::vector<std::unique_ptr<Instruction>> insts;
  /* 块参数列表 */
  std::vector<std::pair<std::string, std::string>>
      params; // [(name, type), ...]

  explicit BasicBlock(Function *func, std::string name)
      : func(func), name(name) {}

  bool HasTerminator() const {
    if (insts.empty())
      return false;
    auto op = insts.back()->op;
    return op == Opcode::Br || op == Opcode::Jmp || op == Opcode::Ret;
  }

  /*
   * 添加基本块参数，添加类型为 type 的参数，名称为
   * %<block_name>_arg<index>，返回该名称寄存器的值
   */
  Value AddParam(const std::string &type) {
    std::string param_name =
        "%" + name + "_arg" + std::to_string(params.size());
    params.push_back({param_name, type});
    return Value::Reg(param_name);
  }

  void Append(std::unique_ptr<Instruction> inst) {
    insts.push_back(std::move(inst));
  }

  static BasicBlock *Create(Function *func, std::string name) {
    return new BasicBlock(func, std::move(name));
  }
};

struct Function {
  std::string name;
  std::string ret_type;
  std::vector<std::unique_ptr<BasicBlock>> blocks;

  // 函数的 exit 块（统一返回出口）
  BasicBlock *exit_bb = nullptr;

  Function(std::string name = "", std::string ret_type = "")
      : name(std::move(name)), ret_type(std::move(ret_type)) {}

  BasicBlock *CreateBlock(const std::string &block_name) {
    blocks.push_back(std::make_unique<BasicBlock>(this, block_name));
    return blocks.back().get();
  }
};
