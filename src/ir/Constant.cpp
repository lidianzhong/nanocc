#include "ir/Constant.h"
#include "ir/Type.h"

namespace nanocc {

Constant *Constant::getNullValue(Type *ty) {
  if (ty->isIntegerTy()) {
    return ConstantInt::get(ty, 0);
  }
  return ConstantZero::get(ty);
}

ConstantArray::ConstantArray(ArrayType *ty, const std::vector<Constant *> &val)
    : Constant(ConstantArrayVal, ty) {
  for (Constant *c : val) {
    addOperand(c);
  }
}

ConstantArray *ConstantArray::get(ArrayType *ty,
                                  const std::vector<Constant *> &val) {
  return new ConstantArray(ty, val);
}

ConstantZero *ConstantZero::get(Type *ty) { return new ConstantZero(ty); }

} // namespace nanocc
