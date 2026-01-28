#pragma once

#include "frontend/ASTVisitor.h"

#include <fstream>
#include <string>

using namespace nanocc;

class DumpVisitor : public ASTVisitor {
public:
  DumpVisitor(const std::string &filename = "hello.ast");
  ~DumpVisitor();

  void Visit(CompUnitAST &node) override;
  void Visit(FuncFParamAST &node) override;
  void Visit(FuncDefAST &node) override;
  void Visit(BlockAST &node) override;
  void Visit(ConstDeclAST &node) override;
  void Visit(ConstDefAST &node) override;
  void Visit(VarDeclAST &node) override;
  void Visit(VarDefAST &node) override;
  void Visit(InitVarAST &node) override;
  void Visit(AssignStmtAST &node) override;
  void Visit(ExpStmtAST &node) override;
  void Visit(IfStmtAST &node) override;
  void Visit(WhileStmtAST &node) override;
  void Visit(BreakStmtAST &node) override;
  void Visit(ContinueStmtAST &node) override;
  void Visit(ReturnStmtAST &node) override;
  void Visit(LValAST &node) override;
  void Visit(NumberAST &node) override;
  void Visit(UnaryExpAST &node) override;
  void Visit(BinaryExpAST &node) override;
  void Visit(FuncCallAST &node) override;

private:
  int indent_level = 0;
  mutable std::ofstream out_file;

  void print_indent() const;
  void print_node(const std::string &name);
};