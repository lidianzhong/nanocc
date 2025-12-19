#pragma once

#include <memory>
#include <string>
#include <iostream>

// AST 的基类
class BaseAST {
public:
    virtual ~BaseAST() = default;
    virtual void Dump() const = 0;
    virtual void DumpIR() const = 0;
};

// FuncType AST
class FuncTypeAST : public BaseAST {
public:
    std::string name;
    void Dump() const override;
    void DumpIR() const override {}
};

// CompUnit AST
class CompUnitAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> func_def;
    void Dump() const override;
    void DumpIR() const override;
};

// FuncDef AST
class FuncDefAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> func_type;
    std::string ident;
    std::unique_ptr<BaseAST> block;

    void Dump() const override;
    void DumpIR() const override;
};

// Block AST
class BlockAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> stmt;
    void Dump() const override;
    void DumpIR() const override;
};

// Stmt AST
class StmtAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> number;
    void Dump() const override;
    void DumpIR() const override;
};

// Number AST
class NumberAST : public BaseAST {
public:
    int val;
    void Dump() const override;
    void DumpIR() const override;
};