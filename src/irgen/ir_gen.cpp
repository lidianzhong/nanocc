#include "irgen/ir_gen.h"
#include <cassert>
#include <iostream>

IRGenerator::~IRGenerator() {
  if (builder) {
    koopa_delete_raw_program_builder(builder);
  }
  if (program) {
    koopa_delete_program(program);
  }
}

void IRGenerator::Dump() const { std::cout << buffer.str(); }

std::string IRGenerator::GetIR() const { return buffer.str(); }

koopa_raw_program_t IRGenerator::GetProgram() {
  if (builder) {
    koopa_delete_raw_program_builder(builder);
    builder = nullptr;
  }
  if (program) {
    koopa_delete_program(program);
    program = nullptr;
  }

  std::string ir = buffer.str();
  // 解析 IR 字符串
  koopa_error_code_t ret = koopa_parse_from_string(ir.c_str(), &program);
  assert(ret == KOOPA_EC_SUCCESS);

  // 创建 raw program builder
  builder = koopa_new_raw_program_builder();
  // 构建 raw program
  return koopa_build_raw_program(builder, program);
}

std::string IRGenerator::NewTempReg() {
  return "%" + std::to_string(temp_reg_id++);
}

void IRGenerator::Visit(const BaseAST *ast) {
  if (auto compUnit = dynamic_cast<const CompUnitAST *>(ast)) {
    VisitCompUnit(compUnit);
  } else if (auto funcDef = dynamic_cast<const FuncDefAST *>(ast)) {
    VisitFuncDef(funcDef);
  } else if (auto funcType = dynamic_cast<const FuncTypeAST *>(ast)) {
    VisitFuncType(funcType);
  } else if (auto block = dynamic_cast<const BlockAST *>(ast)) {
    VisitBlock(block);
  } else if (auto stmt = dynamic_cast<const StmtAST *>(ast)) {
    VisitStmt(stmt);
  } else if (auto exp = dynamic_cast<const ExpAST *>(ast)) {
    VisitExp(exp);
  } else if (auto primary = dynamic_cast<const PrimaryExpAST *>(ast)) {
    VisitPrimaryExp(primary);
  } else if (auto unary = dynamic_cast<const UnaryExpAST *>(ast)) {
    VisitUnaryExp(unary);
  } else if (auto number = dynamic_cast<const NumberAST *>(ast)) {
    VisitNumber(number);
  } else {
    // Handle unknown AST node types if necessary
    assert(false);
  }
}

void IRGenerator::VisitCompUnit(const CompUnitAST *ast) {
  Visit(ast->func_def.get());
}

void IRGenerator::VisitFuncDef(const FuncDefAST *ast) {
  // 1. 输出函数头
  buffer << "fun @" << ast->ident << "(): ";
  auto funcTypeAst = dynamic_cast<const FuncTypeAST *>(ast->func_type.get());
  if (funcTypeAst && funcTypeAst->name == "int") {
    buffer << "i32";
  }
  buffer << " {" << std::endl;

  // 2. 输出函数体入口
  buffer << "%entry:" << std::endl;

  // 3. 递归生成 block 内部的指令
  Visit(ast->block.get());

  // 4. 输出右大括号
  buffer << "}" << std::endl;
}

void IRGenerator::VisitFuncType(const FuncTypeAST *ast) {
  // No IR generation needed for function type
}

void IRGenerator::VisitBlock(const BlockAST *ast) { Visit(ast->stmt.get()); }

void IRGenerator::VisitStmt(const StmtAST *ast) {
  // 计算表达式的值
  Visit(ast->exp.get());
  // 生成 ret 指令，使用 last_val
  buffer << "  ret " << last_val << std::endl;
}

void IRGenerator::VisitExp(const ExpAST *ast) { Visit(ast->unary.get()); }

void IRGenerator::VisitPrimaryExp(const PrimaryExpAST *ast) {
  if (std::holds_alternative<PrimaryExpAST::Number>(ast->data)) {
    int val = std::get<PrimaryExpAST::Number>(ast->data).val;
    last_val = std::to_string(val);
  } else {
    auto &exp = std::get<PrimaryExpAST::Exp>(ast->data);
    Visit(exp.ptr.get());
  }
}

void IRGenerator::VisitUnaryExp(const UnaryExpAST *ast) {
  if (std::holds_alternative<UnaryExpAST::Primary>(ast->data)) {
    auto &primary = std::get<UnaryExpAST::Primary>(ast->data);
    Visit(primary.ptr.get());
  } else {
    auto &unary = std::get<UnaryExpAST::Unary>(ast->data);
    char op = unary.op;
    Visit(unary.ptr.get());

    // 生成指令
    std::string src = last_val;
    std::string dest = NewTempReg();

    if (op == '+') {
      // +Exp 不需要做任何事，结果就是 Exp 的结果
      last_val = src;
    } else if (op == '-') {
      buffer << "  " << dest << " = sub 0, " << src << std::endl;
      last_val = dest;
    } else if (op == '!') {
      buffer << "  " << dest << " = eq " << src << ", 0" << std::endl;
      last_val = dest;
    }
  }
}

void IRGenerator::VisitNumber(const NumberAST *ast) {
  last_val = std::to_string(ast->val);
}
