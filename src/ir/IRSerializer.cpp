#include "ir/IRSerializer.h"

#include <cassert>
#include <sstream>

namespace IRSerializer {

// 从 Operand 中获取 Value 的字符串表示
static std::string OperandToString(const Operand &op) {
  if (std::holds_alternative<Value>(op)) {
    return std::get<Value>(op).toString();
  }
  assert(false && "OperandToString: unexpected operand type");
  return "";
}

// 将 BranchTarget 转换为字符串（包含块参数）
static std::string BranchTargetToString(const BranchTarget &target) {
  std::ostringstream oss;
  oss << "%" << target.target->name;
  if (!target.args.empty()) {
    oss << "(";
    for (size_t i = 0; i < target.args.size(); i++) {
      if (i > 0)
        oss << ", ";
      oss << target.args[i].toString();
    }
    oss << ")";
  }
  return oss.str();
}

// 将单条指令转换为 IR 文本
static std::string InstructionToString(const Instruction &inst) {
  std::ostringstream oss;

  switch (inst.op) {
  case Opcode::Add:
    oss << "  " << OperandToString(inst.args[0]) << " = add "
        << OperandToString(inst.args[1]) << ", "
        << OperandToString(inst.args[2]);
    break;
  case Opcode::Sub:
    oss << "  " << OperandToString(inst.args[0]) << " = sub "
        << OperandToString(inst.args[1]) << ", "
        << OperandToString(inst.args[2]);
    break;
  case Opcode::Mul:
    oss << "  " << OperandToString(inst.args[0]) << " = mul "
        << OperandToString(inst.args[1]) << ", "
        << OperandToString(inst.args[2]);
    break;
  case Opcode::Div:
    oss << "  " << OperandToString(inst.args[0]) << " = div "
        << OperandToString(inst.args[1]) << ", "
        << OperandToString(inst.args[2]);
    break;
  case Opcode::Mod:
    oss << "  " << OperandToString(inst.args[0]) << " = mod "
        << OperandToString(inst.args[1]) << ", "
        << OperandToString(inst.args[2]);
    break;
  case Opcode::Lt:
    oss << "  " << OperandToString(inst.args[0]) << " = lt "
        << OperandToString(inst.args[1]) << ", "
        << OperandToString(inst.args[2]);
    break;
  case Opcode::Gt:
    oss << "  " << OperandToString(inst.args[0]) << " = gt "
        << OperandToString(inst.args[1]) << ", "
        << OperandToString(inst.args[2]);
    break;
  case Opcode::Le:
    oss << "  " << OperandToString(inst.args[0]) << " = le "
        << OperandToString(inst.args[1]) << ", "
        << OperandToString(inst.args[2]);
    break;
  case Opcode::Ge:
    oss << "  " << OperandToString(inst.args[0]) << " = ge "
        << OperandToString(inst.args[1]) << ", "
        << OperandToString(inst.args[2]);
    break;
  case Opcode::Eq:
    oss << "  " << OperandToString(inst.args[0]) << " = eq "
        << OperandToString(inst.args[1]) << ", "
        << OperandToString(inst.args[2]);
    break;
  case Opcode::Ne:
    oss << "  " << OperandToString(inst.args[0]) << " = ne "
        << OperandToString(inst.args[1]) << ", "
        << OperandToString(inst.args[2]);
    break;
  case Opcode::And:
    oss << "  " << OperandToString(inst.args[0]) << " = and "
        << OperandToString(inst.args[1]) << ", "
        << OperandToString(inst.args[2]);
    break;
  case Opcode::Or:
    oss << "  " << OperandToString(inst.args[0]) << " = or "
        << OperandToString(inst.args[1]) << ", "
        << OperandToString(inst.args[2]);
    break;
  case Opcode::Alloc:
    oss << "  " << OperandToString(inst.args[0]) << " = alloc i32";
    break;
  case Opcode::Load:
    oss << "  " << OperandToString(inst.args[0]) << " = load "
        << OperandToString(inst.args[1]);
    break;
  case Opcode::Store:
    oss << "  store " << OperandToString(inst.args[0]) << ", "
        << OperandToString(inst.args[1]);
    break;
  case Opcode::Br: {
    // br cond, then_target, else_target
    const auto &then_target = std::get<BranchTarget>(inst.args[1]);
    const auto &else_target = std::get<BranchTarget>(inst.args[2]);
    oss << "  br " << OperandToString(inst.args[0]) << ", "
        << BranchTargetToString(then_target) << ", "
        << BranchTargetToString(else_target);
    break;
  }
  case Opcode::Jmp: {
    // jump target
    const auto &target = std::get<BranchTarget>(inst.args[0]);
    oss << "  jump " << BranchTargetToString(target);
    break;
  }
  case Opcode::Ret:
    if (inst.args.empty()) {
      oss << "  ret";
    } else {
      oss << "  ret " << OperandToString(inst.args[0]);
    }
    break;
  case Opcode::Call:
    if (inst.args.size() == 3) {
      // ret_reg = call func_name, args
      oss << "  " << OperandToString(inst.args[0]) << " = call "
          << std::get<std::string>(inst.args[1]) << "(";
      const auto &arg_values = std::get<std::vector<Value>>(inst.args[2]);
      for (size_t i = 0; i < arg_values.size(); i++) {
        if (i > 0)
          oss << ", ";
        oss << arg_values[i].toString();
      }
      oss << ")";
    } else {
      // call func_name, args
      oss << "  call " << std::get<std::string>(inst.args[0]) << "(";
      const auto &arg_values = std::get<std::vector<Value>>(inst.args[1]);
      for (size_t i = 0; i < arg_values.size(); i++) {
        if (i > 0)
          oss << ", ";
        oss << arg_values[i].toString();
      }
      oss << ")";
    }
    break;
  }

  return oss.str();
}

// 将基本块头转换为字符串（包含块参数声明）
static std::string BlockHeaderToString(const BasicBlock &bb) {
  std::ostringstream oss;
  oss << "%" << bb.name;
  if (!bb.params.empty()) {
    oss << "(";
    for (size_t i = 0; i < bb.params.size(); i++) {
      if (i > 0)
        oss << ", ";
      oss << bb.params[i].first << ": " << bb.params[i].second;
    }
    oss << ")";
  }
  oss << ":";
  return oss.str();
}

// 将单个函数转换为 IR 文本
static std::string FunctionToString(const Function &func) {
  std::ostringstream oss;

  // 函数头
  oss << "fun " << func.name << "(";
  for (size_t i = 0; i < func.param_names.size(); i++) {
    if (i > 0)
      oss << ", ";
    oss << func.param_names[i] << ": i32";
  }
  if (func.ret_type.empty()) {
    oss << ") {\n";
  } else {
    oss << "): " << func.ret_type << " {\n";
  }

  // 遍历所有基本块
  for (const auto &bb : func.blocks) {
    oss << "\n" << BlockHeaderToString(*bb) << "\n";

    // 遍历基本块中的所有指令
    for (const auto &inst : bb->insts) {
      oss << InstructionToString(*inst) << "\n";
    }
  }

  oss << "}\n";

  return oss.str();
}

std::string ToIR(const IRModule &module) {
  std::ostringstream oss;

  for (const auto &func : module.GetFunctions()) {
    oss << FunctionToString(*func);
  }

  return oss.str();
}

koopa_raw_program_t ToProgram(const std::string &ir) {
  koopa_program_t program = nullptr;
  koopa_error_code_t ret = koopa_parse_from_string(ir.c_str(), &program);
  assert(ret == KOOPA_EC_SUCCESS);

  auto builder = koopa_new_raw_program_builder();
  return koopa_build_raw_program(builder, program);
}

koopa_raw_program_t ToProgram(const IRModule &module) {
  std::string ir = ToIR(module);
  return ToProgram(ir);
}

} // namespace IRSerializer
