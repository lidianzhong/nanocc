#pragma once

#include "ir/User.h"

namespace ldz {

class Constant : public User {
protected:
  Constant(ValueID vID, Type *ty) : User(vID, ty) {}

public:
  static bool classof(const Value *V) {
    return V->getValueID() >= ConstantIntVal &&
           V->getValueID() <= ConstantZeroVal;
  }

  static Constant *getNullValue(Type *ty);
};

class ConstantInt : public Constant {
private:
  int value_;
  ConstantInt(Type *ty, int value)
      : Constant(ConstantIntVal, ty), value_(value) {}

public:
  static ConstantInt *get(Type *ty, int value) {
    return new ConstantInt(ty, value);
  }

  int getValue() const { return value_; }

  static bool classof(const Value *V) {
    return V->getValueID() == ConstantIntVal;
  }
};

class ArrayType;

class ConstantArray : public Constant {
private:
  ConstantArray(ArrayType *ty, const std::vector<Constant *> &val);

public:
  static ConstantArray *get(ArrayType *ty, const std::vector<Constant *> &val);

  static bool classof(const Value *V) {
    return V->getValueID() == ConstantArrayVal;
  }
};

class ConstantAggregate : public Constant {
  // Helper for aggregate constants like arrays or structs
public:
  static Constant *getNullValue(Type *ty) { return Constant::getNullValue(ty); }
};

class ConstantZero : public Constant {
private:
  explicit ConstantZero(Type *ty) : Constant(ConstantZeroVal, ty) {}

public:
  static ConstantZero *get(Type *ty);

  static bool classof(const Value *V) {
    return V->getValueID() == ConstantZeroVal;
  }
};

} // namespace ldz
