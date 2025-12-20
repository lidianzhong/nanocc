#include "ast.h"
#include <iostream>

void Indent(int indent) {
  for (int i = 0; i < indent; ++i) {
    std::cout << "  ";
  }
}

void CompUnitAST::Dump(int indent) const {
  std::cout << "CompUnitAST {\n";
  Indent(indent + 1);
  func_def->Dump(indent + 1);
  std::cout << "\n";
  Indent(indent);
  std::cout << "}";
}

void FuncDefAST::Dump(int indent) const {
  std::cout << "FuncDefAST {\n";
  Indent(indent + 1);
  func_type->Dump(indent + 1);
  std::cout << ",\n";
  Indent(indent + 1);
  std::cout << "Ident: " << ident << ",\n";
  Indent(indent + 1);
  block->Dump(indent + 1);
  std::cout << "\n";
  Indent(indent);
  std::cout << "}";
}

void FuncTypeAST::Dump(int indent) const {
  std::cout << "FuncTypeAST { " << name << " }";
}

void BlockAST::Dump(int indent) const {
  std::cout << "BlockAST {\n";
  Indent(indent + 1);
  stmt->Dump(indent + 1);
  std::cout << "\n";
  Indent(indent);
  std::cout << "}";
}

void StmtAST::Dump(int indent) const {
  std::cout << "StmtAST {\n";
  Indent(indent + 1);
  exp->Dump(indent + 1);
  std::cout << "\n";
  Indent(indent);
  std::cout << "}";
}

void ExpAST::Dump(int indent) const {
  std::cout << "ExpAST {\n";
  Indent(indent + 1);
  lor_exp->Dump(indent + 1);
  std::cout << "\n";
  Indent(indent);
  std::cout << "}";
}

void PrimaryExpAST::Dump(int indent) const {
  std::cout << "PrimaryExpAST { ";
  if (std::holds_alternative<Number>(data)) {
    std::cout << "Number: " << std::get<Number>(data).val;
  } else if (std::holds_alternative<Exp>(data)) {
    std::cout << "Exp: ";
    std::get<Exp>(data).ptr->Dump(indent + 1);
  } else {
    throw std::runtime_error("Invalid PrimaryExpAST variant");
  }
  std::cout << " }";
}

void UnaryExpAST::Dump(int indent) const {
  std::cout << "UnaryExpAST {\n";
  Indent(indent + 1);
  if (std::holds_alternative<Primary>(data)) {
    std::cout << "Primary: ";
    std::get<Primary>(data).ptr->Dump(indent + 1);
  } else if (std::holds_alternative<Unary>(data)) {
    auto &unary = std::get<Unary>(data);
    std::cout << "Op: " << unary.op << ",\n";
    Indent(indent + 1);
    std::cout << "Unary: ";
    unary.ptr->Dump(indent + 1);
  } else {
    throw std::runtime_error("Invalid UnaryExpAST variant");
  }
  std::cout << "\n";
  Indent(indent);
  std::cout << "}";
}

void NumberAST::Dump(int indent) const {
  std::cout << "NumberAST { " << val << " }";
}

void MulExpAST::Dump(int indent) const {
  std::cout << "MulExpAST {\n";
  Indent(indent + 1);
  if (std::holds_alternative<Unary>(data)) {
    std::cout << "Unary: ";
    std::get<Unary>(data).ptr->Dump(indent + 1);
  } else if (std::holds_alternative<Mul>(data)) {
    auto &mul = std::get<Mul>(data);
    std::cout << "Mul: ";
    mul.mul_exp->Dump(indent + 1);
    std::cout << ",\n";
    Indent(indent + 1);
    std::cout << "Op: " << mul.op << ",\n";
    Indent(indent + 1);
    std::cout << "Unary: ";
    mul.unary_exp->Dump(indent + 1);
  } else {
    throw std::runtime_error("Invalid MulExpAST variant");
  }
  std::cout << "\n";
  Indent(indent);
  std::cout << "}";
}

