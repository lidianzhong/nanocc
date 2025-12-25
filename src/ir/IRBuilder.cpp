#include <cassert>
#include <iostream>

#include "ir/IRBuilder.h"

koopa_raw_program_t IRBuilder::Build() {
  std::string ir = buffer_.str();
  koopa_program_t program = nullptr;
  koopa_error_code_t ret = koopa_parse_from_string(ir.c_str(), &program);
  assert(ret == KOOPA_EC_SUCCESS);

  auto builder = koopa_new_raw_program_builder();
  return koopa_build_raw_program(builder, program);
}

void IRBuilder::StartFunction(const std::string &name,
                              const std::string &ret_type) {
  buffer_ << "fun @" << name << "(): " << ret_type << " {" << std::endl;
}

void IRBuilder::EndFunction() { buffer_ << "}" << std::endl; }

void IRBuilder::CreateBasicBlock(const std::string &label) {
  buffer_ << "%" << label << ":" << std::endl;
}

std::string IRBuilder::CreateAlloca(const std::string &type,
                                    const std::string &var_name) {
  std::string alloc_reg = NewTempReg_(var_name);
  buffer_ << "  " << alloc_reg << " = alloc " << type << std::endl;
  return alloc_reg;
}

std::string IRBuilder::CreateLoad(const std::string &alloc) {
  std::string load_reg = NewTempReg_();
  buffer_ << "  " << load_reg << " = load " << alloc << std::endl;
  return load_reg;
}

void IRBuilder::CreateStore(const std::string &value,
                            const std::string &alloc) {
  buffer_ << "  store " << value << ", " << alloc << std::endl;
}

std::string IRBuilder::CreateUnaryOp(const std::string &op,
                                     const std::string &value) {
  std::string result_reg = NewTempReg_();

  if (op == "-") {
    // 取负：sub 0, value
    buffer_ << "  " << result_reg << " = sub 0, " << value << std::endl;
  } else if (op == "!") {
    // 逻辑非：eq value, 0
    buffer_ << "  " << result_reg << " = eq " << value << ", 0" << std::endl;
  } else if (op == "+") {
    // 正号：直接返回原值
    return value;
  } else {
    // 未知操作符
    std::cerr << "Unknown unary operator: " << op << std::endl;
    return "";
  }

  return result_reg;
}

std::string IRBuilder::CreateBinaryOp(const std::string &op,
                                      const std::string &lhs,
                                      const std::string &rhs) {
  std::string result_reg = NewTempReg_();
  std::string ir_op;

  if (op == "+") {
    ir_op = "add";
  } else if (op == "-") {
    ir_op = "sub";
  } else if (op == "*") {
    ir_op = "mul";
  } else if (op == "/") {
    ir_op = "div";
  } else if (op == "%") {
    ir_op = "mod";
  } else if (op == "<") {
    ir_op = "lt";
  } else if (op == ">") {
    ir_op = "gt";
  } else if (op == "<=") {
    ir_op = "le";
  } else if (op == ">=") {
    ir_op = "ge";
  } else if (op == "==") {
    ir_op = "eq";
  } else if (op == "!=") {
    ir_op = "ne";
  } else if (op == "&&") {
    // 逻辑与：需要先转换为布尔值，然后使用 and
    std::string temp1 = NewTempReg_();
    buffer_ << "  " << temp1 << " = ne " << lhs << ", 0" << std::endl;
    std::string temp2 = NewTempReg_();
    buffer_ << "  " << temp2 << " = ne " << rhs << ", 0" << std::endl;
    buffer_ << "  " << result_reg << " = and " << temp1 << ", " << temp2
            << std::endl;
    return result_reg;
  } else if (op == "||") {
    // 逻辑或：需要先转换为布尔值，然后使用 or
    std::string temp1 = NewTempReg_();
    buffer_ << "  " << temp1 << " = ne " << lhs << ", 0" << std::endl;
    std::string temp2 = NewTempReg_();
    buffer_ << "  " << temp2 << " = ne " << rhs << ", 0" << std::endl;
    buffer_ << "  " << result_reg << " = or " << temp1 << ", " << temp2
            << std::endl;
    return result_reg;
  } else {
    std::cerr << "Unknown binary operator: " << op << std::endl;
    return "";
  }

  buffer_ << "  " << result_reg << " = " << ir_op << " " << lhs << ", " << rhs
          << std::endl;
  return result_reg;
}

std::string IRBuilder::CreateNumber(int value) { return std::to_string(value); }

void IRBuilder::CreateReturn(const std::string &value) {
  if (value.empty()) {
    buffer_ << "  ret" << std::endl;
  } else {
    buffer_ << "  ret " << value << std::endl;
  }
}

std::string IRBuilder::NewTempReg_(const std::string &var_name) {
  if (!var_name.empty()) {
    int &counter = var_reg_counters_[var_name];
    return "@" + var_name + "_" + std::to_string(++counter);
  } else {
    return "%" + std::to_string(temp_reg_id_++);
  }
}