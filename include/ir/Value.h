#pragma once

#include "ir/Type.h"
#include "ir/Use.h"

namespace nanocc {

class Type;
class Value;
class Use;
class User;

class Value {
  friend class Use;

public:
  enum ValueID {
    ArgumentVal,
    BasicBlockVal,

    // Constant Range Start
    ConstantIntVal,
    ConstantArrayVal,
    ConstantZeroVal,
    ConstantExprVal,
    GlobalVariableVal,
    FunctionVal,
    // Constant Range End

    InstructionVal,
  };

protected:
  ValueID vID_;
  Type *vTy_;
  Use *useList_ = nullptr;

protected:
  Value(ValueID id, Type *ty) : vID_(id), vTy_(ty) {}

public:
  virtual ~Value() = default;
  Value(const Value &) = delete;

  //===--------------------------------------------------------------------===//
  // Accessors.
  //
  ValueID getValueID() const { return vID_; }
  Type *getType() const { return vTy_; }

  //===--------------------------------------------------------------------===//
  // Methods for managing uses.
  //
  bool use_empty() const { return useList_ == nullptr; }

  using use_iterator = Use *;
  use_iterator use_begin() { return useList_; }
  use_iterator use_end() { return nullptr; }
};

inline void Use::set(Value *V) {
  if (val_)
    removeFromList();
  val_ = V;
  if (V)
    addToList(&V->useList_);
}

inline Value *Use::operator=(Value *RHS) {
  set(RHS);
  return RHS;
}

inline const Use &Use::operator=(const Use &RHS) {
  set(RHS.val_);
  return *this;
}

} // namespace nanocc