void AddExpAST::Dump(int indent) const {
  std::cout << "AddExpAST {\n";
  Indent(indent + 1);
  if (std::holds_alternative<Mul>(data)) {
    std::cout << "Mul: ";
    std::get<Mul>(data).ptr->Dump(indent + 1);
  } else if (std::holds_alternative<Add>(data)) {
    auto &add = std::get<Add>(data);
    std::cout << "Add: ";
    add.add_exp->Dump(indent + 1);
    std::cout << ",\n";
    Indent(indent + 1);
    std::cout << "Op: " << add.op << ",\n";
    Indent(indent + 1);
    std::cout << "Mul: ";
    add.mul_exp->Dump(indent + 1);
  } else {
    throw std::runtime_error("Invalid AddExpAST variant");
  }
  std::cout << "\n";
  Indent(indent);
  std::cout << "}";
}

void RelExpAST::Dump(int indent) const {
  std::cout << "RelExpAST {\n";
  Indent(indent + 1);
  if (std::holds_alternative<Add>(data)) {
    std::cout << "Add: ";
    std::get<Add>(data).ptr->Dump(indent + 1);
  } else if (std::holds_alternative<Rel>(data)) {
    auto &rel = std::get<Rel>(data);
    std::cout << "Rel: ";
    rel.rel_exp->Dump(indent + 1);
    std::cout << ",\n";
    Indent(indent + 1);
    std::cout << "Op: " << rel.op << ",\n";
    Indent(indent + 1);
    std::cout << "Add: ";
    rel.add_exp->Dump(indent + 1);
  } else {
    throw std::runtime_error("Invalid RelExpAST variant");
  }
  std::cout << "\n";
  Indent(indent);
  std::cout << "}";
}

void EqExpAST::Dump(int indent) const {
  std::cout << "EqExpAST {\n";
  Indent(indent + 1);
  if (std::holds_alternative<Rel>(data)) {
    std::cout << "Rel: ";
    std::get<Rel>(data).ptr->Dump(indent + 1);
  } else if (std::holds_alternative<Eq>(data)) {
    auto &eq = std::get<Eq>(data);
    std::cout << "Eq: ";
    eq.eq_exp->Dump(indent + 1);
    std::cout << ",\n";
    Indent(indent + 1);
    std::cout << "Op: " << eq.op << ",\n";
    Indent(indent + 1);
    std::cout << "Rel: ";
    eq.rel_exp->Dump(indent + 1);
  } else {
    throw std::runtime_error("Invalid EqExpAST variant");
  }
  std::cout << "\n";
  Indent(indent);
  std::cout << "}";
}

void LAndExpAST::Dump(int indent) const {
  std::cout << "LAndExpAST {\n";
  Indent(indent + 1);
  if (std::holds_alternative<Eq>(data)) {
    std::cout << "Eq: ";
    std::get<Eq>(data).ptr->Dump(indent + 1);
  } else if (std::holds_alternative<LAnd>(data)) {
    auto &land = std::get<LAnd>(data);
    std::cout << "LAnd: ";
    land.land_exp->Dump(indent + 1);
    std::cout << ",\n";
    Indent(indent + 1);
    std::cout << "Op: &&,\n";
    Indent(indent + 1);
    std::cout << "Eq: ";
    land.eq_exp->Dump(indent + 1);
  } else {
    throw std::runtime_error("Invalid LAndExpAST variant");
  }
  std::cout << "\n";
  Indent(indent);
  std::cout << "}";
}

void LOrExpAST::Dump(int indent) const {
  std::cout << "LOrExpAST {\n";
  Indent(indent + 1);
  if (std::holds_alternative<LAnd>(data)) {
    std::cout << "LAnd: ";
    std::get<LAnd>(data).ptr->Dump(indent + 1);
  } else if (std::holds_alternative<LOr>(data)) {
    auto &lor = std::get<LOr>(data);
    std::cout << "LOr: ";
    lor.lor_exp->Dump(indent + 1);
    std::cout << ",\n";
    Indent(indent + 1);
    std::cout << "Op: ||,\n";
    Indent(indent + 1);
    std::cout << "LAnd: ";
    lor.land_exp->Dump(indent + 1);
  } else {
    throw std::runtime_error("Invalid LOrExpAST variant");
  }
  std::cout << "\n";
  Indent(indent);
  std::cout << "}";
}
