#pragma once

#include <memory>
#include <string>
#include <iostream>

// AST 的基类
class BaseAST {
public:
    virtual ~BaseAST() = default;

    virtual void Dump() const = 0;
};

// CompUnit AST
class CompUnitAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> func_def;

    void Dump() const override {
        std::cout << "CompUnitAST { ";
        func_def->Dump();
        std::cout << " }";
    }
};

// FuncDef AST
class FuncDefAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> func_type;
    std::string ident;
    std::unique_ptr<BaseAST> block;

    void Dump() const override {
        std::cout << "FuncDefAST { ";
        func_type->Dump();
        std::cout << ", " << ident << ", ";
        block->Dump();
        std::cout << " }";
    }
};

// FuncType AST
class FuncTypeAST : public BaseAST {
public:
    std::string name;

    void Dump() const override {
        std::cout << "FuncTypeAST { " << name << " }";
    }
};

// Block AST
class BlockAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> stmt;

    void Dump() const override {
        std::cout << "BlockAST { ";
        stmt->Dump();
        std::cout << " }";
    }
};

// Stmt AST
class StmtAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> number;

    void Dump() const override {
        std::cout << "StmtAST { ";
        number->Dump();
        std::cout << " }";
    }
};

// Number AST
class NumberAST : public BaseAST {
public:
    int val;

    void Dump() const override {
        std::cout << "NumberAST { " << val << " }";
    }
};