#include "ir/IRGenVisitor.h"
#include "frontend/AST.h"
#include "frontend/SymbolTable.h"
#include "ir/IR.h"
#include "ir/IRBuilder.h"
#include <cassert>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

IRGenVisitor::IRGenVisitor()
    : module_(std::make_unique<IRModule>()),
      builder_(std::make_unique<IRBuilder>()),
      symtab_(std::make_unique<SymbolTable>()) {}

void IRGenVisitor::VisitCompUnit_(const CompUnitAST *ast) {
  for (auto &func_def : ast->func_defs) {
    func_def->Accept(*this);
  }
}

void IRGenVisitor::VisitFuncDef_(const FuncDefAST *ast) {
  if (module_->GetFunction(ast->ident)) {
    throw std::runtime_error("[Semantic Error]: Duplicate function name " +
                             ast->ident);
  }
  std::string ret_type = ast->ret_type == "int" ? "i32" : "";
  std::string param_type = "i32";
  std::vector<std::string> param_names;
  for (auto &param : ast->params) {
    param_names.push_back(param->ident);
  }
  Function *new_func =
      module_->CreateFunction(ast->ident, ret_type, param_names);
  builder_->SetCurrentFunction(new_func);

  // 将函数名注册到符号表
  symtab_->Define(ast->ident, ret_type);
  symtab_->EnterScope();

  auto *entry_bb = builder_->CreateBlock("entry");
  new_func->exit_bb = builder_->CreateBlock("exit");
  builder_->SetInsertPoint(entry_bb);

  for (auto &param : ast->params) {
    Value param_addr = builder_->CreateAlloca(param_type, param->ident);
    builder_->CreateStore(Value::Reg("%" + param->ident), param_addr);
    symtab_->Define(param->ident, SYMBOL_TYPE_VARIABLE, param_addr.reg_or_addr);
  }

  // 分配返回值变量
  if (ret_type == "i32") {
    Value ret_addr = builder_->CreateAlloca("i32", "ret");
    symtab_->Define("@ret", SYMBOL_TYPE_VARIABLE, ret_addr.reg_or_addr);
  }

  // 访问函数的 Block(不再重复进入作用域)
  for (auto &stmt : ast->block->items) {
    stmt->Accept(*this);
  }

  // 确保当前块有终结指令
  if (!builder_->cur_bb_->HasTerminator()) {
    builder_->CreateJump(new_func->exit_bb);
  }

  // exit块：加载返回值并返回
  builder_->SetInsertPoint(new_func->exit_bb);
  if (ret_type == "i32") {
    auto ret_symbol = symtab_->Lookup("@ret");
    assert(ret_symbol && "Return symbol not found");
    assert(ret_symbol->type == SYMBOL_TYPE_VARIABLE &&
           "Return symbol is not a variable");
    Value ret_val = Value::Addr(*ret_symbol);
    builder_->CreateReturn(ret_val);
  } else {
    builder_->CreateReturn();
  }

  builder_->EndFunction();
  symtab_->ExitScope();
}

void IRGenVisitor::VisitBlock_(const BlockAST *ast) {
  symtab_->EnterScope();

  for (auto &item : ast->items) {
    if (builder_->cur_bb_->HasTerminator()) {
      break;
    }
    if (item) {
      item->Accept(*this);
    }
  }

  symtab_->ExitScope();
}

void IRGenVisitor::VisitConstDecl_(const ConstDeclAST *ast) {
  for (auto &def : ast->const_defs) {
    def->Accept(*this);
  }
}

void IRGenVisitor::VisitConstDef_(const ConstDefAST *ast) {
  assert(ast->init_val);

  Value init_val = Eval(ast->init_val.get());

  if (!init_val.isImmediate()) {
    std::cerr << "const init must be a constant expression" << std::endl;
    assert(false);
  }

  symtab_->Define(ast->ident, SYMBOL_TYPE_CONSTANT, init_val.imm);
}

void IRGenVisitor::VisitVarDecl_(const VarDeclAST *ast) {
  for (auto &def : ast->var_defs) {
    def->Accept(*this);
  }
}

void IRGenVisitor::VisitVarDef_(const VarDefAST *ast) {
  // 分配变量
  Value alloc_addr = builder_->CreateAlloca("i32", ast->ident);
  symtab_->Define(ast->ident, SYMBOL_TYPE_VARIABLE, alloc_addr.reg_or_addr);

  // 初始化
  Value init_val;
  if (ast->init_val) {
    init_val = Eval(ast->init_val.get());
  } else {
    init_val = Value::Imm(0);
  }

  builder_->CreateStore(init_val, alloc_addr);
}

