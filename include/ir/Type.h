#pragma once

#include <cassert>
#include <string>
#include <vector>

namespace ldz {

class Type {
public:
  //===--------------------------------------------------------------------===//
  /// Definitions of all of the base types for the Type system.
  ///
  enum TypeID {
    VoidTyID,     ///< type with no size
    LabelTyID,    ///< Labels
    IntegerTyID,  ///< Arbitrary bit width integers
    FunctionTyID, ///< Functions
    PointerTyID,  ///< Pointers
    ArrayTyID,    ///< Arrays
  };

private:
  TypeID tid_; // The current base type of this type.

protected:
  explicit Type(TypeID tid) : tid_(tid) {}
  virtual ~Type() = default;

public:
  //===--------------------------------------------------------------------===//
  // Accessors for working with types.
  //

  /// Return the type id for the type. This will return one of the TypeID enum
  /// elements defined above.
  TypeID getTypeID() const { return tid_; }

  /// @brief
  bool isVoidTy() const { return getTypeID() == VoidTyID; }

  /// Return true if this is a label.
  bool isLabelTy() const { return getTypeID() == LabelTyID; }

  /// Return true if this is an integer type.
  bool isIntegerTy() const { return getTypeID() == IntegerTyID; }

  /// Return true if this is a function type.
  bool isFunctionTy() const { return getTypeID() == FunctionTyID; }

  /// Return true if this is a pointer type.
  bool isPointerTy() const { return getTypeID() == PointerTyID; }

  /// Return true if this is an array type.
  bool isArrayTy() const { return getTypeID() == ArrayTyID; }

  //===--------------------------------------------------------------------===//
  // Static constructors for the various types.
  //
  static Type *getInt32Ty();
  static Type *getVoidTy();
  static Type *getLabelTy();
  static Type *getPointerTy(Type *elementType);
  static Type *getArrayTy(Type *elementType, int numElements);

  //===--------------------------------------------------------------------===//
  // Specific type queries.
  //
  Type *getPointerElementType() const;
  Type *getArrayElementType() const;
  int getArrayNumElements() const;
  Type *getFunctionReturnType() const;
  const std::vector<Type *> &getFunctionParamTypes() const;
  bool isFunctionVarArg() const;

  //===--------------------------------------------------------------------===//
  // String representation of the type.
  //
  virtual std::string toString() const = 0;
};

class VoidType : public Type {
public:
  VoidType() : Type(VoidTyID) {}
  std::string toString() const override { return "void"; }
};

class LabelType : public Type {
public:
  LabelType() : Type(LabelTyID) {}
  std::string toString() const override { return "label"; }
};

class PointerType : public Type {
public:
  explicit PointerType(Type *eltTy) : Type(PointerTyID), elementType_(eltTy) {}
  Type *getElementType() const { return elementType_; }
  std::string toString() const override {
    return "*" + elementType_->toString();
  }

private:
  Type *elementType_;
};

class IntegerType : public Type {
public:
  explicit IntegerType(int bitWidth) : Type(IntegerTyID), bitWidth_(bitWidth) {}
  std::string toString() const override {
    return "i" + std::to_string(bitWidth_);
  }

private:
  int bitWidth_;
};

class ArrayType : public Type {
public:
  ArrayType(Type *eltTy, int numElts)
      : Type(ArrayTyID), elementType_(eltTy), numElements_(numElts) {}
  Type *getElementType() const { return elementType_; }
  int getNumElements() const { return numElements_; }
  std::string toString() const override {
    return "[" + elementType_->toString() + ", " +
           std::to_string(numElements_) + "]";
  }

private:
  Type *elementType_;
  int numElements_;
};

class FunctionType : public Type {
public:
  FunctionType(Type *retTy, std::vector<Type *> params)
      : Type(FunctionTyID), returnType_(retTy), paramTypes_(params) {}
  Type *getReturnType() const { return returnType_; }
  const std::vector<Type *> &getParamTypes() const { return paramTypes_; }
  std::string toString() const override {
    std::string str = "(";
    for (size_t i = 0; i < paramTypes_.size(); ++i) {
      if (i > 0)
        str += ", ";
      str += paramTypes_[i]->toString();
    }
    str += ") -> " + returnType_->toString();
    return str;
  }

  static FunctionType *get(Type *retTy, std::initializer_list<Type *> params) {
    return new FunctionType(retTy, std::vector<Type *>(params));
  }

  static FunctionType *get(Type *retTy, const std::vector<Type *> &params) {
    return new FunctionType(retTy, params);
  }

private:
  Type *returnType_;
  std::vector<Type *> paramTypes_;
};

inline Type *Type::getInt32Ty() {
  static IntegerType int32Ty(32);
  return &int32Ty;
}

inline Type *Type::getVoidTy() {
  static VoidType voidTy;
  return &voidTy;
}

inline Type *Type::getLabelTy() {
  static LabelType labelTy;
  return &labelTy;
}

inline Type *Type::getPointerTy(Type *elementType) {
  return new PointerType(elementType);
}

inline Type *Type::getArrayTy(Type *elementType, int numElements) {
  return new ArrayType(elementType, numElements);
}

inline Type *Type::getPointerElementType() const {
  if (auto *pt = dynamic_cast<const PointerType *>(this))
    return pt->getElementType();
  return nullptr;
}

inline Type *Type::getArrayElementType() const {
  if (auto *at = dynamic_cast<const ArrayType *>(this))
    return at->getElementType();
  return nullptr;
}

inline int Type::getArrayNumElements() const {
  if (auto *at = dynamic_cast<const ArrayType *>(this))
    return at->getNumElements();
  return 0;
}

inline Type *Type::getFunctionReturnType() const {
  if (auto *ft = dynamic_cast<const FunctionType *>(this))
    return ft->getReturnType();
  return nullptr;
}

inline const std::vector<Type *> &Type::getFunctionParamTypes() const {
  static std::vector<Type *> empty;
  if (auto *ft = dynamic_cast<const FunctionType *>(this))
    return ft->getParamTypes();
  return empty;
}

} // namespace ldz
