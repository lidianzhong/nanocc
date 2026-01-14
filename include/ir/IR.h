#pragma once

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
  Call,    // function call
  FuncDecl // function declaration
};

enum class ValueKind {
  Immediate, // immediate value
  Register,  // register
  Address,   // address
};

struct symbol_t;
struct BasicBlock;
struct Function;

struct Value {
  ValueKind kind;
  std::string reg_or_addr; // reg or addr
  int32_t imm;             // imm

  bool isImmediate() const { return kind == ValueKind::Immediate; }
  bool isRegister() const { return kind == ValueKind::Register; }
  bool isAddress() const { return kind == ValueKind::Address; }

  std::string toString() const;

  static Value Imm(int32_t imm);
  static Value Reg(std::string reg_or_addr);
  static Value Addr(std::string reg_or_addr);

  static Value Imm(const symbol_t &symbol);
  static Value Addr(const symbol_t &symbol);
};

struct BranchTarget {
  BasicBlock *target;
  std::vector<Value> args;

  BranchTarget(BasicBlock *bb, std::vector<Value> arguments)
      : target(bb), args(std::move(arguments)) {}
};

using Operand = std::variant<Value, BasicBlock *, BranchTarget, std::string,
                             std::vector<Value>, std::vector<std::string>>;

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
  std::vector<std::pair<std::string, std::string>> params;

  explicit BasicBlock(Function *func, std::string name);
  bool HasTerminator() const;
  Value AddParam(const std::string &type);
  void Append(std::unique_ptr<Instruction> inst);
  static BasicBlock *Create(Function *func, std::string name);
};

struct Function {
  std::string name;
  std::string ret_type;
  std::vector<std::pair<std::string, std::string>> params; // Name, Type
  std::vector<std::unique_ptr<BasicBlock>> blocks;

  BasicBlock *exit_bb = nullptr;
  Value ret_addr;
  bool has_return = false;

  Function(std::string name, std::string ret_type = "",
           std::vector<std::pair<std::string, std::string>> params = {});
  BasicBlock *CreateBlock(const std::string &block_name);
};