void IRGenVisitor::VisitAssignStmt_(const AssignStmtAST *ast) {
  if (!ast->lval || !ast->exp) {
    std::cerr << "assign: lval or exp is null" << std::endl;
    return;
  }

  auto lval_symbol = symtab_->Lookup(ast->lval->ident);
  if (!lval_symbol) {
    std::cerr << "symbol not found: " << ast->lval->ident << std::endl;
    assert(false);
  }

  if (lval_symbol->type != SYMBOL_TYPE_VARIABLE) {
    std::cerr << "cannot assign to non-variable: " << ast->lval->ident
              << std::endl;
    assert(false);
  }

  Value dest_addr = Value::Addr(*lval_symbol);
  Value src_val = Eval(ast->exp.get());

  builder_->CreateStore(src_val, dest_addr);
}

void IRGenVisitor::VisitExpStmt_(const ExpStmtAST *ast) {
  if (ast->exp) {
    Eval(ast->exp.get()); // 求值但丢弃结果
  }
}

void IRGenVisitor::VisitIfStmt_(const IfStmtAST *ast) {
  // TODO: 目前If还不是SSA形式

  auto *then_bb = builder_->CreateBlock("then");
  auto *end_bb = builder_->CreateBlock("end");

  Value cond = Eval(ast->exp.get());

  if (ast->else_stmt) {
    auto *else_bb = builder_->CreateBlock("else");
    builder_->CreateBranch(cond, then_bb, {}, else_bb, {});

    // then分支
    builder_->SetInsertPoint(then_bb);
    assert(ast->then_stmt);
    ast->then_stmt->Accept(*this);
    if (!builder_->cur_bb_->HasTerminator()) {
      builder_->CreateJump(end_bb);
    }

    // else分支
    builder_->SetInsertPoint(else_bb);
    ast->else_stmt->Accept(*this);
    if (!builder_->cur_bb_->HasTerminator()) {
      builder_->CreateJump(end_bb);
    }
  } else {
    builder_->CreateBranch(cond, then_bb, {}, end_bb, {});

    // then分支
    builder_->SetInsertPoint(then_bb);
    assert(ast->then_stmt);
    ast->then_stmt->Accept(*this);
    if (!builder_->cur_bb_->HasTerminator()) {
      builder_->CreateJump(end_bb);
    }
  }

  builder_->SetInsertPoint(end_bb);
}

void IRGenVisitor::VisitWhileStmt_(const WhileStmtAST *ast) {

  auto *entry_bb = builder_->CreateBlock("while_entry");
  auto *body_bb = builder_->CreateBlock("while_body");
  auto *end_bb = builder_->CreateBlock("while_end");

  break_targets_.push_back(end_bb);
  continue_targets_.push_back(entry_bb);

  // 跳转到 entry 块
  builder_->CreateJump(entry_bb);

  // entry 块
  builder_->SetInsertPoint(entry_bb);
  Value cond = Eval(ast->cond.get());
  builder_->CreateBranch(cond, body_bb, {}, end_bb, {});

  // body 块
  builder_->SetInsertPoint(body_bb);
  assert(ast->body);
  ast->body->Accept(*this);
  if (!builder_->cur_bb_->HasTerminator()) {
    builder_->CreateJump(entry_bb);
  }

  break_targets_.pop_back();
  continue_targets_.pop_back();

  // end 块
  builder_->SetInsertPoint(end_bb);
}

void IRGenVisitor::VisitBreakStmt_(const BreakStmtAST *ast) {
  if (break_targets_.empty()) {
    throw std::runtime_error(
        "[Semantic Error]: 'break' statement not within a loop");
  }
  builder_->CreateJump(break_targets_.back());
}

void IRGenVisitor::VisitContinueStmt_(const ContinueStmtAST *ast) {
  if (continue_targets_.empty()) {
    throw std::runtime_error(
        "[Semantic Error]: 'continue' statement not within a loop");
  }
  builder_->CreateJump(continue_targets_.back());
}

