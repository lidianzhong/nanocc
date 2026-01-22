#pragma once

#include "ir/Type.h"
#include <string>
#include <iostream>

class Value {
public:
    explicit Value(Type *ty, const std::string &name = "") 
        : type_(ty), name_(name) {}
    virtual ~Value() = default;

    Type *getType() const { return type_; }
    
    const std::string &getName() const { return name_; }
    void setName(const std::string &name) { name_ = name; }

    // 返回 Value 的字符串表示 (例如 "10" 或 "%0")
    virtual std::string toString() const = 0; 

protected:
    Type *type_;
    std::string name_;
};

// ConstantInt: 编译期整数常量
class ConstantInt : public Value {
public:
    explicit ConstantInt(int val) 
        : Value(Type::getInt32Ty(), ""), value_(val) {}

    int getValue() const { return value_; }
    
    std::string toString() const override {
        return std::to_string(value_);
    }

    static ConstantInt *get(int val) {
        return new ConstantInt(val);
    }

private:
    int value_;
};

// ValueObj: 运行时变量 (指令结果，alloc地址等)
// 它们的 toString() 通常直接返回名字
class ValueObj : public Value {
public:
    ValueObj(Type *ty, const std::string &name) : Value(ty, name) {}
    
    std::string toString() const override {
        return name_;
    }
};