#pragma once

#include <vector>
#include <string>
#include <memory>
#include <cassert>

class Type {
public:
    enum TypeID {
        IntegerTyID,
        VoidTyID,
        LabelTyID,
        PointerTyID,
        ArrayTyID,
        FunctionTyID
    };

    explicit Type(TypeID tid) : tid_(tid) {}
    virtual ~Type() = default;

    TypeID getTypeID() const { return tid_; }

    bool isIntegerTy() const { return tid_ == IntegerTyID; }
    bool isVoidTy() const { return tid_ == VoidTyID; }
    bool isLabelTy() const { return tid_ == LabelTyID; }
    bool isPointerTy() const { return tid_ == PointerTyID; }
    bool isArrayTy() const { return tid_ == ArrayTyID; }
    bool isFunctionTy() const { return tid_ == FunctionTyID; }

    static Type *getInt32Ty();
    static Type *getVoidTy();
    static Type *getLabelTy();
    static Type *getPointerTy(Type *elementType);
    static Type *getArrayTy(Type *elementType, int numElements);

    // 辅助获取子类型的方法
    Type *getPointerElementType() const;
    Type *getArrayElementType() const;
    int getArrayNumElements() const;
    
    // 打印类型用于调试或IR输出
    virtual std::string toString() const = 0;

private:
    TypeID tid_;
};

class IntegerType : public Type {
public:
    explicit IntegerType(int bitWidth) : Type(IntegerTyID), bitWidth_(bitWidth) {}
    std::string toString() const override { return "i" + std::to_string(bitWidth_); }
private:
    int bitWidth_;
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
    std::string toString() const override { return "*" + elementType_->toString(); } 
private:
    Type *elementType_;
};

class ArrayType : public Type {
public:
    ArrayType(Type *eltTy, int numElts) 
        : Type(ArrayTyID), elementType_(eltTy), numElements_(numElts) {}
    Type *getElementType() const { return elementType_; }
    int getNumElements() const { return numElements_; }
    std::string toString() const override { 
        return "[" + elementType_->toString() + ", " + std::to_string(numElements_) + "]"; 
    }
private:
    Type *elementType_;
    int numElements_;
};

// 实现 Type 的静态方法
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
    // 简单实现：每次new，实际上应该缓存
    return new PointerType(elementType);
}

inline Type *Type::getArrayTy(Type *elementType, int numElements) {
    return new ArrayType(elementType, numElements);
}

inline Type *Type::getPointerElementType() const {
    if (auto *pt = dynamic_cast<const PointerType*>(this))
        return pt->getElementType();
    return nullptr;
}

inline Type *Type::getArrayElementType() const {
    if (auto *at = dynamic_cast<const ArrayType*>(this))
        return at->getElementType();
    return nullptr;
}

inline int Type::getArrayNumElements() const {
    if (auto *at = dynamic_cast<const ArrayType*>(this))
        return at->getNumElements();
    return 0;
}