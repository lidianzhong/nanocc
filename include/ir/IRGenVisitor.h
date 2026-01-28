#pragma once

#include "frontend/AST.h"
#include "frontend/ASTVisitor.h"
#include "ir/Constant.h"
#include <unordered_set>
#include <vector>

namespace nanocc {

class BaseAST;
class BasicBlock;
class Constant;
class GlobalVariable;
class Module;
class IRBuilder;
class Type;
class ValueSymbolTable;
class Value;

class IRGenVisitor : public ASTVisitor {
public:
  explicit IRGenVisitor(Module &module);

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

private:
  [[deprecated("FORBIDDEN: Visit(LValAST) is not allowed in IRGenVisitor")]]
  void Visit(LValAST &node) override;

  [[deprecated("FORBIDDEN: Visit(LValAST) is not allowed in IRGenVisitor")]]
  void Visit(NumberAST &node) override;

  [[deprecated("FORBIDDEN: Visit(LValAST) is not allowed in IRGenVisitor")]]
  void Visit(UnaryExpAST &node) override;

  [[deprecated("FORBIDDEN: Visit(LValAST) is not allowed in IRGenVisitor")]]
  void Visit(BinaryExpAST &node) override;

  [[deprecated("FORBIDDEN: Visit(LValAST) is not allowed in IRGenVisitor")]]
  void Visit(FuncCallAST &node) override;

private:
  /// @todo Currently, IRGenVisitor holds its lifecycle.
  /// whether use unique_ptr and consider ownership
  Module &module_;

  /// @todo Currently, IRGenVisitor holds its lifecycle.
  /// whether use unique_ptr and consider ownership
  IRBuilder *builder_;

  /// @todo Currently, IRGenVisitor holds its lifecycle.
  /// whether use unique_ptr and consider ownership
  ValueSymbolTable *nameValues_;

private:
  /// Loop related targets
  /// @todo Consider not using member variables to hold these states
  std::vector<BasicBlock *> breakTargets_;

  /// @todo Consider not using member variables to hold these states
  std::vector<BasicBlock *> continueTargets_;

  // Track constant global variables for folding
  std::unordered_set<GlobalVariable *> constGlobals_;

private:
  void visitCompUnit_(const CompUnitAST *ast);
  void visitFuncDef_(const FuncDefAST *ast);
  void visitBlock_(const BlockAST *ast);
  void visitConstDecl_(const ConstDeclAST *ast);
  void visitConstDef_(const ConstDefAST *ast);
  void visitVarDecl_(const VarDeclAST *ast);
  void visitVarDef_(const VarDefAST *ast);
  void visitAssignStmt_(const AssignStmtAST *ast);
  void visitExpStmt_(const ExpStmtAST *ast);
  void visitIfStmt_(const IfStmtAST *ast);
  void visitWhileStmt_(const WhileStmtAST *ast);
  void visitBreakStmt_(const BreakStmtAST *ast);
  void visitContinueStmt_(const ContinueStmtAST *ast);
  void visitReturnStmt_(const ReturnStmtAST *ast);

  Value *evalRVal(BaseAST *ast);
  Value *evalLVal(LValAST *ast);
  ConstantInt *evalNumber(NumberAST *ast);
  Value *evalUnaryExp(UnaryExpAST *ast);
  Value *evalBinaryExp(BinaryExpAST *ast);
  Value *evalFuncCall(FuncCallAST *ast);
  Value *evalLogicalAnd(BinaryExpAST *ast, Value *lhsVal = nullptr);
  Value *evalLogicalOr(BinaryExpAST *ast, Value *lhsVal = nullptr);

  /// Register library functions into the module and symbol table
  void registerLibFunctions();

  /// Helper for flatten const expr evaluation
  int evalConstExpr(const BaseAST *ast);

  /// Helper for initializing global arrays
  Constant *initializeGlobalArray(const InitVarAST *init, Type *ty);

  /// Helper for initializing local arrays
  void initializeLocalArray(const InitVarAST *init, Value *baseAddr,
                            Type *type);

  Constant *evalConstant(const InitVarAST *init, Type *ty);
};

} // namespace nanocc