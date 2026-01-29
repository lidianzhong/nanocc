#pragma once

#include "nanocc/ir/Value.h"
#include <vector>

namespace nanocc {

class User : public Value {
public:
  virtual ~User() {
    for (auto &U : operands_) {
      delete U;
    }
  }

  Value *getOperand(unsigned i) const { return operands_[i]->get(); }

  void setOperand(unsigned i, Value *V) { operands_[i]->set(V); }

  unsigned getNumOperands() const { return operands_.size(); }

  void addOperand(Value *V) {
    auto U = new Use(this);
    U->set(V);
    operands_.push_back(U);
  }

protected:
  User(ValueID id, Type *ty) : Value(id, ty) {}
  std::vector<Use *> operands_;

  void allocateOperands(unsigned n) {
    for (unsigned i = 0; i < n; i++) {
      operands_.push_back(new Use(this));
    }
  }
};

} // namespace nanocc