void IRGenVisitor::VisitReturnStmt_(const ReturnStmtAST *ast) {
  if (builder_->cur_bb_->HasTerminator()) {
    return;
  }

  if (ast->exp) {
    Value src_val = Eval(ast->exp.get());

    auto ret_symbol = symtab_->Lookup("@ret");
    if (!ret_symbol) {
      std::cerr << "symbol not found: @ret" << std::endl;
      assert(false);
    }
    Value dest_addr = Value::Addr(*ret_symbol);

    builder_->CreateStore(src_val, dest_addr);
  }

  // 跳转到函数的 exit 块
  if (builder_->cur_func_ && builder_->cur_func_->exit_bb) {
    builder_->CreateJump(builder_->cur_func_->exit_bb);
  }
}

// ==================== 表达式求值 ====================

Value IRGenVisitor::Eval(BaseAST *ast) {
  if (auto *lval = dynamic_cast<LValAST *>(ast)) {
    return EvalLVal(lval);
  } else if (auto *number = dynamic_cast<NumberAST *>(ast)) {
    return EvalNumber(number);
  } else if (auto *unary = dynamic_cast<UnaryExpAST *>(ast)) {
    return EvalUnaryExp(unary);
  } else if (auto *binary = dynamic_cast<BinaryExpAST *>(ast)) {
    return EvalBinaryExp(binary);
  } else if (auto *func_call = dynamic_cast<FuncCallAST *>(ast)) {
    return EvalFuncCall(func_call);
  } else {
    std::cerr << "Eval: unsupported expression type" << std::endl;
    // assert(false);
  }

  return Value::Imm(0);
}

Value IRGenVisitor::EvalLVal(LValAST *ast) {
  auto lval_symbol = symtab_->Lookup(ast->ident);
  if (!lval_symbol) {
    std::cerr << "symbol not found: " << ast->ident << std::endl;
    assert(false);
  }
  Value val;
  if (lval_symbol->type == SYMBOL_TYPE_CONSTANT) {
    val = Value::Imm(*lval_symbol);
  } else if (lval_symbol->type == SYMBOL_TYPE_VARIABLE) {
    val = Value::Addr(*lval_symbol);
  } else {
    std::cerr << "unsupported lval symbol type for: " << ast->ident
              << std::endl;
    assert(false);
  }

  // 常量直接返回立即数，变量返回地址
  return val;
}

Value IRGenVisitor::EvalNumber(NumberAST *ast) { return Value::Imm(ast->val); }

Value IRGenVisitor::EvalUnaryExp(UnaryExpAST *ast) {
  if (!ast->exp) {
    std::cerr << "unary operand is null" << std::endl;
    assert(false);
  }

  Value operand = Eval(ast->exp.get());
  return builder_->CreateUnaryOp(ast->op, operand);
}

Value IRGenVisitor::EvalBinaryExp(BinaryExpAST *ast) {
  if (!ast->lhs || !ast->rhs) {
    std::cerr << "binary operand is null" << std::endl;
    assert(false);
  }

  Value lhs = Eval(ast->lhs.get());
  Value rhs = Eval(ast->rhs.get());

  // 常量折叠
  if (lhs.isImmediate() && rhs.isImmediate()) {
    int l = lhs.imm, r = rhs.imm;
    int result;
    if (ast->op == "+")
      result = l + r;
    else if (ast->op == "-")
      result = l - r;
    else if (ast->op == "*")
      result = l * r;
    else if (ast->op == "/")
      result = l / r;
    else if (ast->op == "%")
      result = l % r;
    else if (ast->op == "<")
      result = l < r;
    else if (ast->op == ">")
      result = l > r;
    else if (ast->op == "<=")
      result = l <= r;
    else if (ast->op == ">=")
      result = l >= r;
    else if (ast->op == "==")
      result = l == r;
    else if (ast->op == "!=")
      result = l != r;
    else if (ast->op == "&&")
      result = l && r;
    else if (ast->op == "||")
      result = l || r;
    else {
      std::cerr << "Unknown binary operator: " << ast->op << std::endl;
      assert(false);
    }
    return Value::Imm(result);
  }

  // 短路求值 && 与 ||
  if (ast->op == "&&") {
    return EvalLogicalAnd(ast);
  }

  if (ast->op == "||") {
    return EvalLogicalOr(ast);
  }

  return builder_->CreateBinaryOp(ast->op, lhs, rhs);
}

