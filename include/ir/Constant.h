#pragma once

#include "ir/User.h"

namespace nanocc {

/// Base class for all constant values.
/// see GlobalValue (Function, GlobalVariable) in GlobalValue.h
class Constant : public User {
protected:
  Constant(ValueID vID, Type *ty) : User(vID, ty) {}

public:
  /// Check if a Value is a Constant
  static bool classof(const Value *V) {
    return V->getValueID() >= ConstantIntVal && V->getValueID() <= FunctionVal;
  }

  /// Get the null value for a given type.
  /// for integer types, it's zero; for other types, it returns ConstantZero.
  static Constant *getNullValue(Type *ty);
};

/// Aggregate constants like arrays
class ConstantAggregate : public Constant {
public:
  static Constant *getNullValue(Type *ty) { return Constant::getNullValue(ty); }
};

/// Constant array class
class ConstantArray : public Constant {
private:
  ConstantArray(ArrayType *ty, const std::vector<Constant *> &val);

public:
  /// construct a ConstantArray by given ArrayType and elements
  /// @param ty ArrayType of the constant array
  /// @param val vector of Constant elements
  static ConstantArray *get(ArrayType *ty, const std::vector<Constant *> &val);

  /// Check if a Value is a ConstantArray
  static bool classof(const Value *V) {
    return V->getValueID() == ConstantArrayVal;
  }
};

/// Constant expression class
class ConstantExpr : public Constant {
  /// @todo Add expression handling
public:
  /// Check if a Value is a ConstantExpr
  static bool classof(const Value *V) {
    return V->getValueID() == ConstantExprVal;
  }
};

/// Constant integer class
class ConstantInt : public Constant {
private:
  int value_;
  ConstantInt(Type *ty, int value)
      : Constant(ConstantIntVal, ty), value_(value) {}

public:
  /// construct a ConstantInt with given type and integer value
  /// @param ty Type of the constant integer
  /// @param value integer value
  static ConstantInt *get(Type *ty, int value) {
    return new ConstantInt(ty, value);
  }

  /// Get the integer value
  int getValue() const { return value_; }

  /// Check if a Value is a ConstantInt
  static bool classof(const Value *V) {
    return V->getValueID() == ConstantIntVal;
  }
};

/// Constant zero class for non-integer types
class ConstantZero : public Constant {
private:
  explicit ConstantZero(Type *ty) : Constant(ConstantZeroVal, ty) {}

public:
  /// construct a ConstantZero with given type
  /// @param ty Type of the constant zero
  static ConstantZero *get(Type *ty);

  /// Check if a Value is a ConstantZero
  static bool classof(const Value *V) {
    return V->getValueID() == ConstantZeroVal;
  }
};

} // namespace nanocc
