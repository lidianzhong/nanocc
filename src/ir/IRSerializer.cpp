#include "ir/IRSerializer.h"
#include "ir/BasicBlock.h"
#include "ir/Constant.h"
#include "ir/Function.h"
#include "ir/GlobalVariable.h"
#include "ir/Instruction.h"
#include "ir/Module.h"
#include "ir/Type.h"
#include "ir/Value.h"

#include <cassert>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace nanocc {
namespace IRSerializer {

class SlotTracker {
  std::map<const Value *, int> valMap_;
  int nextID_ = 0;

public:
  void reset() {
    valMap_.clear();
    nextID_ = 0;
  }

  int getID(const Value *v) {
    if (valMap_.find(v) == valMap_.end()) {
      valMap_[v] = nextID_++;
    }
    return valMap_[v];
  }

  void assignIDs(const Function *F) {
    reset();
    for (auto arg : F->getArgs()) {
      getID(arg);
    }
    for (auto bb : F->getBasicBlockList()) {
      if (bb->getName().empty()) {
        getID(bb);
      }
      for (const auto &inst : bb->getInstList()) {
        if (!inst->getType()->isVoidTy()) {
          getID(inst.get());
        }
      }
    }
  }
};

static SlotTracker tracker;

static std::string getValName(const Value *v) {
  if (auto *c = dynamic_cast<const ConstantInt *>(v)) {
    return std::to_string(c->getValue());
  }
  if (auto *func = dynamic_cast<const Function *>(v)) {
    return "@" + func->getName();
  }
  if (auto *gv = dynamic_cast<const GlobalVariable *>(v)) {
    return "@" + gv->getName();
  }
  if (auto *bb = dynamic_cast<const BasicBlock *>(v)) {
    if (!bb->getName().empty())
      return "%" + bb->getName();
    return "%" + std::to_string(tracker.getID(bb));
  }
  if (auto arg = dynamic_cast<const Argument *>(v)) {
    if (!arg->getName().empty())
      return "%" + arg->getName();
  }
  return "%" + std::to_string(tracker.getID(v));
}

static void SerializeConstant(const Constant *c, std::ostream &os) {
  if (auto *ci = dynamic_cast<const ConstantInt *>(c)) {
    os << ci->getValue();
  } else if (dynamic_cast<const ConstantZero *>(c)) {
    os << "zeroinit";
  } else if (auto *ca = dynamic_cast<const ConstantArray *>(c)) {
    os << "{";
    for (unsigned i = 0; i < ca->getNumOperands(); ++i) {
      if (i > 0)
        os << ", ";
      SerializeConstant(dynamic_cast<const Constant *>(ca->getOperand(i)), os);
    }
    os << "}";
  } else {
    os << "zeroinit";
  }
}

static void SerializeInstruction(const Instruction *inst, std::ostream &os) {
  if (!inst->getType()->isVoidTy()) {
    os << "  " << getValName(inst) << " = ";
  } else {
    os << "  ";
  }

  switch (inst->getOpcode()) {
  case Instruction::Opcode::Alloc:
  case Instruction::Opcode::AllocArray:
    if (auto *ptrTy = dynamic_cast<PointerType *>(inst->getType())) {
      os << "alloc " << ptrTy->getElementType()->toString();
    }
    break;
  case Instruction::Opcode::Load:
    os << "load " << getValName(inst->getOperand(0));
    break;
  case Instruction::Opcode::Store:
    os << "store " << getValName(inst->getOperand(0)) << ", "
       << getValName(inst->getOperand(1));
    break;
  case Instruction::Opcode::GetElemPtr:
    os << "getelemptr " << getValName(inst->getOperand(0)) << ", "
       << getValName(inst->getOperand(1));
    break;
  case Instruction::Opcode::GetPtr:
    os << "getptr " << getValName(inst->getOperand(0)) << ", "
       << getValName(inst->getOperand(1));
    break;
  case Instruction::Opcode::Add:
    os << "add " << getValName(inst->getOperand(0)) << ", "
       << getValName(inst->getOperand(1));
    break;
  case Instruction::Opcode::Sub:
    os << "sub " << getValName(inst->getOperand(0)) << ", "
       << getValName(inst->getOperand(1));
    break;
  case Instruction::Opcode::Mul:
    os << "mul " << getValName(inst->getOperand(0)) << ", "
       << getValName(inst->getOperand(1));
    break;
  case Instruction::Opcode::Div:
    os << "div " << getValName(inst->getOperand(0)) << ", "
       << getValName(inst->getOperand(1));
    break;
  case Instruction::Opcode::Mod:
    os << "mod " << getValName(inst->getOperand(0)) << ", "
       << getValName(inst->getOperand(1));
    break;
  case Instruction::Opcode::Lt:
    os << "lt " << getValName(inst->getOperand(0)) << ", "
       << getValName(inst->getOperand(1));
    break;
  case Instruction::Opcode::Gt:
    os << "gt " << getValName(inst->getOperand(0)) << ", "
       << getValName(inst->getOperand(1));
    break;
  case Instruction::Opcode::Le:
    os << "le " << getValName(inst->getOperand(0)) << ", "
       << getValName(inst->getOperand(1));
    break;
  case Instruction::Opcode::Ge:
    os << "ge " << getValName(inst->getOperand(0)) << ", "
       << getValName(inst->getOperand(1));
    break;
  case Instruction::Opcode::Eq:
    os << "eq " << getValName(inst->getOperand(0)) << ", "
       << getValName(inst->getOperand(1));
    break;
  case Instruction::Opcode::Ne:
    os << "ne " << getValName(inst->getOperand(0)) << ", "
       << getValName(inst->getOperand(1));
    break;
  case Instruction::Opcode::And:
    os << "and " << getValName(inst->getOperand(0)) << ", "
       << getValName(inst->getOperand(1));
    break;
  case Instruction::Opcode::Or:
    os << "or " << getValName(inst->getOperand(0)) << ", "
       << getValName(inst->getOperand(1));
    break;
  case Instruction::Opcode::Br:
    if (inst->getNumOperands() == 3) {
      os << "br " << getValName(inst->getOperand(0)) << ", "
         << getValName(inst->getOperand(1)) << ", "
         << getValName(inst->getOperand(2));
    }
    break;
  case Instruction::Opcode::Jmp:
    os << "jump " << getValName(inst->getOperand(0));
    break;
  case Instruction::Opcode::Call:
    os << "call " << getValName(inst->getOperand(0)) << "(";
    for (unsigned i = 1; i < inst->getNumOperands(); ++i) {
      if (i > 1)
        os << ", ";
      os << getValName(inst->getOperand(i));
    }
    os << ")";
    break;
  case Instruction::Opcode::Ret:
    if (inst->getNumOperands() > 0)
      os << "ret " << getValName(inst->getOperand(0));
    else
      os << "ret";
    break;
  default:
    break;
  }
  os << "\n";
}

std::string ToIR(const Module &module) {
  std::ostringstream oss;
  for (const auto &gv : const_cast<Module &>(module).getGlobalList()) {
    if (auto *ptrTy = dynamic_cast<PointerType *>(gv->getType())) {
      oss << "global " << getValName(gv) << " = alloc "
          << ptrTy->getElementType()->toString() << ", ";
      if (gv->getInit()) {
        SerializeConstant(gv->getInit(), oss);
      } else {
        oss << "zeroinit";
      }
      oss << "\n";
    }
  }
  oss << "\n";

  for (const auto &func : const_cast<Module &>(module).getFunctionList()) {
    tracker.assignIDs(func);

    if (func->getBasicBlockList().empty()) {
      oss << "decl " << getValName(func) << "(";
      const auto &args = func->getArgs();
      for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0)
          oss << ", ";
        oss << args[i]->getType()->toString();
      }
      oss << ")";
      if (!func->getType()->getFunctionReturnType()->isVoidTy()) {
        oss << ": " << func->getType()->getFunctionReturnType()->toString();
      }
      oss << "\n";
    } else {
      oss << "fun " << getValName(func) << "(";
      const auto &args = func->getArgs();
      for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0)
          oss << ", ";
        oss << getValName(args[i]) << ": " << args[i]->getType()->toString();
      }
      oss << ")";
      if (!func->getType()->getFunctionReturnType()->isVoidTy()) {
        oss << ": " << func->getType()->getFunctionReturnType()->toString();
      }
      oss << " {\n";
      for (auto bb : func->getBasicBlockList()) {
        oss << getValName(bb) << ":\n";
        for (const auto &inst : bb->getInstList()) {
          SerializeInstruction(inst.get(), oss);
        }
        oss << "\n";
      }
      oss << "}\n\n";
    }
  }
  return oss.str();
}

koopa_raw_program_t ToProgram(const std::string &ir) {
  koopa_program_t program;
  koopa_parse_from_string(ir.c_str(), &program);
  koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
  koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
  koopa_delete_program(program);
  return raw;
}

koopa_raw_program_t ToProgram(const Module &module) {
  return ToProgram(ToIR(module));
}

} // namespace IRSerializer
} // namespace nanocc
