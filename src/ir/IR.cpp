#include "ir/IR.h"
#include "frontend/SymbolTable.h" // 此时包含是安全的
#include <cstdint>

std::string Value::toString() const {
  if (kind == ValueKind::Immediate)
    return std::to_string(imm);
  return reg_or_addr;
}

Value Value::Imm(int32_t imm) { return {ValueKind::Immediate, "", imm}; }

Value Value::Reg(std::string reg_or_addr) {
  return {ValueKind::Register, reg_or_addr, 0};
}

Value Value::Addr(std::string reg_or_addr) {
  return {ValueKind::Address, reg_or_addr, 0};
}

Value Value::Imm(const symbol_t &symbol) {
  return Value::Imm(std::get<int>(symbol.value));
}

Value Value::Addr(const symbol_t &symbol) {
  return Value::Addr(std::get<std::string>(symbol.value));
}

BasicBlock::BasicBlock(Function *func, std::string name)
    : func(func), name(std::move(name)) {}

bool BasicBlock::HasTerminator() const {
  if (insts.empty())
    return false;
  auto op = insts.back()->op;
  return op == Opcode::Br || op == Opcode::Jmp || op == Opcode::Ret;
}

Value BasicBlock::AddParam(const std::string &type) {
  std::string param_name = "%" + name + "_arg" + std::to_string(params.size());
  params.push_back({param_name, type});
  return Value::Reg(param_name);
}

void BasicBlock::Append(std::unique_ptr<Instruction> inst) {
  insts.push_back(std::move(inst));
}

BasicBlock *BasicBlock::Create(Function *func, std::string name) {
  return new BasicBlock(func, std::move(name));
}

Function::Function(std::string name, std::string ret_type,
                   std::vector<std::string> p_names)
    : name("@" + std::move(name)), ret_type(std::move(ret_type)) {
  for (auto &p_name : p_names) {
    p_name = "%" + p_name;
  }
  param_names = std::move(p_names);
}

BasicBlock *Function::CreateBlock(const std::string &block_name) {
  blocks.push_back(std::make_unique<BasicBlock>(this, block_name));
  return blocks.back().get();
}