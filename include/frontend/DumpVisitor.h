#pragma once

#include "frontend/ASTVisitor.h"
#include <iostream>
#include <string>

class DumpVisitor : public ASTVisitor {
public:
  DumpVisitor() = default;

  void Visit(CompUnitAST &node) override;
  void Visit(FuncDefAST &node) override;
  void Visit(BlockAST &node) override;
  void Visit(ConstDeclAST &node) override;
  void Visit(ConstDefAST &node) override;
  void Visit(VarDeclAST &node) override;
  void Visit(VarDefAST &node) override;
  void Visit(AssignStmtAST &node) override;
  void Visit(ExpStmtAST &node) override;
  void Visit(ReturnStmtAST &node) override;
  void Visit(LValAST &node) override;
  void Visit(NumberAST &node) override;
  void Visit(UnaryExpAST &node) override;
  void Visit(BinaryExpAST &node) override;

private:
  int indent_level = 0;

  void print_indent() const;
  void print_node(const std::string &name);
};