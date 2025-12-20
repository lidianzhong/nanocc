#include <sstream>
#include <string>

#include "ast.h"
#include "koopa.h"

class IRGenerator {
public:
  void Visit(const BaseAST *ast);
  void Dump() const;
  std::string GetIR() const;
  koopa_raw_program_t GetProgram();

  ~IRGenerator();

private:
  std::stringstream buffer; // 用于存储生成的 IR 字符串
  std::string last_val; // 存储上一个表达式的结果（寄存器名或立即数）
  int temp_reg_id = 0; // 临时寄存器计数

  // Koopa 解析相关资源
  koopa_program_t program = nullptr;
  koopa_raw_program_builder_t builder = nullptr;

  // 分配新的临时寄存器
  std::string NewTempReg();

  void VisitCompUnit(const CompUnitAST *ast);
  void VisitFuncDef(const FuncDefAST *ast);
  void VisitFuncType(const FuncTypeAST *ast);
  void VisitBlock(const BlockAST *ast);
  void VisitStmt(const StmtAST *ast);
  void VisitExp(const ExpAST *ast);
  void VisitPrimaryExp(const PrimaryExpAST *ast);
  void VisitUnaryExp(const UnaryExpAST *ast);
  void VisitNumber(const NumberAST *ast);
};