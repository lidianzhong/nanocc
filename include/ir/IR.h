#pragma once

#include "ir/Value.h"

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
  Alloc,            // allocate local variable
  GlobalAlloc,      // allocate global variable
  AllocArray,       // allocate local array
  GlobalAllocArray, // allocate global array
  Load,             // load from memory
  Store,            // store to memory
  GetElemPtr,       // get element pointer
  GetPtr,           // get pointer
  // Control flow
  Br,  // conditional branch
  Jmp, // unconditional jump
  Ret, // return
  // Function
  Call,    // function call
  FuncDecl // function declaration
};

class BasicBlock;
class Function;

struct BranchTarget {
  BasicBlock *target;
  std::vector<Value*> args;

  BranchTarget(BasicBlock *bb, std::vector<Value*> arguments)
      : target(bb), args(std::move(arguments)) {}
};

using Operand = std::variant<Value*, BasicBlock*, BranchTarget, std::string,
                             std::vector<Value*>, std::vector<std::string>>;

// Instruction 现在继承自 ValueObj，因为它有一个结果（可能是 void）
class Instruction : public ValueObj {
public:
  Opcode op;
  std::vector<Operand> args;

  // 构造函数：需要 Type 和 Name (用于 ValueObj)
  // 如果是 Void 指令 (Store, Br)，Type=VoidTy, Name=""
  template <typename... Args>
  Instruction(Type *ty, const std::string &name, Opcode op, Args &&...args) 
      : ValueObj(ty, name), op(op) {
    (this->args.emplace_back(std::forward<Args>(args)), ...);
  }
};

// BasicBlock 作为一个 Value (LabelType)
class BasicBlock : public Value {
public:
  Function *func;
  std::vector<Instruction*> insts; // 指令列表 (所有权在 Module 或此处?)
  // 简化起见，这里只存指针，内存管理另说

  explicit BasicBlock(Function *func, const std::string &name);
  
  // Value 接口
  std::string toString() const override { return name_; }

  // 旧接口适配
  void Append(Instruction *inst) { insts.push_back(inst); }
  
  bool HasTerminator() const;
};

class Function : public Value {
public:
  std::vector<BasicBlock*> blocks;
  
  struct Param {
      std::string name;
      Type *type;
  };
  std::vector<Param> params;

  Function(const std::string &name, Type *retTy) : Value(retTy, name) {}

  void AddParam(const std::string &name, Type *ty) {
      params.push_back({name, ty});
  }

  BasicBlock *CreateBlock(const std::string &name);
  std::string toString() const override { return name_; }
};