Value IRGenVisitor::EvalFuncCall(FuncCallAST *ast) {
  Function *func = module_->GetFunction(ast->ident);
  if (!func) {
    std::cerr << "function not found: " << ast->ident << std::endl;
    assert(false);
  }

  std::vector<Value> arg_values;
  for (auto &arg_exp : ast->args) {
    Value arg_val = Eval(arg_exp.get());
    arg_values.push_back(arg_val);
  }

  Value ret_reg;
  if (func->ret_type == "i32") {
    ret_reg = builder_->CreateCall(func->name, arg_values, true);
  } else {
    builder_->CreateCall(func->name, arg_values, false);
  }
  return ret_reg;
}

/**
 * 逻辑与的返回值一定是 i32 类型的立即数
 */
Value IRGenVisitor::EvalLogicalAnd(BinaryExpAST *ast) {
  auto *rhs_bb = builder_->CreateBlock("and_rhs");
  auto *end_bb = builder_->CreateBlock("and_end");

  // 计算左操作数
  Value lhs = Eval(ast->lhs.get());
  builder_->CreateBranch(lhs, rhs_bb, {}, end_bb, {Value::Imm(0)});

  // 计算右操作数
  builder_->SetInsertPoint(rhs_bb);
  Value rhs = Eval(ast->rhs.get());
  Value rhs_bool = builder_->CreateBinaryOp("!=", rhs, Value::Imm(0));
  builder_->CreateJump(end_bb, {rhs_bool});

  // end_bb 基本快的第一个参数，它是一个 i32 类型的寄存器，将其中的值返回
  builder_->SetInsertPoint(end_bb);
  Value res_reg = end_bb->AddParam("i32");
  return res_reg;
}

/**
 * 逻辑或的返回值一定是 i32 类型的立即数
 */
Value IRGenVisitor::EvalLogicalOr(BinaryExpAST *ast) {
  auto *rhs_bb = builder_->CreateBlock("or_rhs");
  auto *end_bb = builder_->CreateBlock("or_end");

  // 计算左操作数
  Value lhs = Eval(ast->lhs.get());
  builder_->CreateBranch(lhs, end_bb, {Value::Imm(1)}, rhs_bb, {});

  // 计算右操作数
  builder_->SetInsertPoint(rhs_bb);
  Value rhs = Eval(ast->rhs.get());
  Value rhs_bool = builder_->CreateBinaryOp("!=", rhs, Value::Imm(0));
  builder_->CreateJump(end_bb, {rhs_bool});

  // end_bb 基本快的第一个参数，它是一个 i32 类型的寄存器，将其中的值返回
  builder_->SetInsertPoint(end_bb);
  Value res_reg = end_bb->AddParam("i32");
  return res_reg;
}
// ==================== Visitor接口实现 ====================

void IRGenVisitor::Visit(CompUnitAST &node) { VisitCompUnit_(&node); }
void IRGenVisitor::Visit(FuncFParamAST &node) {}
void IRGenVisitor::Visit(FuncDefAST &node) { VisitFuncDef_(&node); }
void IRGenVisitor::Visit(BlockAST &node) { VisitBlock_(&node); }
void IRGenVisitor::Visit(ConstDeclAST &node) { VisitConstDecl_(&node); }
void IRGenVisitor::Visit(ConstDefAST &node) { VisitConstDef_(&node); }
void IRGenVisitor::Visit(VarDeclAST &node) { VisitVarDecl_(&node); }
void IRGenVisitor::Visit(VarDefAST &node) { VisitVarDef_(&node); }
void IRGenVisitor::Visit(AssignStmtAST &node) { VisitAssignStmt_(&node); }
void IRGenVisitor::Visit(ExpStmtAST &node) { VisitExpStmt_(&node); }
void IRGenVisitor::Visit(IfStmtAST &node) { VisitIfStmt_(&node); }
void IRGenVisitor::Visit(WhileStmtAST &node) { VisitWhileStmt_(&node); }
void IRGenVisitor::Visit(BreakStmtAST &node) { VisitBreakStmt_(&node); }
void IRGenVisitor::Visit(ContinueStmtAST &node) { VisitContinueStmt_(&node); }
void IRGenVisitor::Visit(ReturnStmtAST &node) { VisitReturnStmt_(&node); }
void IRGenVisitor::Visit(LValAST &node) {}
void IRGenVisitor::Visit(NumberAST &node) {}
void IRGenVisitor::Visit(UnaryExpAST &node) {}
void IRGenVisitor::Visit(BinaryExpAST &node) {}
void IRGenVisitor::Visit(FuncCallAST &node) {}
