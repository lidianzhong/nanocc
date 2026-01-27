#pragma once

namespace ldz {

class CompUnitAST;
class FuncFParamAST;
class FuncDefAST;
class BlockAST;
class ConstDeclAST;
class ConstDefAST;
class VarDeclAST;
class VarDefAST;
class InitVarAST;
class AssignStmtAST;
class ExpStmtAST;
class IfStmtAST;
class WhileStmtAST;
class BreakStmtAST;
class ContinueStmtAST;
class ReturnStmtAST;
class LValAST;
class NumberAST;
class UnaryExpAST;
class BinaryExpAST;
class FuncCallAST;

class ASTVisitor {
public:
  virtual ~ASTVisitor() = default;

  virtual void Visit(CompUnitAST &node) = 0;
  virtual void Visit(FuncFParamAST &node) = 0;
  virtual void Visit(FuncDefAST &node) = 0;
  virtual void Visit(BlockAST &node) = 0;
  virtual void Visit(ConstDeclAST &node) = 0;
  virtual void Visit(ConstDefAST &node) = 0;
  virtual void Visit(VarDeclAST &node) = 0;
  virtual void Visit(VarDefAST &node) = 0;
  virtual void Visit(InitVarAST &node) = 0;
  virtual void Visit(AssignStmtAST &node) = 0;
  virtual void Visit(ExpStmtAST &node) = 0;
  virtual void Visit(IfStmtAST &node) = 0;
  virtual void Visit(WhileStmtAST &node) = 0;
  virtual void Visit(BreakStmtAST &node) = 0;
  virtual void Visit(ContinueStmtAST &node) = 0;
  virtual void Visit(ReturnStmtAST &node) = 0;
  virtual void Visit(LValAST &node) = 0;
  virtual void Visit(NumberAST &node) = 0;
  virtual void Visit(UnaryExpAST &node) = 0;
  virtual void Visit(BinaryExpAST &node) = 0;
  virtual void Visit(FuncCallAST &node) = 0;
};

} // namespace ldz
