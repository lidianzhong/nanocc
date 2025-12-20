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
  unary->Dump();
  std::cout << " }";
}

void PrimaryExpAST::Dump() const {
  std::cout << "PrimaryExpAST { ";
  if (std::holds_alternative<Number>(data)) {
    std::cout << std::get<Number>(data).val;
  } else {
    std::get<Exp>(data).ptr->Dump();
  }
  std::cout << " }";
}

void UnaryExpAST::Dump() const {
  std::cout << "UnaryExpAST { ";
  if (std::holds_alternative<Primary>(data)) {
    std::get<Primary>(data).ptr->Dump();
  } else {
    auto &unary = std::get<Unary>(data);
    std::cout << unary.op << ", ";
    unary.ptr->Dump();
  }
  std::cout << " }";
}

void NumberAST::Dump() const { std::cout << "NumberAST { " << val << " }"; }
