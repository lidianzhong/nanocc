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

  virtual void Dump(int indent = 0) const = 0;
};

// FuncType AST
class FuncTypeAST : public BaseAST {
public:
  std::string name;

  void Dump(int indent = 0) const override;
};

// CompUnit AST
class CompUnitAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> func_def;

  void Dump(int indent = 0) const override;
};

// FuncDef AST
class FuncDefAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> func_type;
  std::string ident;
  std::unique_ptr<BaseAST> block;

  void Dump(int indent = 0) const override;
};

// Block AST
class BlockAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> stmt;

  void Dump(int indent = 0) const override;
};

// Stmt AST
class StmtAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> exp;

  void Dump(int indent = 0) const override;
};

// ExpAST
class ExpAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> lor_exp;

  void Dump(int indent = 0) const override;
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

  void Dump(int indent = 0) const override;
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

  void Dump(int indent = 0) const override;
};

// Number AST
class NumberAST : public BaseAST {
public:
  int32_t val;

  void Dump(int indent = 0) const override;
};

// MulExp AST
class MulExpAST : public BaseAST {
public:
  struct Unary {
    std::unique_ptr<BaseAST> ptr;
  };
  struct Mul {
    std::unique_ptr<BaseAST> mul_exp;
    char op;
    std::unique_ptr<BaseAST> unary_exp;
  };
  std::variant<Unary, Mul> data;

  MulExpAST(std::variant<Unary, Mul> &&d) : data(std::move(d)) {}

  void Dump(int indent = 0) const override;
};

// AddExp AST
class AddExpAST : public BaseAST {
public:
  struct Mul {
    std::unique_ptr<BaseAST> ptr;
  };
  struct Add {
    std::unique_ptr<BaseAST> add_exp;
    char op;
    std::unique_ptr<BaseAST> mul_exp;
  };
  std::variant<Mul, Add> data;

  AddExpAST(std::variant<Mul, Add> &&d) : data(std::move(d)) {}

  void Dump(int indent = 0) const override;
};

// RelExp AST
class RelExpAST : public BaseAST {
public:
  struct Add {
    std::unique_ptr<BaseAST> ptr;
  };
  struct Rel {
    std::unique_ptr<BaseAST> rel_exp;
    std::string op;
    std::unique_ptr<BaseAST> add_exp;
  };
  std::variant<Add, Rel> data;

  RelExpAST(std::variant<Add, Rel> &&d) : data(std::move(d)) {}

  void Dump(int indent = 0) const override;
};

// EqExp AST
class EqExpAST : public BaseAST {
public:
  struct Rel {
    std::unique_ptr<BaseAST> ptr;
  };
  struct Eq {
    std::unique_ptr<BaseAST> eq_exp;
    std::string op;
    std::unique_ptr<BaseAST> rel_exp;
  };
  std::variant<Rel, Eq> data;

  EqExpAST(std::variant<Rel, Eq> &&d) : data(std::move(d)) {}
  void Dump(int indent = 0) const override;
};

// LAndExp AST
class LAndExpAST : public BaseAST {
public:
  struct Eq {
    std::unique_ptr<BaseAST> ptr;
  };
  struct LAnd {
    std::unique_ptr<BaseAST> land_exp;
    std::unique_ptr<BaseAST> eq_exp;
  };
  std::variant<Eq, LAnd> data;

  LAndExpAST(std::variant<Eq, LAnd> &&d) : data(std::move(d)) {}
  void Dump(int indent = 0) const override;
};

// LOrExp AST
class LOrExpAST : public BaseAST {
public:
  struct LAnd {
    std::unique_ptr<BaseAST> ptr;
  };
  struct LOr {
    std::unique_ptr<BaseAST> lor_exp;
    std::unique_ptr<BaseAST> land_exp;
  };
  std::variant<LAnd, LOr> data;

  LOrExpAST(std::variant<LAnd, LOr> &&d) : data(std::move(d)) {}
  void Dump(int indent = 0) const override;
};