#include "ir/IRSerializer.h"
#include "ir/IR.h"
#include "ir/Type.h"
#include "ir/Value.h"
#include <sstream>
#include <variant>

namespace IRSerializer {

static std::string OperandToString(const Operand &op) {
    if (std::holds_alternative<Value*>(op)) {
        Value *v = std::get<Value*>(op);
        if (!v) return "null";
        return v->toString();
    } else if (std::holds_alternative<BasicBlock*>(op)) {
        return "%" + std::get<BasicBlock*>(op)->getName();
    } else if (std::holds_alternative<std::string>(op)) {
        return std::get<std::string>(op);
    } else if (std::holds_alternative<BranchTarget>(op)) {
        const auto &bt = std::get<BranchTarget>(op);
        std::string s = "%" + bt.target->getName();
        if (!bt.args.empty()) {
            s += "(";
            for (size_t i = 0; i < bt.args.size(); ++i) {
                if (i > 0) s += ", ";
                s += bt.args[i]->toString();
            }
            s += ")";
        }
        return s;
    }
    return "";
}

static void SerializeInstruction(const Instruction *inst, std::ostream &os) {
    if (inst->op == Opcode::GlobalAlloc) {
        os << "global " << inst->toString() << " = alloc " 
           << inst->getType()->getPointerElementType()->toString() << ", ";
        
        bool has_init = false;
        if (!inst->args.empty()) {
            if (std::holds_alternative<Value*>(inst->args[0])) {
                if (std::get<Value*>(inst->args[0]) != nullptr) {
                    has_init = true;
                }
            } else {
                has_init = true;
            }
        }

        if (has_init)
            os << OperandToString(inst->args[0]);
        else
            os << "zeroinit";
        os << "\n";
        return;
    }

    if (!inst->getType()->isVoidTy()) {
        os << "  " << inst->toString() << " = ";
    } else {
        os << "  ";
    }

    switch (inst->op) {
        case Opcode::Alloc:
             os << "alloc " << inst->getType()->getPointerElementType()->toString();
             break;
        case Opcode::Load:
             os << "load " << OperandToString(inst->args[0]);
             break;
        case Opcode::Store:
             os << "store " << OperandToString(inst->args[0]) << ", " << OperandToString(inst->args[1]);
             break;
        case Opcode::GetElemPtr:
             os << "getelemptr " << OperandToString(inst->args[0]) << ", " << OperandToString(inst->args[1]);
             break;
        case Opcode::GetPtr:
             os << "getptr " << OperandToString(inst->args[0]) << ", " << OperandToString(inst->args[1]);
             break;
        case Opcode::Add: os << "add " << OperandToString(inst->args[0]) << ", " << OperandToString(inst->args[1]); break;
        case Opcode::Sub: os << "sub " << OperandToString(inst->args[0]) << ", " << OperandToString(inst->args[1]); break;
        case Opcode::Mul: os << "mul " << OperandToString(inst->args[0]) << ", " << OperandToString(inst->args[1]); break;
        case Opcode::Div: os << "div " << OperandToString(inst->args[0]) << ", " << OperandToString(inst->args[1]); break;
        case Opcode::Mod: os << "mod " << OperandToString(inst->args[0]) << ", " << OperandToString(inst->args[1]); break;
        case Opcode::Lt: os << "lt " << OperandToString(inst->args[0]) << ", " << OperandToString(inst->args[1]); break;
        case Opcode::Gt: os << "gt " << OperandToString(inst->args[0]) << ", " << OperandToString(inst->args[1]); break;
        case Opcode::Le: os << "le " << OperandToString(inst->args[0]) << ", " << OperandToString(inst->args[1]); break;
        case Opcode::Ge: os << "ge " << OperandToString(inst->args[0]) << ", " << OperandToString(inst->args[1]); break;
        case Opcode::Eq: os << "eq " << OperandToString(inst->args[0]) << ", " << OperandToString(inst->args[1]); break;
        case Opcode::Ne: os << "ne " << OperandToString(inst->args[0]) << ", " << OperandToString(inst->args[1]); break;
        case Opcode::And: os << "and " << OperandToString(inst->args[0]) << ", " << OperandToString(inst->args[1]); break;
        case Opcode::Or: os << "or " << OperandToString(inst->args[0]) << ", " << OperandToString(inst->args[1]); break;
        case Opcode::Br:
             os << "br " << OperandToString(inst->args[0]) << ", " 
                << OperandToString(inst->args[1]) << ", " << OperandToString(inst->args[2]);
             break;
        case Opcode::Jmp:
             os << "jump " << OperandToString(inst->args[0]);
             break;
        case Opcode::Ret:
             if (inst->args.empty()) os << "ret";
             else os << "ret " << OperandToString(inst->args[0]);
             break;
        case Opcode::Call:
             os << "call @" << std::get<std::string>(inst->args[0]) << "(";
             if (inst->args.size() > 1 && std::holds_alternative<std::vector<Value*>>(inst->args[1])) {
                 const auto& args = std::get<std::vector<Value*>>(inst->args[1]);
                 for (size_t i = 0; i < args.size(); ++i) {
                     if (i > 0) os << ", ";
                     os << args[i]->toString();
                 }
             }
             os << ")";
             break;
        default: break;
    }
    os << "\n";
}

std::string ToIR(const IRModule &module) {
    std::ostringstream oss;
    for (const auto &inst : module.globals_) {
        SerializeInstruction(inst.get(), oss);
    }
    oss << "\n";

    for (const auto &func : module.GetFunctions()) {
        if (func->blocks.empty()) {
            // Declaration
            oss << "decl " << "@" + func->getName() << "(";
            for (size_t i = 0; i < func->params.size(); ++i) {
                 if (i > 0) oss << ", ";
                 oss << func->params[i].type->toString();
            }
            oss << ")";
            if (!func->getType()->isVoidTy()) {
                 oss << ": " << func->getType()->toString();
            }
            oss << "\n";
        } else {
            // Definition
            oss << "fun " << "@" + func->getName() << "(";
            for (size_t i = 0; i < func->params.size(); ++i) {
                 if (i > 0) oss << ", ";
                 oss << func->params[i].name << ": " << func->params[i].type->toString();
            }
            oss << ")";
            
            if (!func->getType()->isVoidTy()) {
                 oss << ": " << func->getType()->toString();
            }
            
            oss << " {\n";
            for (const auto &bb : func->blocks) {
                 oss << "%" << bb->getName() << ":\n";
                 for (const auto &inst : bb->insts) {
                     SerializeInstruction(inst, oss);
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
    koopa_error_code_t ret = koopa_parse_from_string(ir.c_str(), &program);
    assert(ret == KOOPA_EC_SUCCESS);
    koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
    koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
    koopa_delete_program(program);
    return raw;
}

koopa_raw_program_t ToProgram(const IRModule &module) {
    return ToProgram(ToIR(module));
}

} // namespace IRSerializer
