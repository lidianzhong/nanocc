#pragma once

#include "nanocc/ir/User.h"
#include <memory>

namespace nanocc {

class BasicBlock;

class Instruction : public User {
public:
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

  Instruction(Type *ty, Opcode op, unsigned numOperands,
              BasicBlock *parent = nullptr)
      : User(InstructionVal, ty), op_(op), parent_(parent) {
    allocateOperands(numOperands);
  }

  void setParent(BasicBlock *BB) { parent_ = BB; }
  BasicBlock *getParent() { return parent_; }
  const BasicBlock *getParent() const { return parent_; }

  Opcode getOpcode() const { return op_; }

  static std::unique_ptr<Instruction> create(Type *ty, Opcode op,
                                             unsigned numOperands,
                                             BasicBlock *parent = nullptr) {
    return std::make_unique<Instruction>(ty, op, numOperands, parent);
  }

  static bool classof(const Value *V) {
    return V->getValueID() == InstructionVal;
  }

private:
  Opcode op_;
  BasicBlock *parent_;
};

} // namespace nanocc