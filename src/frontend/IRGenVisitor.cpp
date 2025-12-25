#include <cassert>
#include <iostream>
#include <memory>
#include <variant>

#include "frontend/IRGenVisitor.h"

#include "frontend/AST.h"
#include "frontend/SymbolTable.h"
#include "ir/IRBuilder.h"

IRGenVisitor::IRGenVisitor()
    : symtab_(), builder_(std::make_unique<IRBuilder>()) {}

std::string IRGenVisitor::GetIR() const {
  assert(builder_);
  return builder_->GetIR();
}

koopa_raw_program_t IRGenVisitor::GetProgram() const {
  assert(builder_);
  return builder_->Build();
}

void IRGenVisitor::VisitCompUnit_(const CompUnitAST *ast) {
  if (ast->func_def) {
    ast->func_def->Accept(*this);
  }
}

void IRGenVisitor::VisitFuncDef_(const FuncDefAST *ast) {
  std::string ret_type = ast->ret_type == "int" ? "i32" : "void";
  builder_->StartFunction(ast->ident, ret_type);
  builder_->CreateBasicBlock("entry");
  if (ast->block) {
    ast->block->Accept(*this);
  }
  builder_->EndFunction();
}

void IRGenVisitor::VisitBlock_(const BlockAST *ast) {
  builder_->buffer_ << std::endl; // debug
  symtab_.EnterScope();

  for (auto &item : ast->items) {
    if (item) {
      item->Accept(*this);
    }
  }

  symtab_.ExitScope();
  builder_->buffer_ << std::endl; // debug
}

void IRGenVisitor::VisitConstDecl_(const ConstDeclAST *ast) {
  for (auto &def : ast->const_defs) {
    if (def) {
      def->Accept(*this);
    }
  }
}

void IRGenVisitor::VisitConstDef_(const ConstDefAST *ast) {
  // 常量不允许在定义时不初始化
  assert(ast->init_val);
  ast->init_val->Accept(*this);

  // 常量初始化计算后一定是常量
  assert(std::holds_alternative<int32_t>(last_val_));
  symtab_.Define(ast->ident, std::get<int32_t>(last_val_));
}

void IRGenVisitor::VisitVarDecl_(const VarDeclAST *ast) {
  for (auto &def : ast->var_defs) {
    if (def) {
      def->Accept(*this);
    }
  }
}

void IRGenVisitor::VisitVarDef_(const VarDefAST *ast) {
  // 在当前作用域内分配变量
  std::string alloc_addr = builder_->CreateAlloca("i32", ast->ident);
  // 当前作用域添加一个变量
  symtab_.Define(ast->ident, alloc_addr);

  // 不允许变量在定义时不初始化
  assert(ast->init_val);
  ast->init_val->Accept(*this);
  auto exp_val = last_val_;

  if (std::holds_alternative<int32_t>(exp_val)) { // 常量定义
    builder_->CreateStore(builder_->CreateNumber(std::get<int32_t>(exp_val)),
                          alloc_addr);
  } else if (std::holds_alternative<std::string>(exp_val)) { // 变量定义
    builder_->CreateStore(std::get<std::string>(exp_val), alloc_addr);
  } else {
    std::cerr << "VarDefAST: init_val is not allow to be empty" << std::endl;
    assert(false);
  }

  last_val_ = std::monostate();
}

void IRGenVisitor::VisitAssignStmt_(const AssignStmtAST *ast) {
  if (!ast->lval || !ast->exp) {
    std::cerr << "assign left value or right expression is null" << std::endl;
    return;
  }

  // 从符号表检索变量
  auto lval_symbol = symtab_.Lookup(ast->lval->ident);
  if (!lval_symbol.has_value()) {
    std::cerr << "symbol table not found variable: " << ast->lval->ident
              << std::endl;
    assert(false);
  }

  // 符号表中检索到的必须是可赋值的变量类型
  if (lval_symbol.value().type != SYMBOL_TYPE_VARIABLE) {
    std::cerr << "symbol table found variable: " << ast->lval->ident
              << " is not a variable" << std::endl;
    assert(false);
  }

  std::string lval_offset = std::get<std::string>(lval_symbol.value().value);

  // 计算右侧表达式
  ast->exp->Accept(*this);
  auto exp_val = last_val_;

  if (std::holds_alternative<int32_t>(exp_val)) { // 常量定义
    builder_->CreateStore(builder_->CreateNumber(std::get<int32_t>(exp_val)),
                          lval_offset);
  } else if (std::holds_alternative<std::string>(exp_val)) { // 变量定义
    builder_->CreateStore(std::get<std::string>(exp_val), lval_offset);
  } else {
    std::cerr << "VarDefAST: init_val is not allow to be empty" << std::endl;
    assert(false);
  }
  last_val_ = std::monostate();
}

void IRGenVisitor::VisitExpStmt_(const ExpStmtAST *ast) {
  if (ast->exp) {
    ast->exp->Accept(*this);
  }
}

