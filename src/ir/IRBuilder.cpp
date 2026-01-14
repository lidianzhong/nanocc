#include "ir/IRBuilder.h"
#include "ir/IR.h"

#include <cassert>
#include <iostream>

void IRBuilder::SetCurrentFunction(Function *func) { cur_func_ = func; }

void IRBuilder::EndFunction() {
  cur_func_ = nullptr;
  cur_bb_ = nullptr;
}

BasicBlock *IRBuilder::CreateBlock(const std::string &name) {
  std::string block_name = name == "entry" ? name : NewTempLabel_(name);
  return cur_func_->CreateBlock(block_name);
}

void IRBuilder::SetInsertPoint(BasicBlock *bb) { cur_bb_ = bb; }

/**
 * @addr = alloc type
 */
Value IRBuilder::CreateAlloca(const std::string &type,
                              const std::string &var_name, bool unique) {
  Value addr = NewTempAddr_(var_name, unique);
  Emit(Opcode::Alloc, addr);
  return addr;
}

/**
 * @reg = load @addr
 */
Value IRBuilder::CreateLoad(const Value &addr) {
  assert(addr.isAddress() && "CreateLoad expects an address");
  Value reg = NewTempReg_();
  Emit(Opcode::Load, reg, addr);
  return reg;
}

/**
 * store (imm | @reg), @addr
 */
void IRBuilder::CreateStore(const Value &value, const Value &addr) {
  assert(addr.isAddress() && "CreateStore expects an address");

  Value src_val = value;

  // 如果是地址，先load
  if (value.isAddress()) {
    src_val = CreateLoad(value);
  }

  Emit(Opcode::Store, src_val, addr);
}

/**
 * @input: cond (imm | @reg | @addr)
 *
 * br @cond_reg, %then_bb [args], %else_bb [args]
 */
void IRBuilder::CreateBranch(const Value &cond, BasicBlock *then_bb,
                             const std::vector<Value> &then_args,
                             BasicBlock *else_bb,
                             const std::vector<Value> &else_args) {

  Value cond_reg;

  if (cond.isImmediate()) {
    // 立即数非0为true，0为false
    cond_reg = CreateBinaryOp("!=", cond, Value::Imm(0));
  } else if (cond.isRegister()) {
    cond_reg = cond;
  } else if (cond.isAddress()) {
    // 地址先load，load结果作为条件
    cond_reg = CreateLoad(cond);
  }

  Emit(Opcode::Br, cond_reg, BranchTarget(then_bb, then_args),
       BranchTarget(else_bb, else_args));
}

/**
 * jmp @target_bb [args]
 */
void IRBuilder::CreateJump(BasicBlock *target_bb,
                           const std::vector<Value> &args) {
  Emit(Opcode::Jmp, BranchTarget(target_bb, args));
}

/**
 * ret (imm | @reg | @addr)
 */
void IRBuilder::CreateReturn(const Value &value) {
  Value ret_val = value;

  if (value.isAddress()) {
    // 如果是地址，先load
    ret_val = CreateLoad(value);
  }

  Emit(Opcode::Ret, ret_val);
}

/**
 * ret
 */
void IRBuilder::CreateReturn() { Emit(Opcode::Ret); }

/**
 * @input: value (imm | @reg | @addr)
 *
 * @res_reg = unary_op val_reg (imm | @reg | @addr)
 */
Value IRBuilder::CreateUnaryOp(const std::string &op, const Value &value) {
  if (op == "+") {
    return value;
  }

  // 常量折叠
  if (value.isImmediate()) {
    if (op == "-") {
      return Value::Imm(-value.imm);
    } else if (op == "!") {
      return Value::Imm(value.imm == 0 ? 1 : 0);
    }
  }

  Value val_reg;
  if (value.isAddress()) {
    // 如果是地址，先load，load结果作为操作数
    val_reg = CreateLoad(value);
  } else if (value.isRegister()) {
    val_reg = value;
  };

  Value res_reg = NewTempReg_();
  if (op == "-") {
    Emit(Opcode::Sub, res_reg, Value::Imm(0), val_reg);
  } else if (op == "!") {
    Emit(Opcode::Eq, res_reg, val_reg, Value::Imm(0));
  } else {
    std::cerr << "Unknown unary operator: " << op << std::endl;
    assert(false);
  }

  return res_reg;
}

/**
 * @input: lhs (imm | @reg | @addr), rhs (imm | @reg | @addr)
 *
 * @res_reg = binary_op lhs_reg rhs_reg (imm | @reg)
 */
Value IRBuilder::CreateBinaryOp(const std::string &op, const Value &lhs,
                                const Value &rhs) {

  // 如果是地址，先load
  Value lhs_reg = lhs;
  Value rhs_reg = rhs;
  if (lhs.isAddress()) {
    lhs_reg = CreateLoad(lhs);
  }
  if (rhs.isAddress()) {
    rhs_reg = CreateLoad(rhs);
  }

  Opcode opcode;
  if (op == "+")
    opcode = Opcode::Add;
  else if (op == "-")
    opcode = Opcode::Sub;
  else if (op == "*")
    opcode = Opcode::Mul;
  else if (op == "/")
    opcode = Opcode::Div;
  else if (op == "%")
    opcode = Opcode::Mod;
  else if (op == "<")
    opcode = Opcode::Lt;
  else if (op == ">")
    opcode = Opcode::Gt;
  else if (op == "<=")
    opcode = Opcode::Le;
  else if (op == ">=")
    opcode = Opcode::Ge;
  else if (op == "==")
    opcode = Opcode::Eq;
  else if (op == "!=")
    opcode = Opcode::Ne;
  else if (op == "&&")
    opcode = Opcode::And;
  else if (op == "||")
    opcode = Opcode::Or;
  else {
    std::cerr << "Unknown binary operator: " << op << std::endl;
    assert(false);
  }

  Value res_reg = NewTempReg_();
  Emit(opcode, res_reg, lhs_reg, rhs_reg);
  return res_reg;
}

Value IRBuilder::CreateCall(const std::string &func_name,
                            const std::vector<Value> &args, bool has_return) {
  if (has_return) {
    Value ret_reg = NewTempReg_();
    Emit(Opcode::Call, ret_reg, func_name, args);
    return ret_reg;
  } else {
    Emit(Opcode::Call, func_name, args);
    return Value::Reg(""); // undefined
  }
}

Value IRBuilder::NewTempReg_() {
  return Value::Reg("%" + std::to_string(temp_reg_id_++));
}

Value IRBuilder::NewTempAddr_(const std::string &addr_name, bool unique) {
  assert(!addr_name.empty() && "new addr name cannot be empty");
  std::string final_name = addr_name;
  if (unique) {
    int &counter = temp_addr_counters_[addr_name];
    final_name = addr_name + "_" + std::to_string(++counter);
    ;
  }
  return Value::Addr("@" + final_name);
}

std::string IRBuilder::NewTempLabel_(const std::string &prefix) {
  int &counter = temp_label_counters_[prefix];
  return prefix + "_" + std::to_string(++counter);
}
