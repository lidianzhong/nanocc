#pragma once

#include <memory>
#include <sstream>
#include <string>

#include "koopa.h"

#include "ASTVisitor.h"
#include "SymbolTable.h"
#include "ir/IRBuilder.h"

class IRGenVisitor : public ASTVisitor {
public:
  explicit IRGenVisitor();

  // 获取 IR 文本（用于 -koopa 模式）
  std::string GetIR() const;

  // 获取 raw program（用于 -riscv 模式）
  koopa_raw_program_t GetProgram() const;

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
  SymbolTable symtab_;
  std::unique_ptr<IRBuilder> builder_;
  std::variant<std::monostate, int32_t, std::string>
      last_val_; // 要不为空，要不为常量值，要不为偏移值

  void VisitCompUnit_(const CompUnitAST *ast);
  void VisitFuncDef_(const FuncDefAST *ast);
  void VisitBlock_(const BlockAST *ast);
  void VisitConstDecl_(const ConstDeclAST *ast);
  void VisitConstDef_(const ConstDefAST *ast);
  void VisitVarDecl_(const VarDeclAST *ast);
  void VisitVarDef_(const VarDefAST *ast);
  void VisitAssignStmt_(const AssignStmtAST *ast);
  void VisitExpStmt_(const ExpStmtAST *ast);
  void VisitReturnStmt_(const ReturnStmtAST *ast);
  void VisitLVal_(const LValAST *ast);
  void VisitNumber_(const NumberAST *ast);
  void VisitUnaryExp_(const UnaryExpAST *ast);
  void VisitBinaryExp_(const BinaryExpAST *ast);
};