#include "ir/IR.h"
#include "ir/Type.h"

BasicBlock::BasicBlock(Function *func, const std::string &name)
    : Value(Type::getLabelTy(), name), func(func) {}

bool BasicBlock::HasTerminator() const {
  if (insts.empty())
    return false;
  Instruction *last = insts.back();
  return last->op == Opcode::Br || last->op == Opcode::Jmp || last->op == Opcode::Ret;
}

BasicBlock *Function::CreateBlock(const std::string &name) {
  auto *bb = new BasicBlock(this, name);
  blocks.push_back(bb);
  return bb;
}
