#pragma once

#include "ir/BasicBlock.h"
#include "ir/Instruction.h"
#include "ir/Module.h"
#include "ir/Value.h"
#include <memory>
#include <utility>

namespace ldz {

class IRBuilder {
private:
  Module *module_;
  BasicBlock *BB_;

public:
  IRBuilder(Module *module) : module_(module) {}
  ~IRBuilder() = default;

  void setInsertPoint(BasicBlock *BB) { BB_ = BB; }
  BasicBlock *getInsertBlock() const { return BB_; }

  /// Insert instruction into current basic block
  Instruction *insert(std::unique_ptr<Instruction> inst) {
    if (BB_)
      BB_->getInstList().push_back(std::move(inst));
    return BB_->getInstList().back().get();
  }

  //===--------------------------------------------------------------------===//
  // Create instructions for Arithmetic & Logical Operations
  //

  /// Create unary operation instruction
  /// @deprecated use `createBinaryOp` with constant 0 instead
  Instruction *createUnaryOp(Instruction::Opcode op, Value *value);

  /// Create binary operation instruction
  Value *createBinaryOp(Instruction::Opcode op, Value *lhs, Value *rhs);

  //===--------------------------------------------------------------------===//
  // Create instructions for Memory Operations
  //

  /// Allocate a local variable
  /// @todo Need consider array allocation and global variable allocation
  Instruction *createAlloca(Type *type, const std::string &var_name = "");

  /// load value from memory
  Instruction *createLoad(Value *ptr);

  /// store value to memory
  Instruction *createStore(Value *value, Value *ptr);

  Instruction *createGetPtr(Value *base_ptr, Value *index);

  Instruction *createGetElemPtr(Value *base_ptr, Value *index);

  //===--------------------------------------------------------------------===//
  // Create instructions for Control Flow
  //

  /// Create conditional branch instruction
  Instruction *createCondBr(Value *cond, BasicBlock *true_bb,
                            BasicBlock *false_bb);

  /// Create unconditional jump instruction
  Instruction *createJump(BasicBlock *target_bb);

  /// Create return instruction with value
  Instruction *createRet(Value *value);

  /// Create return instruction without value
  Instruction *createRetVoid();

  /// Create function call instruction
  Instruction *createCall(Value *func, std::vector<Value *> args);
};

} // namespace ldz