void IRGenVisitor::VisitReturnStmt_(const ReturnStmtAST *ast) {
  if (ast->exp) {
    ast->exp->Accept(*this);
    // 有返回值，将exp计算结果返回
    if (std::holds_alternative<int32_t>(last_val_)) {
      builder_->CreateReturn(
          builder_->CreateNumber(std::get<int32_t>(last_val_)));
    } else if (std::holds_alternative<std::string>(last_val_)) {
      builder_->CreateReturn(std::get<std::string>(last_val_));
    } else {
      std::cerr << "ReturnStmtAST: exp is not allow to be empty" << std::endl;
      assert(false);
    }
  } else {
    // void 返回
    builder_->CreateReturn();
  }
  last_val_ = std::monostate();
}

void IRGenVisitor::VisitLVal_(const LValAST *ast) {
  // 访问左值时都采取加载到内存中来
  auto lval_symbol = symtab_.Lookup(ast->ident);

  // 符号表中检索到的如果是1.可赋值的变量类型 2.CONST常量
  if (lval_symbol.value().type == SYMBOL_TYPE_VARIABLE) {
    std::string lval_offset = std::get<std::string>(lval_symbol.value().value);
    last_val_ = builder_->CreateLoad(lval_offset);
  } else if (lval_symbol.value().type == SYMBOL_TYPE_CONSTANT) {
    last_val_ = std::get<int32_t>(lval_symbol.value().value);
  } else {
    std::cerr << "symbol table found variable: " << ast->ident
              << " is not a variable or constant" << std::endl;
    assert(false);
  }
}

void IRGenVisitor::VisitNumber_(const NumberAST *ast) { last_val_ = ast->val; }

void IRGenVisitor::VisitUnaryExp_(const UnaryExpAST *ast) {
  if (ast->exp) {
    ast->exp->Accept(*this);
    if (std::holds_alternative<int32_t>(last_val_)) {
      std::cerr << "operand can not be a number" << std::endl;
      assert(false);
    } else if (std::holds_alternative<std::string>(last_val_)) {
      std::string operand = std::get<std::string>(last_val_);
      last_val_ = builder_->CreateUnaryOp(ast->op, operand);
    } else {
      std::cerr << "operand can not be a monostate" << std::endl;
      assert(false);
    }
  }
}

void IRGenVisitor::VisitBinaryExp_(const BinaryExpAST *ast) {
  if (!ast->lhs || !ast->rhs) {
    std::cerr << "binary expression left or right is null" << std::endl;
    return;
  }

  ast->lhs->Accept(*this);

  std::string lhs_val;
  if (std::holds_alternative<int32_t>(last_val_)) {
    lhs_val = builder_->CreateNumber(std::get<int32_t>(last_val_));
  } else if (std::holds_alternative<std::string>(last_val_)) {
    lhs_val = std::get<std::string>(last_val_);
  } else {
    std::cerr << "left operand can not be a monostate" << std::endl;
    assert(false);
  }

  ast->rhs->Accept(*this);

  std::string rhs_val;
  if (std::holds_alternative<int32_t>(last_val_)) {
    rhs_val = builder_->CreateNumber(std::get<int32_t>(last_val_));
  } else if (std::holds_alternative<std::string>(last_val_)) {
    rhs_val = std::get<std::string>(last_val_);
  } else {
    std::cerr << "right operand can not be a monostate" << std::endl;
    assert(false);
  }

  last_val_ = builder_->CreateBinaryOp(ast->op, lhs_val, rhs_val);
}

void IRGenVisitor::Visit(CompUnitAST &node) { VisitCompUnit_(&node); }
void IRGenVisitor::Visit(FuncDefAST &node) { VisitFuncDef_(&node); }
void IRGenVisitor::Visit(BlockAST &node) { VisitBlock_(&node); }
void IRGenVisitor::Visit(ConstDeclAST &node) { VisitConstDecl_(&node); }
void IRGenVisitor::Visit(ConstDefAST &node) { VisitConstDef_(&node); }
void IRGenVisitor::Visit(VarDeclAST &node) { VisitVarDecl_(&node); }
void IRGenVisitor::Visit(VarDefAST &node) { VisitVarDef_(&node); }
void IRGenVisitor::Visit(AssignStmtAST &node) { VisitAssignStmt_(&node); }
void IRGenVisitor::Visit(ExpStmtAST &node) { VisitExpStmt_(&node); }
void IRGenVisitor::Visit(ReturnStmtAST &node) { VisitReturnStmt_(&node); }
void IRGenVisitor::Visit(LValAST &node) { VisitLVal_(&node); }
void IRGenVisitor::Visit(NumberAST &node) { VisitNumber_(&node); }
void IRGenVisitor::Visit(UnaryExpAST &node) { VisitUnaryExp_(&node); }
void IRGenVisitor::Visit(BinaryExpAST &node) { VisitBinaryExp_(&node); }
