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
      builder_(std::make_unique<IRBuilder>(module_.get())),
      symtab_(std::make_unique<SymbolTable>()) {

  auto RegisterStandardLibrary =
      [this](std::string func_name, std::string ret_type,
             std::vector<std::pair<std::string, std::string>> param_types) {
        module_->CreateFunction(func_name, ret_type, param_types);
      };

  RegisterStandardLibrary("getint", "i32", {});
  RegisterStandardLibrary("getch", "i32", {});
  RegisterStandardLibrary("getarray", "i32", {{"ptr", "*i32"}});
  RegisterStandardLibrary("putint", "", {{"val", "i32"}});
  RegisterStandardLibrary("putch", "", {{"val", "i32"}});
  RegisterStandardLibrary("putarray", "", {{"val", "i32"}, {"ptr", "*i32"}});
  RegisterStandardLibrary("starttime", "", {});
  RegisterStandardLibrary("stoptime", "", {});
}

void IRGenVisitor::VisitCompUnit_(const CompUnitAST *ast) {
  for (auto &func_def : ast->items) {
    func_def->Accept(*this);
  }
}

void IRGenVisitor::VisitFuncDef_(const FuncDefAST *ast) {
  if (module_->GetFunction(ast->ident)) {
    throw std::runtime_error("[Semantic Error]: Duplicate function name " +
                             ast->ident);
  }
  std::string func_name = ast->ident;
  std::string ret_type = ast->ret_type == "int" ? "i32" : "void";
  std::vector<std::pair<std::string, std::string>> params;
  for (auto &param : ast->params) {
    params.push_back({param->ident, param->btype == "int" ? "i32" : "*i32"});
  }

  // 向 IRModule 注册一个函数
  Function *new_func = module_->CreateFunction(func_name, ret_type, params);
  builder_->SetCurrentFunction(new_func);

  // 向 SymbolTable 注册函数名
  symtab_->DefineFunction(func_name, ret_type);
  symtab_->EnterScope();

  auto *entry_bb = builder_->CreateBlock("entry");
  auto *exit_bb = builder_->CreateBlock("exit");

  // 将 exit_bb 记录在函数作用域中
  builder_->cur_func_->exit_bb = exit_bb;

  // 将函数参数存入符号表
  builder_->SetInsertPoint(entry_bb);
  for (auto &param : ast->params) {
    if (param->btype == "int") {
      Value param_addr = builder_->CreateAlloca("i32", param->ident);
      builder_->CreateStore(Value::Reg("%" + param->ident), param_addr);
      symtab_->DefineVariable(param->ident, param_addr.reg_or_addr);
    } else {
      // 数组参数（指针）
      // 这里我们简化处理，不分配栈空间，直接将参数寄存器注册为 POINTER
      // 注意：这意味着我们不支持对数组参数本身的赋值（如 a = b），但这在 C/C++
      // 数组参数中是允许的（作为指针变量） 如果需要支持
      // assignment，我们需要分配 alloc *i32，但目前 alloc 只支持 i32
      symtab_->DefinePointer(param->ident, "%" + param->ident);
    }
  }

  // 分配返回值变量
  if (ret_type == "i32") {
    Value ret_addr = builder_->CreateAlloca("i32", "ret");
    builder_->cur_func_->ret_addr = ret_addr;
    builder_->cur_func_->has_return = true;
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
  if (builder_->cur_func_->has_return) {
    Value ret_val = builder_->CreateLoad(builder_->cur_func_->ret_addr);
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

  if (symtab_->IsGlobal()) {
    symtab_->DefineGlobalConstant(ast->ident, init_val.imm);
  } else {
    symtab_->DefineConstant(ast->ident, init_val.imm);
  }
}

void IRGenVisitor::VisitVarDecl_(const VarDeclAST *ast) {
  for (auto &def : ast->var_defs) {
    def->Accept(*this);
  }
}

void IRGenVisitor::VisitVarDef_(const VarDefAST *ast) {
  if (ast->array_size == nullptr) {
    // 单个变量
    if (symtab_->IsGlobal()) {
      Value init_val = Value::Imm(0);
      if (ast->init_val) {
        init_val = Eval(ast->init_val.get());
        if (!init_val.isImmediate()) {
          // 全局变量初始化必须是常量表达式
          std::cerr
              << "Global variable initializer must be a constant expression"
              << std::endl;
          assert(false);
        }
      }

      Value global_addr =
          builder_->CreateGlobalAlloc("i32", ast->ident, init_val);
      symtab_->DefineGlobalVariable(ast->ident, global_addr.reg_or_addr);
      return;
    } else {
      // 局部变量
      Value alloc_addr = builder_->CreateAlloca("i32", ast->ident);
      symtab_->DefineVariable(ast->ident, alloc_addr.reg_or_addr);

      // 初始化
      if (ast->init_val) {
        Value init_val = Eval(ast->init_val.get());
        builder_->CreateStore(init_val, alloc_addr);
      }
      return;
    }
  } else {
    // 数组变量
    Value array_size_val = Eval(ast->array_size.get());
    if (!array_size_val.isImmediate() || array_size_val.imm <= 0) {
      std::cerr << "Array size must be a positive constant integer"
                << std::endl;
      assert(false);
    }
    int array_size = array_size_val.imm;

    if (symtab_->IsGlobal()) {
      // 全局数组
      std::vector<Value> init_vals;

      if (ast->init_val) {
        FlattenInitList(ast->init_val.get(), init_vals);
        if (init_vals.size() > static_cast<size_t>(array_size)) {
          std::cerr << "[Semantic Error]: Too many initializers for array "
                    << ast->ident << std::endl;
          assert(false);
        }

        init_vals.resize(array_size, Value::Imm(0));
      } else {
        init_vals.resize(array_size, Value::Imm(0));
      }

      Value global_addr = builder_->CreateGlobalArrayAlloca(
          "i32", ast->ident, array_size, init_vals);
      symtab_->DefineGlobalArray(ast->ident, global_addr.reg_or_addr, {array_size});
      return;
    } else {
      // 局部数组
      Value alloc_addr =
          builder_->CreateArrayAlloca("i32", ast->ident, array_size);
      symtab_->DefineArray(ast->ident, alloc_addr.reg_or_addr, {array_size});

      if (ast->init_val) {
        std::vector<Value> init_vals;
        FlattenInitList(ast->init_val.get(), init_vals);

        if (init_vals.size() > static_cast<size_t>(array_size)) {
          std::cerr << "[Semantic Error]: Too many initializers for array "
                    << ast->ident << std::endl;
          assert(false);
        }

        for (int i = 0; i < array_size; i++) {
          Value idx = Value::Imm(i);
          Value ptr = builder_->CreateGetElemPtr(alloc_addr, idx);

          if (i < static_cast<int>(init_vals.size())) {
            builder_->CreateStore(init_vals[i], ptr);
          } else {
            builder_->CreateStore(Value::Imm(0), ptr);
          }
        }
      }
    }
  }
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

    if (builder_->cur_func_->has_return) {
      builder_->CreateStore(src_val, builder_->cur_func_->ret_addr);
    }
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

  Value base;
  if (lval_symbol->type == SYMBOL_TYPE_CONSTANT) {
    return Value::Imm(*lval_symbol);
  } else if (lval_symbol->type == SYMBOL_TYPE_VARIABLE ||
             lval_symbol->type == SYMBOL_TYPE_ARRAY) {
    base = Value::Addr(*lval_symbol);
  } else if (lval_symbol->type == SYMBOL_TYPE_POINTER) {
    base = Value::Reg(std::get<std::string>(lval_symbol->value));
  } else {
    std::cerr << "unsupported lval symbol type for: " << ast->ident
              << std::endl;
    assert(false);
  }

  if (ast->index_exp) {
    Value index = Eval(ast->index_exp.get());
    Value ptr = builder_->CreateGetElemPtr(base, index);
    return ptr;
  } else {
    if (lval_symbol->type == SYMBOL_TYPE_ARRAY) {
      Value ptr = builder_->CreateGetElemPtr(base, Value::Imm(0));
      return Value::Reg(ptr.reg_or_addr);
    } else if (lval_symbol->type == SYMBOL_TYPE_POINTER) {
      return base;
    } else {
      return base;
    }
  }
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

  // 短路求值 && 与 ||
  if (ast->op == "&&") {
    return EvalLogicalAnd(ast);
  }

  if (ast->op == "||") {
    return EvalLogicalOr(ast);
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

void IRGenVisitor::FlattenInitList(const BaseAST *ast,
                                   std::vector<Value> &init_vals) {
  if (auto *init_var = dynamic_cast<const InitVarAST *>(ast)) {
    if (init_var->is_list) {
      for (auto &child : init_var->inits) {
        FlattenInitList(child.get(), init_vals);
      }
    } else {
      init_vals.push_back(Eval(init_var->exp.get()));
    }
  } else {
    // 应该不会走到这里
    assert(false && "FlattenInitList: unexpected AST type");
  }
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
void IRGenVisitor::Visit(InitVarAST &node) {}
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
