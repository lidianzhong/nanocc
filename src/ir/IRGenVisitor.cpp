#include "ir/IRGenVisitor.h"
#include "frontend/AST.h"
#include "frontend/ASTVisitor.h"
#include "ir/Constant.h"
#include "ir/Function.h"
#include "ir/GlobalVariable.h"
#include "ir/IRBuilder.h"
#include "ir/Instruction.h"
#include "ir/Module.h"
#include "ir/Type.h"
#include "ir/Value.h"
#include "ir/ValueSymbolTable.h"
#include <cassert>
#include <cstddef>
#include <vector>

namespace nanocc {

IRGenVisitor::IRGenVisitor(Module &module)
    : module_(module), builder_(new IRBuilder()),
      nameValues_(new ValueSymbolTable()) {

  registerLibFunctions();
}

//===--------------------------------------------------------------------===//
// Each Visitor Entry Point
//

void IRGenVisitor::visitCompUnit_(const CompUnitAST *ast) {
  for (const auto &item : ast->items) {
    if (auto *funcDef = dynamic_cast<FuncDefAST *>(item.get())) {
      visitFuncDef_(funcDef);
    } else if (auto *decl = dynamic_cast<VarDeclAST *>(item.get())) {
      visitVarDecl_(decl);
    } else if (auto *constDecl = dynamic_cast<ConstDeclAST *>(item.get())) {
      visitConstDecl_(constDecl);
    } else {
      assert(false && "Unknown global item");
    }
  }
}

void IRGenVisitor::visitFuncDef_(const FuncDefAST *ast) {
  // return type
  Type *retType =
      (ast->ret_type == "void") ? Type::getVoidTy() : Type::getInt32Ty();

  // determine prameter type
  std::vector<Type *> paramTypes;
  for (const auto &param : ast->params) {
    Type *paramType = nullptr;
    if (param->btype == "int") {
      paramType = Type::getInt32Ty();
    } else if (param->btype == "*int") {
      paramType = Type::getInt32Ty();
      for (auto it = param->dims.rbegin(); it != param->dims.rend(); ++it) {
        assert(*it != nullptr);
        int dim = evalConstExpr(it->get()); // evaluate dim expr
        paramType = Type::getArrayTy(paramType, dim);
      }
      paramType = Type::getPointerTy(paramType);
    } else {
      assert(false && "Unsupported parameter base type");
    }
    paramTypes.push_back(paramType);
  }

  // function object
  FunctionType *FT = FunctionType::get(retType, paramTypes);
  Function *F =
      Function::create(FT, Function::InternalLinkage, ast->ident, module_);
  nameValues_->insert(ast->ident, F); // insert into symbol table

  // create entry block
  BasicBlock *entryBB = BasicBlock::create(*F, "entry");
  F->addBasicBlock(entryBB);
  builder_->setInsertPoint(entryBB);

  nameValues_->enterScope(); // parameter scope

  // allocate space for parameters and store arguments
  const auto &args = F->getArgs();
  for (size_t i = 0; i < args.size(); ++i) {
    Argument *arg = args[i];
    const auto &param = ast->params[i];
    arg->setName(param->ident);
    Instruction *alloca = builder_->createAlloca(paramTypes[i], param->ident);
    builder_->createStore(arg, alloca);
    // insert parameter into symbol table
    nameValues_->insert(param->ident, alloca);
  }

  // visit function body
  if (ast->block) {
    ast->block->Accept(*this);
  }

  // implicit return if needed
  BasicBlock *currBB = builder_->getInsertBlock();
  bool hasTerminator = false;
  if (!currBB->getInstList().empty()) {
    const Instruction *last = currBB->getInstList().back().get();
    if (last->getOpcode() == Instruction::Opcode::Ret ||
        last->getOpcode() == Instruction::Opcode::Br ||
        last->getOpcode() == Instruction::Opcode::Jmp) {
      hasTerminator = true;
    }
  }

  if (!hasTerminator) {
    if (retType->isVoidTy()) {
      builder_->createRetVoid();
    } else {
      builder_->createRet(ConstantInt::get(Type::getInt32Ty(), 0));
    }
  }

  nameValues_->exitScope();
}

void IRGenVisitor::visitBlock_(const BlockAST *ast) {
  nameValues_->enterScope();
  for (const auto &item : ast->items) {
    BasicBlock *currBB = builder_->getInsertBlock();
    if (currBB && !currBB->getInstList().empty()) {
      const Instruction *last = currBB->getInstList().back().get();
      if (last->getOpcode() == Instruction::Opcode::Ret ||
          last->getOpcode() == Instruction::Opcode::Br ||
          last->getOpcode() == Instruction::Opcode::Jmp) {
        break;
      }
    }
    item->Accept(*this);
  }
  nameValues_->exitScope();
}

void IRGenVisitor::visitReturnStmt_(const ReturnStmtAST *ast) {
  if (ast->exp) {
    Value *retVal = evalRVal(ast->exp.get());
    builder_->createRet(retVal);
  } else {
    builder_->createRetVoid();
  }
}

void IRGenVisitor::visitVarDecl_(const VarDeclAST *ast) {
  for (const auto &def : ast->var_defs) {
    def->Accept(*this);
  }
}

void IRGenVisitor::visitVarDef_(const VarDefAST *ast) {
  // determine type
  Type *currType = Type::getInt32Ty();
  for (auto it = ast->dims.rbegin(); it != ast->dims.rend(); ++it) {
    int dim = evalConstExpr(it->get());
    currType = Type::getArrayTy(currType, dim);
  }
  Type *finalType = currType;

  if (nameValues_->isGlobal()) {
    // Global variable
    Constant *initializer = nullptr;
    if (!ast->dims.empty()) {
      // global array
      if (ast->init) {
        initializer = initializeGlobalArray(ast->init.get(), finalType);
      } else {
        initializer = ConstantAggregate::getNullValue(finalType);
      }
    } else {
      // global scalar
      if (ast->init && ast->init->initExpr) {
        initializer =
            dynamic_cast<Constant *>(evalRVal(ast->init->initExpr.get()));
      } else {
        // assert(!ast->init); // In SysY global var decl always has init or is
        // 0
        initializer = Constant::getNullValue(finalType);
      }
    }
    GlobalVariable *GV = GlobalVariable::create(finalType, ast->ident, &module_,
                                                initializer, false);
    // insert global variable into symbol table
    nameValues_->insert(ast->ident, GV);
  } else {
    // Local variable
    Instruction *alloca = builder_->createAlloca(finalType, ast->ident);

    if (ast->IsArray()) {
      if (ast->init) {
        initializeLocalArray(ast->init.get(), alloca, finalType);
      }
    } else if (ast->init && ast->init->initExpr) {
      Value *initVal = evalRVal(ast->init->initExpr.get());
      builder_->createStore(initVal, alloca);
    }

    // insert local variable into symbol table
    nameValues_->insert(ast->ident, alloca);
  }
}

void IRGenVisitor::visitAssignStmt_(const AssignStmtAST *ast) {
  // 获取左侧变量地址
  Value *lval = evalLVal(ast->lval.get());
  Value *rval = evalRVal(ast->exp.get());
  builder_->createStore(rval, lval);
}

void IRGenVisitor::visitExpStmt_(const ExpStmtAST *ast) {
  if (ast->exp)
    evalRVal(ast->exp.get());
}

Value *IRGenVisitor::evalBinaryExp(BinaryExpAST *ast) {
  // Short-circuit evaluation for logical AND
  if (ast->op == "&&") {
    Value *lhs = evalRVal(ast->lhs.get());
    if (auto *cL = dynamic_cast<ConstantInt *>(lhs)) {
      if (cL->getValue() == 0) {
        return ConstantInt::get(Type::getInt32Ty(), 0);
      }

      Value *rhs = evalRVal(ast->rhs.get());
      if (auto *cR = dynamic_cast<ConstantInt *>(rhs)) {
        return ConstantInt::get(Type::getInt32Ty(), cR->getValue() != 0);
      }
      return builder_->createBinaryOp(Instruction::Opcode::Ne, rhs,
                                      ConstantInt::get(Type::getInt32Ty(), 0));
    }
    return evalLogicalAnd(ast, lhs);
  }

  // Short-circuit evaluation for logical OR
  if (ast->op == "||") {
    Value *lhs = evalRVal(ast->lhs.get());
    if (auto *cL = dynamic_cast<ConstantInt *>(lhs)) {
      if (cL->getValue() != 0)
        return ConstantInt::get(Type::getInt32Ty(), 1);

      Value *rhs = evalRVal(ast->rhs.get());
      if (auto *cR = dynamic_cast<ConstantInt *>(rhs)) {
        return ConstantInt::get(Type::getInt32Ty(), cR->getValue() != 0);
      }
      return builder_->createBinaryOp(Instruction::Opcode::Ne, rhs,
                                      ConstantInt::get(Type::getInt32Ty(), 0));
    }
    return evalLogicalOr(ast, lhs);
  }

  Value *lhs = evalRVal(ast->lhs.get());
  Value *rhs = evalRVal(ast->rhs.get());

  // Constant folding for binary operations
  if (auto *CL = dynamic_cast<ConstantInt *>(lhs)) {
    if (auto *CR = dynamic_cast<ConstantInt *>(rhs)) {
      int lval = CL->getValue();
      int rval = CR->getValue();
      int res = 0;
      if (ast->op == "+")
        res = lval + rval;
      else if (ast->op == "-")
        res = lval - rval;
      else if (ast->op == "*")
        res = lval * rval;
      else if (ast->op == "/")
        res = (rval == 0) ? 0 : lval / rval;
      else if (ast->op == "%")
        res = (rval == 0) ? 0 : lval % rval;
      else if (ast->op == "<")
        res = lval < rval;
      else if (ast->op == "<=")
        res = lval <= rval;
      else if (ast->op == ">")
        res = lval > rval;
      else if (ast->op == ">=")
        res = lval >= rval;
      else if (ast->op == "==")
        res = lval == rval;
      else if (ast->op == "!=")
        res = lval != rval;
      else
        assert(false && "Unknown binary op in constant folding");
      return ConstantInt::get(Type::getInt32Ty(), res);
    }
  }

  Instruction::Opcode op;
  if (ast->op == "+")
    op = Instruction::Opcode::Add;
  else if (ast->op == "-")
    op = Instruction::Opcode::Sub;
  else if (ast->op == "*")
    op = Instruction::Opcode::Mul;
  else if (ast->op == "/")
    op = Instruction::Opcode::Div;
  else if (ast->op == "%")
    op = Instruction::Opcode::Mod;
  else if (ast->op == "<")
    op = Instruction::Opcode::Lt;
  else if (ast->op == "<=")
    op = Instruction::Opcode::Le;
  else if (ast->op == ">")
    op = Instruction::Opcode::Gt;
  else if (ast->op == ">=")
    op = Instruction::Opcode::Ge;
  else if (ast->op == "==")
    op = Instruction::Opcode::Eq;
  else if (ast->op == "!=")
    op = Instruction::Opcode::Ne;
  else
    assert(false && "Unknown binary op");

  return builder_->createBinaryOp(op, lhs, rhs);
}

ConstantInt *IRGenVisitor::evalNumber(NumberAST *ast) {
  return ConstantInt::get(Type::getInt32Ty(), ast->val);
}

Value *IRGenVisitor::evalLVal(LValAST *ast) {
  Value *val = nameValues_->lookup(ast->ident);
  assert(val && "Undefined variable");

  assert(val->getType() != Type::getInt32Ty() && "scalar cannot be addressed");

  // scalar variable return directly
  if (ast->indices.empty())
    return val;

  // local array or local pointer parameter, local scalar
  // global variable same handling

  // determine if is pointer parameter
  Type *ty = val->getType();
  bool isPtrParam =
      ty->isPointerTy() && ty->getPointerElementType()->isPointerTy();

  Value *ptr = val;
  for (size_t i = 0; i < ast->indices.size(); ++i) {
    Value *idx = evalRVal(ast->indices[i].get());
    if (i == 0 && isPtrParam) {
      ptr = builder_->createLoad(ptr);
      ptr = builder_->createGetPtr(ptr, idx);
    } else {
      ptr = builder_->createGetElemPtr(ptr, idx);
    }
  }
  // ptr is lval, must be pointer
  assert(ptr->getType()->isPointerTy());
  return ptr;
}

Value *IRGenVisitor::evalFuncCall(FuncCallAST *ast) {
  Value *func = nameValues_->lookup(ast->ident);
  assert(func && "Function not found");
  std::vector<Value *> args;
  for (const auto &arg : ast->args) {
    args.push_back(evalRVal(arg.get()));
  }
  return builder_->createCall(func, args);
}

Value *IRGenVisitor::evalRVal(BaseAST *ast) {
  if (auto *lval = dynamic_cast<LValAST *>(ast)) {
    Value *val = nameValues_->lookup(lval->ident);
    assert(val && "Variable not found");

    // const scalar no need to load, return directly
    if (auto *cInt = dynamic_cast<ConstantInt *>(val)) {
      return cInt;
    }

    Value *ptr = evalLVal(lval);

    // 数组指针退化
    if (ptr->getType()->getPointerElementType()->isArrayTy()) {
      Value *zero = ConstantInt::get(Type::getInt32Ty(), 0);
      return builder_->createGetElemPtr(ptr, zero);
    }

    return builder_->createLoad(ptr);

  } else if (auto *num = dynamic_cast<NumberAST *>(ast)) {
    return evalNumber(num);
  } else if (auto *bin = dynamic_cast<BinaryExpAST *>(ast)) {
    return evalBinaryExp(bin);
  } else if (auto *unary = dynamic_cast<UnaryExpAST *>(ast)) {
    return evalUnaryExp(unary);
  } else if (auto *call = dynamic_cast<FuncCallAST *>(ast)) {
    return evalFuncCall(call);
  }
  return nullptr;
}

void IRGenVisitor::visitConstDecl_(const ConstDeclAST *ast) {
  for (const auto &def : ast->const_defs) {
    def->Accept(*this);
  }
}

void IRGenVisitor::visitConstDef_(const ConstDefAST *ast) {
  // determine type
  Type *currType = Type::getInt32Ty();
  for (auto it = ast->dims.rbegin(); it != ast->dims.rend(); ++it) {
    int dim = evalConstExpr(it->get());
    currType = Type::getArrayTy(currType, dim);
  }
  Type *finalType = currType;

  if (nameValues_->isGlobal()) {
    // Global variable
    Constant *initializer = nullptr;
    if (!ast->dims.empty()) {
      // global array
      if (ast->init) {
        initializer = initializeGlobalArray(ast->init.get(), finalType);
      } else {
        initializer = ConstantAggregate::getNullValue(finalType);
      }
    } else {
      // global scalar
      if (ast->init && ast->init->initExpr) {
        initializer =
            dynamic_cast<Constant *>(evalRVal(ast->init->initExpr.get()));
      } else {
        // assert(!ast->init); // In SysY global var decl always has init or is
        // 0
        initializer = Constant::getNullValue(finalType);
      }
    }
    GlobalVariable *GV = GlobalVariable::create(finalType, ast->ident, &module_,
                                                initializer, false);
    // insert global variable into symbol table
    nameValues_->insert(ast->ident, GV);
  } else {
    // Local variable
    if (ast->IsArray()) {
      Instruction *alloca = builder_->createAlloca(finalType, ast->ident);
      if (ast->init) {
        initializeLocalArray(ast->init.get(), alloca, finalType);
      }

      nameValues_->insert(ast->ident, alloca);

    } else if (ast->init && ast->init->initExpr) {
      Value *initVal = evalRVal(ast->init->initExpr.get());

      nameValues_->insert(ast->ident, initVal);
    }
  }
}

void IRGenVisitor::visitIfStmt_(const IfStmtAST *ast) {
  Value *cond = evalRVal(ast->exp.get());
  Function *func = builder_->getInsertBlock()->getParent();
  BasicBlock *thenBB = BasicBlock::create(*func, "then");
  BasicBlock *elseBB = nullptr;
  if (ast->else_stmt)
    elseBB = BasicBlock::create(*func, "else");
  BasicBlock *mergeBB = BasicBlock::create(*func, "if_end");

  if (ast->else_stmt) {
    builder_->createCondBr(cond, thenBB, elseBB);
  } else {
    builder_->createCondBr(cond, thenBB, mergeBB);
  }

  // Then
  func->addBasicBlock(thenBB);
  builder_->setInsertPoint(thenBB);
  ast->then_stmt->Accept(*this);

  BasicBlock *currThenBB = builder_->getInsertBlock();
  if (currThenBB->getInstList().empty() ||
      (currThenBB->getInstList().back()->getOpcode() !=
           Instruction::Opcode::Ret &&
       currThenBB->getInstList().back()->getOpcode() !=
           Instruction::Opcode::Br &&
       currThenBB->getInstList().back()->getOpcode() !=
           Instruction::Opcode::Jmp)) {
    builder_->createJump(mergeBB);
  }

  // Else
  if (ast->else_stmt) {
    func->addBasicBlock(elseBB);
    builder_->setInsertPoint(elseBB);
    ast->else_stmt->Accept(*this);

    BasicBlock *currElseBB = builder_->getInsertBlock();
    if (currElseBB->getInstList().empty() ||
        (currElseBB->getInstList().back()->getOpcode() !=
             Instruction::Opcode::Ret &&
         currElseBB->getInstList().back()->getOpcode() !=
             Instruction::Opcode::Br &&
         currElseBB->getInstList().back()->getOpcode() !=
             Instruction::Opcode::Jmp)) {
      builder_->createJump(mergeBB);
    }
  }

  // Merge
  func->addBasicBlock(mergeBB);
  builder_->setInsertPoint(mergeBB);
}

void IRGenVisitor::visitWhileStmt_(const WhileStmtAST *ast) {
  Function *func = builder_->getInsertBlock()->getParent();
  BasicBlock *condBB = BasicBlock::create(*func, "while_cond");
  BasicBlock *bodyBB = BasicBlock::create(*func, "while_body");
  BasicBlock *endBB = BasicBlock::create(*func, "while_end");

  builder_->createJump(condBB);

  // Condition
  func->addBasicBlock(condBB);
  builder_->setInsertPoint(condBB);
  Value *cond = evalRVal(ast->cond.get());
  builder_->createCondBr(cond, bodyBB, endBB);

  // Body
  func->addBasicBlock(bodyBB);
  builder_->setInsertPoint(bodyBB);

  breakTargets_.push_back(endBB);
  continueTargets_.push_back(condBB);

  ast->body->Accept(*this);

  breakTargets_.pop_back();
  continueTargets_.pop_back();

  BasicBlock *currBodyBB = builder_->getInsertBlock();
  if (currBodyBB->getInstList().empty() ||
      (currBodyBB->getInstList().back()->getOpcode() !=
           Instruction::Opcode::Ret &&
       currBodyBB->getInstList().back()->getOpcode() !=
           Instruction::Opcode::Br &&
       currBodyBB->getInstList().back()->getOpcode() !=
           Instruction::Opcode::Jmp)) {
    builder_->createJump(condBB);
  }

  // End
  func->addBasicBlock(endBB);
  builder_->setInsertPoint(endBB);
}

void IRGenVisitor::visitBreakStmt_(const BreakStmtAST *ast) {
  builder_->createJump(breakTargets_.back());
}

void IRGenVisitor::visitContinueStmt_(const ContinueStmtAST *ast) {
  builder_->createJump(continueTargets_.back());
}

// Lib functions
void IRGenVisitor::registerLibFunctions() {
  auto addLibFunc = [&](const std::string &name, Type *retType,
                        const std::initializer_list<Type *> paramTypes) {
    FunctionType *FT = FunctionType::get(retType, paramTypes);
    Function *F =
        Function::create(FT, Function::ExternalLinkage, name, module_);
    nameValues_->insert(name, F);
  };

  Type *i32 = Type::getInt32Ty();
  Type *voidTy = Type::getVoidTy();
  Type *ptrI32 = Type::getPointerTy(i32);

  addLibFunc("getint", i32, {});
  addLibFunc("getch", i32, {});
  addLibFunc("getarray", i32, {ptrI32});
  addLibFunc("putint", voidTy, {i32});
  addLibFunc("putch", voidTy, {i32});
  addLibFunc("putarray", voidTy, {i32, ptrI32});
  addLibFunc("starttime", voidTy, {});
  addLibFunc("stoptime", voidTy, {});
}

Value *IRGenVisitor::evalUnaryExp(UnaryExpAST *ast) {
  Value *val = evalRVal(ast->exp.get());
  if (ast->op == "+")
    return val;
  if (ast->op == "-")
    return builder_->createBinaryOp(
        Instruction::Opcode::Sub, ConstantInt::get(Type::getInt32Ty(), 0), val);
  if (ast->op == "!")
    return builder_->createBinaryOp(Instruction::Opcode::Eq, val,
                                    ConstantInt::get(Type::getInt32Ty(), 0));
  assert(false && "Unknown unary op");
  return nullptr;
}

Value *IRGenVisitor::evalLogicalAnd(BinaryExpAST *ast, Value *lhsVal) {
  Function *func = builder_->getInsertBlock()->getParent();
  BasicBlock *rhsBB = BasicBlock::create(*func, "and_rhs");
  BasicBlock *endBB = BasicBlock::create(*func, "and_end");

  Value *result = builder_->createAlloca(Type::getInt32Ty());
  Value *lhs = lhsVal ? lhsVal : evalRVal(ast->lhs.get());
  builder_->createStore(ConstantInt::get(Type::getInt32Ty(), 0), result);

  // if lhs is true, check rhs. else result is 0.
  builder_->createCondBr(lhs, rhsBB, endBB);

  func->addBasicBlock(rhsBB);
  builder_->setInsertPoint(rhsBB);
  Value *rhs = evalRVal(ast->rhs.get());
  Value *rhsBool = builder_->createBinaryOp(
      Instruction::Opcode::Ne, rhs, ConstantInt::get(Type::getInt32Ty(), 0));
  builder_->createStore(rhsBool, result);
  builder_->createJump(endBB);

  func->addBasicBlock(endBB);
  builder_->setInsertPoint(endBB);
  return builder_->createLoad(result);
}

Value *IRGenVisitor::evalLogicalOr(BinaryExpAST *ast, Value *lhsVal) {
  Function *func = builder_->getInsertBlock()->getParent();
  BasicBlock *rhsBB = BasicBlock::create(*func, "or_rhs");
  BasicBlock *endBB = BasicBlock::create(*func, "or_end");

  Value *result = builder_->createAlloca(Type::getInt32Ty());
  Value *lhs = lhsVal ? lhsVal : evalRVal(ast->lhs.get());
  builder_->createStore(ConstantInt::get(Type::getInt32Ty(), 1), result);

  // if lhs is true, result is 1. else check rhs.
  builder_->createCondBr(lhs, endBB, rhsBB);

  func->addBasicBlock(rhsBB);
  builder_->setInsertPoint(rhsBB);
  Value *rhs = evalRVal(ast->rhs.get());
  Value *rhsBool = builder_->createBinaryOp(
      Instruction::Opcode::Ne, rhs, ConstantInt::get(Type::getInt32Ty(), 0));
  builder_->createStore(rhsBool, result);
  builder_->createJump(endBB);

  func->addBasicBlock(endBB);
  builder_->setInsertPoint(endBB);
  return builder_->createLoad(result);
}

int IRGenVisitor::evalConstExpr(const BaseAST *ast) {
  if (const auto *num = dynamic_cast<const NumberAST *>(ast)) {
    return num->val;
  } else if (const auto *bin = dynamic_cast<const BinaryExpAST *>(ast)) {
    int lhs = evalConstExpr(bin->lhs.get());
    int rhs = evalConstExpr(bin->rhs.get());
    if (bin->op == "+")
      return lhs + rhs;
    if (bin->op == "-")
      return lhs - rhs;
    if (bin->op == "*")
      return lhs * rhs;
    if (bin->op == "/")
      return rhs ? lhs / rhs : 0;
    if (bin->op == "%")
      return rhs ? lhs % rhs : 0;
    if (bin->op == "<")
      return lhs < rhs;
    if (bin->op == "<=")
      return lhs <= rhs;
    if (bin->op == ">")
      return lhs > rhs;
    if (bin->op == ">=")
      return lhs >= rhs;
    if (bin->op == "==")
      return lhs == rhs;
    if (bin->op == "!=")
      return lhs != rhs;
  } else if (const auto *unary = dynamic_cast<const UnaryExpAST *>(ast)) {
    int val = evalConstExpr(unary->exp.get());
    if (unary->op == "+")
      return val;
    if (unary->op == "-")
      return -val;
    if (unary->op == "!")
      return val == 0;
  } else if (const auto *lval = dynamic_cast<const LValAST *>(ast)) {
    Value *val = nameValues_->lookup(lval->ident);
    assert(val && "Variable not found");

    if (auto *cInt = dynamic_cast<ConstantInt *>(val)) {
      return cInt->getValue();
    }

    if (auto *GV = dynamic_cast<GlobalVariable *>(val)) {
      Constant *curr = GV->getInit();
      if (!curr)
        assert(false && "Global variable has no initializer");

      for (const auto &idx : lval->indices) {
        int idxVal = evalConstExpr(idx.get());
        if (auto *cArr = dynamic_cast<ConstantArray *>(curr)) {
          curr = dynamic_cast<Constant *>(cArr->getOperand(idxVal));
        } else if (dynamic_cast<ConstantZero *>(curr)) {
          return 0;
        } else {
          assert(false && "Indexing non-array type");
        }
      }

      if (auto *cInt = dynamic_cast<ConstantInt *>(curr)) {
        return cInt->getValue();
      } else if (dynamic_cast<ConstantZero *>(curr)) {
        return 0;
      }
      assert(false && "Not an integer constant");
    }
    assert(false && "Not a constant lval");
  } else if (dynamic_cast<const FuncCallAST *>(ast)) {
    assert(false && "Function calls not supported in constant expressions");
  }
  assert(false);
  return 0; // TODO: Implement full constant folding
}

void IRGenVisitor::Visit(CompUnitAST &node) { visitCompUnit_(&node); }
void IRGenVisitor::Visit(FuncFParamAST &node) { assert(false); }
void IRGenVisitor::Visit(FuncDefAST &node) { visitFuncDef_(&node); }
void IRGenVisitor::Visit(BlockAST &node) { visitBlock_(&node); }
void IRGenVisitor::Visit(ConstDeclAST &node) { visitConstDecl_(&node); }
void IRGenVisitor::Visit(ConstDefAST &node) { visitConstDef_(&node); }
void IRGenVisitor::Visit(VarDeclAST &node) { visitVarDecl_(&node); }
void IRGenVisitor::Visit(VarDefAST &node) { visitVarDef_(&node); }
void IRGenVisitor::Visit(InitVarAST &node) { assert(false); }
void IRGenVisitor::Visit(AssignStmtAST &node) { visitAssignStmt_(&node); }
void IRGenVisitor::Visit(ExpStmtAST &node) { visitExpStmt_(&node); }
void IRGenVisitor::Visit(IfStmtAST &node) { visitIfStmt_(&node); }
void IRGenVisitor::Visit(WhileStmtAST &node) { visitWhileStmt_(&node); }
void IRGenVisitor::Visit(BreakStmtAST &node) { visitBreakStmt_(&node); }
void IRGenVisitor::Visit(ContinueStmtAST &node) { visitContinueStmt_(&node); }
void IRGenVisitor::Visit(ReturnStmtAST &node) { visitReturnStmt_(&node); }
void IRGenVisitor::Visit(LValAST &node) { assert(false); }
void IRGenVisitor::Visit(NumberAST &node) { assert(false); }
void IRGenVisitor::Visit(UnaryExpAST &node) { assert(false); }
void IRGenVisitor::Visit(BinaryExpAST &node) { assert(false); }
void IRGenVisitor::Visit(FuncCallAST &node) { assert(false); }

Constant *IRGenVisitor::initializeGlobalArray(const InitVarAST *init,
                                              Type *type) {
  // 1. Calculate dimensions
  std::vector<int> dims;
  Type *tmp = type;
  while (tmp->isArrayTy()) {
    dims.push_back(tmp->getArrayNumElements());
    tmp = tmp->getArrayElementType();
  }
  int total_elements = 1;
  for (int d : dims)
    total_elements *= d;

  // 2. Flatten AST values to integer constants
  std::vector<int> vals;
  std::function<void(const InitVarAST *, int)> flatten =
      [&](const InitVarAST *curr, int depth) {
        if (curr->initExpr) {
          vals.push_back(evalConstExpr(curr->initExpr.get()));
          return;
        }
        for (const auto &child : curr->initList) {
          if (child->initExpr) {
            vals.push_back(evalConstExpr(child->initExpr.get()));
          } else {
            flatten(child.get(), depth + 1);
          }
        }
      };
  flatten(init, 0);

  // Padding
  while (vals.size() < total_elements)
    vals.push_back(0);

  // 3. Reconstruct ConstantArray recursively
  int current_offset = 0;
  std::function<Constant *(Type *)> build_const =
      [&](Type *currType) -> Constant * {
    if (currType->isIntegerTy()) {
      return ConstantInt::get(currType, vals[current_offset++]);
    }
    ArrayType *arrTy = dynamic_cast<ArrayType *>(currType);
    std::vector<Constant *> elements;
    for (int i = 0; i < arrTy->getNumElements(); ++i) {
      elements.push_back(build_const(arrTy->getElementType()));
    }
    return ConstantArray::get(arrTy, elements);
  };

  return build_const(type);
}

void IRGenVisitor::initializeLocalArray(const InitVarAST *init, Value *baseAddr,
                                        Type *type) {
  std::vector<int> dims;
  Type *tmp = type;
  while (tmp->isArrayTy()) {
    dims.push_back(tmp->getArrayNumElements());
    tmp = tmp->getArrayElementType();
  }

  int total_elements = 1;
  for (int d : dims)
    total_elements *= d;

  std::vector<Value *> initValues;
  // Recursive lambda to flatten init list
  std::function<void(const InitVarAST *, int)> flatten =
      [&](const InitVarAST *curr, int depth) {
        if (curr->initExpr) {
          initValues.push_back(evalRVal(curr->initExpr.get()));
          return;
        }

        // Braced list
        // If we are at a depth where we expect elements, we iterate
        for (const auto &child : curr->initList) {
          if (child->initExpr) {
            // Scalar inside braces, just push
            initValues.push_back(evalRVal(child->initExpr.get()));
          } else {
            // Nested braces, recurse
            // Simple heuristic: if we are supposed to go deeper, we do.
            // SysY structural equivalence is looser, but let's trust AST
            // structure roughly matches
            flatten(child.get(), depth + 1);
          }
        }
      };

  // Start flattening
  flatten(init, 0);

  // Fill remaining with zero
  while (initValues.size() < total_elements) {
    initValues.push_back(ConstantInt::get(Type::getInt32Ty(), 0));
  }

  // Store values into array
  std::vector<int> current_idx(dims.size(), 0);
  for (int i = 0; i < total_elements; ++i) {
    Value *val = initValues[i];
    if (val->getType()->isIntegerTy()) {
      // Check bitwidth if strict, for now assume i32
    }

    // Generate GEP to element
    Value *ptr = baseAddr;
    for (int d = 0; d < dims.size(); ++d) {
      ptr = builder_->createGetElemPtr(
          ptr, ConstantInt::get(Type::getInt32Ty(), current_idx[d]));
    }
    builder_->createStore(val, ptr);

    // Increment multi-dim index
    for (int d = dims.size() - 1; d >= 0; --d) {
      current_idx[d]++;
      if (current_idx[d] < dims[d])
        break;
      current_idx[d] = 0;
    }
  }
}

Constant *IRGenVisitor::evalConstant(const InitVarAST *init, Type *ty) {
  if (init->initExpr) {
    return dynamic_cast<Constant *>(evalRVal(init->initExpr.get()));
  }

  ArrayType *arrTy = dynamic_cast<ArrayType *>(ty);
  if (!arrTy)
    return Constant::getNullValue(ty);

  std::vector<Constant *> values;
  for (const auto &subInit : init->initList) {
    values.push_back(evalConstant(subInit.get(), arrTy->getElementType()));
  }
  while (values.size() < arrTy->getNumElements()) {
    values.push_back(Constant::getNullValue(arrTy->getElementType()));
  }
  return ConstantArray::get(arrTy, values);
}

} // namespace nanocc
