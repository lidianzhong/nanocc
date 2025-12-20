#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <variant>

// AST 的基类
class BaseAST {
public:
  virtual ~BaseAST() = default;

  virtual void Dump() const = 0;
};

// FuncType AST
class FuncTypeAST : public BaseAST {
public:
  std::string name;

  void Dump() const override;
};

// CompUnit AST
class CompUnitAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> func_def;

  void Dump() const override;
};

// FuncDef AST
class FuncDefAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> func_type;
  std::string ident;
  std::unique_ptr<BaseAST> block;

  void Dump() const override;
};

// Block AST
class BlockAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> stmt;

  void Dump() const override;
};

// Stmt AST
class StmtAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> exp;

  void Dump() const override;
};

// ExpAST
class ExpAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> unary;

  void Dump() const override;
};

// PrimaryExp AST
class PrimaryExpAST : public BaseAST {
public:
  struct Exp {
    std::unique_ptr<BaseAST> ptr;
  };
  struct Number {
    int32_t val;
  };
  std::variant<Exp, Number> data;

  PrimaryExpAST(std::variant<Exp, Number> &&d) : data(std::move(d)) {}

  void Dump() const override;
};

// UnaryExp AST
class UnaryExpAST : public BaseAST {
public:
  struct Primary {
    std::unique_ptr<BaseAST> ptr;
  };
  struct Unary {
    char op;
    std::unique_ptr<BaseAST> ptr;
  };
  std::variant<Primary, Unary> data;

  UnaryExpAST(std::variant<Primary, Unary> &&d) : data(std::move(d)) {}

  void Dump() const override;
};

// Number AST
class NumberAST : public BaseAST {
public:
  int32_t val;

  void Dump() const override;
};