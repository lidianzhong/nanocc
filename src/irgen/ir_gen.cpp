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
  } else if (auto mulExp = dynamic_cast<const MulExpAST *>(ast)) {
    VisitMulExp(mulExp);
  } else if (auto addExp = dynamic_cast<const AddExpAST *>(ast)) {
    VisitAddExp(addExp);
  } else if (auto relExp = dynamic_cast<const RelExpAST *>(ast)) {
    VisitRelExp(relExp);
  } else if (auto eqExp = dynamic_cast<const EqExpAST *>(ast)) {
    VisitEqExp(eqExp);
  } else if (auto landExp = dynamic_cast<const LAndExpAST *>(ast)) {
    VisitLAndExp(landExp);
  } else if (auto lorExp = dynamic_cast<const LOrExpAST *>(ast)) {
    VisitLOrExp(lorExp);
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

void IRGenerator::VisitExp(const ExpAST *ast) { Visit(ast->lor_exp.get()); }

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

void IRGenerator::VisitMulExp(const MulExpAST *ast) {
  if (std::holds_alternative<MulExpAST::Unary>(ast->data)) {
    auto &unary = std::get<MulExpAST::Unary>(ast->data);
    Visit(unary.ptr.get());
  } else {
    auto &mul = std::get<MulExpAST::Mul>(ast->data);
    Visit(mul.mul_exp.get());
    std::string left = last_val;
    Visit(mul.unary_exp.get());
    std::string right = last_val;

    std::string dest = NewTempReg();
    char op = mul.op;
    if (op == '*') {
      buffer << "  " << dest << " = mul " << left << ", " << right << std::endl;
    } else if (op == '/') {
      buffer << "  " << dest << " = div " << left << ", " << right << std::endl;
    } else if (op == '%') {
      buffer << "  " << dest << " = mod " << left << ", " << right << std::endl;
    }
    last_val = dest;
  }
}

void IRGenerator::VisitAddExp(const BaseAST *ast) {
  auto addExpAst = dynamic_cast<const AddExpAST *>(ast);
  if (std::holds_alternative<AddExpAST::Mul>(addExpAst->data)) {
    auto &mul = std::get<AddExpAST::Mul>(addExpAst->data);
    Visit(mul.ptr.get());
  } else {
    auto &add = std::get<AddExpAST::Add>(addExpAst->data);
    Visit(add.add_exp.get());
    std::string left = last_val;
    Visit(add.mul_exp.get());
    std::string right = last_val;

    std::string dest = NewTempReg();
    char op = add.op;
    if (op == '+') {
      buffer << "  " << dest << " = add " << left << ", " << right << std::endl;
    } else if (op == '-') {
      buffer << "  " << dest << " = sub " << left << ", " << right << std::endl;
    }
    last_val = dest;
  }
}

void IRGenerator::VisitRelExp(const BaseAST *ast) {
  auto relExpAst = dynamic_cast<const RelExpAST *>(ast);
  if (std::holds_alternative<RelExpAST::Add>(relExpAst->data)) {
    auto &add = std::get<RelExpAST::Add>(relExpAst->data);
    Visit(add.ptr.get());
  } else {
    auto &rel = std::get<RelExpAST::Rel>(relExpAst->data);
    Visit(rel.rel_exp.get());
    std::string left = last_val;
    Visit(rel.add_exp.get());
    std::string right = last_val;

    std::string dest = NewTempReg();
    std::string op = rel.op;
    if (op == "<") {
      buffer << "  " << dest << " = lt " << left << ", " << right << std::endl;
    } else if (op == "<=") {
      buffer << "  " << dest << " = le " << left << ", " << right << std::endl;
    } else if (op == ">") {
      buffer << "  " << dest << " = gt " << left << ", " << right << std::endl;
    } else if (op == ">=") {
      buffer << "  " << dest << " = ge " << left << ", " << right << std::endl;
    }
    last_val = dest;
  }
}

void IRGenerator::VisitEqExp(const BaseAST *ast) {
  auto eqExpAst = dynamic_cast<const EqExpAST *>(ast);
  if (std::holds_alternative<EqExpAST::Rel>(eqExpAst->data)) {
    auto &rel = std::get<EqExpAST::Rel>(eqExpAst->data);
    Visit(rel.ptr.get());
  } else {
    auto &eq = std::get<EqExpAST::Eq>(eqExpAst->data);
    Visit(eq.eq_exp.get());
    std::string left = last_val;
    Visit(eq.rel_exp.get());
    std::string right = last_val;

    std::string dest = NewTempReg();
    std::string op = eq.op;
    if (op == "==") {
      buffer << "  " << dest << " = eq " << left << ", " << right << std::endl;
    } else if (op == "!=") {
      buffer << "  " << dest << " = ne " << left << ", " << right << std::endl;
    }
    last_val = dest;
  }
}

void IRGenerator::VisitLAndExp(const BaseAST *ast) {
  auto landExpAst = dynamic_cast<const LAndExpAST *>(ast);
  if (std::holds_alternative<LAndExpAST::Eq>(landExpAst->data)) {
    Visit(std::get<LAndExpAST::Eq>(landExpAst->data).ptr.get());
  } else {
    auto &land = std::get<LAndExpAST::LAnd>(landExpAst->data);
    
    // 1. 处理左操作数：计算并强制转换为布尔值 (ne %val, 0)
    Visit(land.land_exp.get());
    std::string left = last_val;
    std::string b_left = NewTempReg();
    buffer << "  " << b_left << " = ne " << left << ", 0" << std::endl;

    // 2. 处理右操作数：计算并强制转换为布尔值
    Visit(land.eq_exp.get());
    std::string right = last_val;
    std::string b_right = NewTempReg();
    buffer << "  " << b_right << " = ne " << right << ", 0" << std::endl;

    // 3. 执行按位与运算
    // 此时 1 && 1 = 1, 1 && 0 = 0, 0 && 0 = 0，符合逻辑与预期
    std::string dest = NewTempReg();
    buffer << "  " << dest << " = and " << b_left << ", " << b_right << std::endl;
    last_val = dest;
  }
}

void IRGenerator::VisitLOrExp(const BaseAST *ast) {
  auto lorExpAst = dynamic_cast<const LOrExpAST *>(ast);
  if (std::holds_alternative<LOrExpAST::LAnd>(lorExpAst->data)) {
    Visit(std::get<LOrExpAST::LAnd>(lorExpAst->data).ptr.get());
  } else {
    auto &lor = std::get<LOrExpAST::LOr>(lorExpAst->data);
    
    // 1. 处理左操作数并强制布尔化
    Visit(lor.lor_exp.get());
    std::string left = last_val;
    std::string b_left = NewTempReg();
    // %b_left = ne %left, 0  (在 Koopa 中，ne x, 0 等价于将 x 转换为 0 或 1)
    buffer << "  " << b_left << " = ne " << left << ", 0" << std::endl;

    // 2. 处理右操作数并强制布尔化
    Visit(lor.land_exp.get());
    std::string right = last_val;
    std::string b_right = NewTempReg();
    buffer << "  " << b_right << " = ne " << right << ", 0" << std::endl;

    // 3. 执行按位或，此时操作数保证是 0 或 1
    std::string dest = NewTempReg();
    buffer << "  " << dest << " = or " << b_left << ", " << b_right << std::endl;
    last_val = dest;
  }
}