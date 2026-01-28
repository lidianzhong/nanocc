#include "ir/IRBuilder.h"
#include "ir/Constant.h"
#include "ir/Instruction.h"
#include "ir/Type.h"
#include "ir/Value.h"
#include <cassert>
#include <utility>

namespace nanocc {

Value *IRBuilder::createBinaryOp(Instruction::Opcode op, Value *lhs,
                                 Value *rhs) {
  if (auto *L = dynamic_cast<ConstantInt *>(lhs)) {
    if (auto *R = dynamic_cast<ConstantInt *>(rhs)) {
      int lval = L->getValue();
      int rval = R->getValue();
      int res = 0;
      switch (op) {
      case Instruction::Opcode::Add:
        res = lval + rval;
        break;
      case Instruction::Opcode::Sub:
        res = lval - rval;
        break;
      case Instruction::Opcode::Mul:
        res = lval * rval;
        break;
      case Instruction::Opcode::Div:
        res = (rval == 0) ? 0 : lval / rval;
        break;
      case Instruction::Opcode::Mod:
        res = (rval == 0) ? 0 : lval % rval;
        break;
      case Instruction::Opcode::Lt:
        res = lval < rval;
        break;
      case Instruction::Opcode::Le:
        res = lval <= rval;
        break;
      case Instruction::Opcode::Gt:
        res = lval > rval;
        break;
      case Instruction::Opcode::Ge:
        res = lval >= rval;
        break;
      case Instruction::Opcode::Eq:
        res = lval == rval;
        break;
      case Instruction::Opcode::Ne:
        res = lval != rval;
        break;
      default:
        assert(false && "Unknown Opcode");
      }
      return ConstantInt::get(Type::getInt32Ty(), res);
    }
  }

  Type *resTy = lhs->getType();
  if (op >= Instruction::Opcode::Lt && op <= Instruction::Opcode::Ne) {
    resTy = Type::getInt32Ty();
  }

  assert(BB_ && "BasicBlock is null when creating instruction!");
  auto inst = Instruction::create(resTy, op, 2, BB_);
  inst->setOperand(0, lhs);
  inst->setOperand(1, rhs);
  return insert(std::move(inst));
}

Instruction *IRBuilder::createAlloca(Type *type, const std::string &var_name) {
  Type *ptrTy = Type::getPointerTy(type);

  auto inst = Instruction::create(ptrTy, Instruction::Opcode::Alloc, 0, BB_);
  return insert(std::move(inst));
}

Instruction *IRBuilder::createLoad(Value *ptr) {
  Type *ptrTy = ptr->getType();
  assert(ptrTy->isPointerTy() && "Load operand must be pointer");
  Type *resTy = ptrTy->getPointerElementType();

  auto inst = Instruction::create(resTy, Instruction::Opcode::Load, 1, BB_);
  inst->setOperand(0, ptr);
  return insert(std::move(inst));
}

Instruction *IRBuilder::createStore(Value *value, Value *ptr) {
  auto inst = Instruction::create(Type::getVoidTy(), Instruction::Opcode::Store,
                                  2, BB_);
  inst->setOperand(0, value);
  inst->setOperand(1, ptr);
  return insert(std::move(inst));
}

Instruction *IRBuilder::createGetPtr(Value *base_ptr, Value *index) {
  Type *ptrTy = base_ptr->getType();
  assert(ptrTy->isPointerTy());
  auto inst = Instruction::create(ptrTy, Instruction::Opcode::GetPtr, 2, BB_);
  inst->setOperand(0, base_ptr);
  inst->setOperand(1, index);
  return insert(std::move(inst));
}

Instruction *IRBuilder::createGetElemPtr(Value *base_ptr, Value *index) {
  Type *ptrTy = base_ptr->getType();
  assert(ptrTy->isPointerTy());

  Type *elemTy = ptrTy->getPointerElementType();
  Type *resTy = nullptr;
  if (elemTy->isArrayTy()) {
    resTy = Type::getPointerTy(elemTy->getArrayElementType());
  } else {
    resTy = ptrTy;
  }

  auto inst =
      Instruction::create(resTy, Instruction::Opcode::GetElemPtr, 2, BB_);
  inst->setOperand(0, base_ptr);
  inst->setOperand(1, index);
  return insert(std::move(inst));
}

Instruction *IRBuilder::createCondBr(Value *cond, BasicBlock *true_bb,
                                     BasicBlock *false_bb) {
  auto inst =
      Instruction::create(Type::getVoidTy(), Instruction::Opcode::Br, 3, BB_);
  inst->setOperand(0, cond);
  inst->setOperand(1, true_bb);
  inst->setOperand(2, false_bb);
  return insert(std::move(inst));
}

Instruction *IRBuilder::createJump(BasicBlock *target_bb) {
  auto inst =
      Instruction::create(Type::getVoidTy(), Instruction::Opcode::Jmp, 1, BB_);
  inst->setOperand(0, target_bb);
  return insert(std::move(inst));
}

Instruction *IRBuilder::createRet(Value *value) {
  auto inst =
      Instruction::create(Type::getVoidTy(), Instruction::Opcode::Ret, 1, BB_);
  inst->setOperand(0, value);
  return insert(std::move(inst));
}

Instruction *IRBuilder::createRetVoid() {
  auto inst =
      Instruction::create(Type::getVoidTy(), Instruction::Opcode::Ret, 0, BB_);
  return insert(std::move(inst));
}

Instruction *IRBuilder::createCall(Value *func, std::vector<Value *> args) {
  Type *retTy = Type::getVoidTy();
  if (auto *ft = dynamic_cast<FunctionType *>(func->getType())) {
    retTy = ft->getReturnType();
  }
  auto inst = Instruction::create(retTy, Instruction::Opcode::Call,
                                  args.size() + 1, BB_);
  inst->setOperand(0, func);
  for (size_t i = 0; i < args.size(); ++i) {
    inst->setOperand(i + 1, args[i]);
  }
  return insert(std::move(inst));
}

} // namespace nanocc
