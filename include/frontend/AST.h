#pragma once

#include "ASTVisitor.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace ldz {

/// AST 基类
class BaseAST {
public:
  virtual ~BaseAST() = default;
  virtual void Accept(ASTVisitor &visitor) = 0;
};

///
/// 程序结构
///

/// 编译单元
class CompUnitAST : public BaseAST {
public:
  std::vector<std::unique_ptr<BaseAST>> items; // func_defs or decls
  void Accept(ASTVisitor &visitor) override;
};

/// 函数参数
class FuncFParamAST : public BaseAST {
public:
  std::string btype; // "int" or "*int"
  std::string ident;
  std::vector<std::unique_ptr<BaseAST>> dims; // 可能为空
  void Accept(ASTVisitor &visitor) override;
};

/// 函数定义
class FuncDefAST : public BaseAST {
public:
  std::string ret_type;
  std::string ident;
  std::vector<std::unique_ptr<FuncFParamAST>> params;
  std::unique_ptr<BlockAST> block;

  void Accept(ASTVisitor &visitor) override;
};

/// 代码块
class BlockAST : public BaseAST {
public:
  std::vector<std::unique_ptr<BaseAST>> items; // Decl or Stmt
  void Accept(ASTVisitor &visitor) override;
};

///
/// 声明相关
///

/// 常量声明
class ConstDeclAST : public BaseAST {
public:
  std::string btype;
  std::vector<std::unique_ptr<BaseAST>> const_defs;
  void Accept(ASTVisitor &visitor) override;
};

/// 常量定义
class ConstDefAST : public BaseAST {
public:
  std::string ident;
  std::vector<std::unique_ptr<BaseAST>> dims; // 可能为空
  std::unique_ptr<InitVarAST> init;           // 可能为空
  bool IsArray() const { return !dims.empty(); }
  void Accept(ASTVisitor &visitor) override;
};

/// 变量声明
class VarDeclAST : public BaseAST {
public:
  std::string btype; // "int"
  std::vector<std::unique_ptr<BaseAST>> var_defs;
  void Accept(ASTVisitor &visitor) override;
};

/// 变量定义
class VarDefAST : public BaseAST {
public:
  std::string ident;
  std::vector<std::unique_ptr<BaseAST>> dims; // 可能为空
  std::unique_ptr<InitVarAST> init;           // 可能为空
  bool IsArray() const { return !dims.empty(); }
  void Accept(ASTVisitor &visitor) override;
};

/// 变量初始化值
class InitVarAST : public BaseAST {
public:
  // exp or init list
  std::unique_ptr<BaseAST> initExpr;                 // 可能为空
  std::vector<std::unique_ptr<InitVarAST>> initList; // 可能为空
  bool IsList() const { return !initList.empty(); }
  void Accept(ASTVisitor &visitor) override;
};

///
/// 语句相关
///

/// 赋值语句: LVal '=' Exp ';'
class AssignStmtAST : public BaseAST {
public:
  std::unique_ptr<LValAST> lval;
  std::unique_ptr<BaseAST> exp;
  void Accept(ASTVisitor &visitor) override;
};

/// 表达式语句: [Exp] ";"
class ExpStmtAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> exp; // 可能为空
  void Accept(ASTVisitor &visitor) override;
};

/// 条件判断语句: "if" "(" Exp ")" Stmt ["else" Stmt]
class IfStmtAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> exp;
  std::unique_ptr<BaseAST> then_stmt;
  std::unique_ptr<BaseAST> else_stmt; // 可能为空
  void Accept(ASTVisitor &visitor) override;
};

/// 循环语句: "while" "(" Exp ")" Stmt
class WhileStmtAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> cond;
  std::unique_ptr<BaseAST> body;
  void Accept(ASTVisitor &visitor) override;
};

/// 跳出循环语句: "break" ";"
class BreakStmtAST : public BaseAST {
public:
  void Accept(ASTVisitor &visitor) override;
};

/// 继续循环语句: "continue" ";"
class ContinueStmtAST : public BaseAST {
public:
  void Accept(ASTVisitor &visitor) override;
};

/// 返回语句: "return [Exp] ";"
class ReturnStmtAST : public BaseAST {
public:
  std::unique_ptr<BaseAST> exp; // 可能为空
  void Accept(ASTVisitor &visitor) override;
};

/// 左值
class LValAST : public BaseAST {
public:
  std::string ident;
  std::vector<std::unique_ptr<BaseAST>> indices; // 可能为空
  void Accept(ASTVisitor &visitor) override;
};

///
/// 表达式相关
///

/// 数字字面量
class NumberAST : public BaseAST {
public:
  int32_t val;
  void Accept(ASTVisitor &visitor) override;
};

/// 一元表达式
class UnaryExpAST : public BaseAST {
public:
  std::string op; // "+", "-", "!"
  std::unique_ptr<BaseAST> exp;
  void Accept(ASTVisitor &visitor) override;
};

/// 二元表达式
class BinaryExpAST : public BaseAST {
public:
  std::string op; // "+", "-", "*", "/", "%", "<", ">", "<=", ">=", "==", "!=",
                  // "&&", "||"
  std::unique_ptr<BaseAST> lhs;
  std::unique_ptr<BaseAST> rhs;
  void Accept(ASTVisitor &visitor) override;
};

/// 函数调用
class FuncCallAST : public BaseAST {
public:
  std::string ident;
  std::vector<std::unique_ptr<BaseAST>> args;
  void Accept(ASTVisitor &visitor) override;
};

inline void CompUnitAST::Accept(ASTVisitor &visitor) { visitor.Visit(*this); }
inline void FuncFParamAST::Accept(ASTVisitor &visitor) { visitor.Visit(*this); }
inline void FuncDefAST::Accept(ASTVisitor &visitor) { visitor.Visit(*this); }
inline void BlockAST::Accept(ASTVisitor &visitor) { visitor.Visit(*this); }
inline void ConstDeclAST::Accept(ASTVisitor &visitor) { visitor.Visit(*this); }
inline void ConstDefAST::Accept(ASTVisitor &visitor) { visitor.Visit(*this); }
inline void VarDeclAST::Accept(ASTVisitor &visitor) { visitor.Visit(*this); }
inline void VarDefAST::Accept(ASTVisitor &visitor) { visitor.Visit(*this); }
inline void InitVarAST::Accept(ASTVisitor &visitor) { visitor.Visit(*this); }
inline void AssignStmtAST::Accept(ASTVisitor &visitor) { visitor.Visit(*this); }
inline void ExpStmtAST::Accept(ASTVisitor &visitor) { visitor.Visit(*this); }
inline void IfStmtAST::Accept(ASTVisitor &visitor) { visitor.Visit(*this); }
inline void WhileStmtAST::Accept(ASTVisitor &visitor) { visitor.Visit(*this); }
inline void BreakStmtAST::Accept(ASTVisitor &visitor) { visitor.Visit(*this); }
inline void ContinueStmtAST::Accept(ASTVisitor &visitor) {
  visitor.Visit(*this);
}
inline void ReturnStmtAST::Accept(ASTVisitor &visitor) { visitor.Visit(*this); }
inline void LValAST::Accept(ASTVisitor &visitor) { visitor.Visit(*this); }
inline void NumberAST::Accept(ASTVisitor &visitor) { visitor.Visit(*this); }
inline void UnaryExpAST::Accept(ASTVisitor &visitor) { visitor.Visit(*this); }
inline void BinaryExpAST::Accept(ASTVisitor &visitor) { visitor.Visit(*this); }
inline void FuncCallAST::Accept(ASTVisitor &visitor) { visitor.Visit(*this); }

} // namespace ldz