#pragma once

#include "ir/Constant.h"

namespace ldz {

class GlobalValue : public Constant {
public:
  // Linkage types, currently defined in Function, could be moved here in future

protected:
  GlobalValue(ValueID id, Type *ty) : Constant(id, ty) {}

public:
  static bool classof(const Value *V) {
    return V->getValueID() == GlobalVariableVal ||
           V->getValueID() == FunctionVal;
  }
};

} // namespace ldz
