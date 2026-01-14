#pragma once

#include "frontend/AST.h"
#include "frontend/ASTVisitor.h"
#include "frontend/SymbolTable.h"
#include "ir/IR.h"
#include "ir/IRBuilder.h"
#include "ir/IRModule.h"

#include <memory>
#include <vector>

class IRGenVisitor : public ASTVisitor {
public:
  explicit IRGenVisitor();

  // 获取 IR 模块
  const IRModule &GetModule() const { return *module_; }

  void Visit(CompUnitAST &node) override;
  void Visit(FuncFParamAST &node) override;
  void Visit(FuncDefAST &node) override;
  void Visit(BlockAST &node) override;
  void Visit(ConstDeclAST &node) override;
  void Visit(ConstDefAST &node) override;
  void Visit(VarDeclAST &node) override;
  void Visit(VarDefAST &node) override;
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
  std::unique_ptr<IRModule> module_;
  std::unique_ptr<IRBuilder> builder_;
  std::unique_ptr<SymbolTable> symtab_;

  // 循环相关
  std::vector<BasicBlock *> break_targets_;
  std::vector<BasicBlock *> continue_targets_;

  void VisitCompUnit_(const CompUnitAST *ast);
  void VisitFuncFParam_(const FuncFParamAST *ast);
  void VisitFuncDef_(const FuncDefAST *ast);
  void VisitBlock_(const BlockAST *ast);
  void VisitConstDecl_(const ConstDeclAST *ast);
  void VisitConstDef_(const ConstDefAST *ast);
  void VisitVarDecl_(const VarDeclAST *ast);
  void VisitVarDef_(const VarDefAST *ast);
  void VisitAssignStmt_(const AssignStmtAST *ast);
  void VisitExpStmt_(const ExpStmtAST *ast);
  void VisitIfStmt_(const IfStmtAST *ast);
  void VisitWhileStmt_(const WhileStmtAST *ast);
  void VisitBreakStmt_(const BreakStmtAST *ast);
  void VisitContinueStmt_(const ContinueStmtAST *ast);
  void VisitReturnStmt_(const ReturnStmtAST *ast);

  Value Eval(BaseAST *ast);
  Value EvalLVal(LValAST *ast);
  Value EvalNumber(NumberAST *ast);
  Value EvalUnaryExp(UnaryExpAST *ast);
  Value EvalBinaryExp(BinaryExpAST *ast);
  Value EvalFuncCall(FuncCallAST *ast);

  Value EvalLogicalAnd(BinaryExpAST *ast);
  Value EvalLogicalOr(BinaryExpAST *ast);
};
