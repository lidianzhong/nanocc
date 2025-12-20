#include "ast.h"

void CompUnitAST::Dump() const {
  std::cout << "CompUnitAST { ";
  func_def->Dump();
  std::cout << " }";
}

void FuncDefAST::Dump() const {
  std::cout << "FuncDefAST { ";
  func_type->Dump();
  std::cout << ", " << ident << ", ";
  block->Dump();
  std::cout << " }";
}

void FuncTypeAST::Dump() const {
  std::cout << "FuncTypeAST { " << name << " }";
}

void BlockAST::Dump() const {
  std::cout << "BlockAST { ";
  stmt->Dump();
  std::cout << " }";
}

void StmtAST::Dump() const {
  std::cout << "StmtAST { ";
  exp->Dump();
  std::cout << " }";
}

void ExpAST::Dump() const {
  std::cout << "ExpAST { ";
  lor_exp->Dump();
  std::cout << " }";
}

void PrimaryExpAST::Dump() const {
  std::cout << "PrimaryExpAST { ";
  if (std::holds_alternative<Number>(data)) {
    std::cout << std::get<Number>(data).val;
  } else if (std::holds_alternative<Exp>(data)) {
    std::get<Exp>(data).ptr->Dump();
  } else {
    throw std::runtime_error("Invalid PrimaryExpAST variant");
  }
  std::cout << " }";
}

void UnaryExpAST::Dump() const {
  std::cout << "UnaryExpAST { ";
  if (std::holds_alternative<Primary>(data)) {
    std::get<Primary>(data).ptr->Dump();
  } else if (std::holds_alternative<Unary>(data)) {
    auto &unary = std::get<Unary>(data);
    std::cout << unary.op << ", ";
    unary.ptr->Dump();
  } else {
    throw std::runtime_error("Invalid UnaryExpAST variant");
  }
  std::cout << " }";
}

void NumberAST::Dump() const { std::cout << "NumberAST { " << val << " }"; }

void MulExpAST::Dump() const {
  std::cout << "MulExpAST { ";
  if (std::holds_alternative<Unary>(data)) {
    std::get<Unary>(data).ptr->Dump();
  } else if (std::holds_alternative<Mul>(data)) {
    auto &mul = std::get<Mul>(data);
    mul.mul_exp->Dump();
    std::cout << ", " << mul.op << ", ";
    mul.unary_exp->Dump();
  } else {
    throw std::runtime_error("Invalid MulExpAST variant");
  }
  std::cout << " }";
}

void AddExpAST::Dump() const {
  std::cout << "AddExpAST { ";
  if (std::holds_alternative<Mul>(data)) {
    std::get<Mul>(data).ptr->Dump();
  } else if (std::holds_alternative<Add>(data)) {
    auto &add = std::get<Add>(data);
    add.add_exp->Dump();
    std::cout << ", " << add.op << ", ";
    add.mul_exp->Dump();
  } else {
    throw std::runtime_error("Invalid AddExpAST variant");
  }
  std::cout << " }";
}

void RelExpAST::Dump() const {
  std::cout << "RelExpAST { ";
  if (std::holds_alternative<Add>(data)) {
    std::get<Add>(data).ptr->Dump();
  } else if (std::holds_alternative<Rel>(data)) {
    auto &rel = std::get<Rel>(data);
    rel.rel_exp->Dump();
    std::cout << ", " << rel.op << ", ";
    rel.add_exp->Dump();
  } else {
    throw std::runtime_error("Invalid RelExpAST variant");
  }
  std::cout << " }";
}

void EqExpAST::Dump() const {
  std::cout << "EqExpAST { ";
  if (std::holds_alternative<Rel>(data)) {
    std::get<Rel>(data).ptr->Dump();
  } else if (std::holds_alternative<Eq>(data)) {
    auto &eq = std::get<Eq>(data);
    eq.eq_exp->Dump();
    std::cout << ", " << eq.op << ", ";
    eq.rel_exp->Dump();
  } else {
    throw std::runtime_error("Invalid EqExpAST variant");
  }
  std::cout << " }";
}

void LAndExpAST::Dump() const {
  std::cout << "LAndExpAST { ";
  if (std::holds_alternative<Eq>(data)) {
    std::get<Eq>(data).ptr->Dump();
  } else if (std::holds_alternative<LAnd>(data)) {
    auto &land = std::get<LAnd>(data);
    land.land_exp->Dump();
    std::cout << ", &&, ";
    land.eq_exp->Dump();
  } else {
    throw std::runtime_error("Invalid LAndExpAST variant");
  }
  std::cout << " }";
}

void LOrExpAST::Dump() const {
  std::cout << "LOrExpAST { ";
  if (std::holds_alternative<LAnd>(data)) {
    std::get<LAnd>(data).ptr->Dump();
  } else if (std::holds_alternative<LOr>(data)) {
    auto &lor = std::get<LOr>(data);
    lor.lor_exp->Dump();
    std::cout << ", ||, ";
    lor.land_exp->Dump();
  } else {
    throw std::runtime_error("Invalid LOrExpAST variant");
  }
  std::cout << " }";
}