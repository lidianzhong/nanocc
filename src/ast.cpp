#include "ast.h"

void CompUnitAST::Dump() const {
    std::cout << "CompUnitAST { ";
    func_def->Dump();
    std::cout << " }";
}

void CompUnitAST::DumpIR() const {
    func_def->DumpIR();
}

void FuncDefAST::Dump() const {
    std::cout << "FuncDefAST { ";
    func_type->Dump();
    std::cout << ", " << ident << ", ";
    block->Dump();
    std::cout << " }";
}

void FuncDefAST::DumpIR() const {
    // 1. 输出函数头
    std::cout << "fun @" << ident << "(): ";
    auto funcTypeAst = dynamic_cast<FuncTypeAST*>(func_type.get());
    if (funcTypeAst && funcTypeAst->name == "int") {
        std::cout << "i32";
    }
    std::cout << " {" << std::endl;

    // 2. 输出函数体入口
    std::cout << "%entry:" << std::endl;

    // 3. 递归生成 block 内部的指令
    block->DumpIR();

    // 4. 输出右大括号
    std::cout << "}" << std::endl;
}

void FuncTypeAST::Dump() const {
    std::cout << "FuncTypeAST { " << name << " }";
}

void BlockAST::Dump() const {
    std::cout << "BlockAST { ";
    stmt->Dump();
    std::cout << " }";
}

void BlockAST::DumpIR() const {
    stmt->DumpIR();
}

void StmtAST::Dump() const {
    std::cout << "StmtAST { ";
    number->Dump();
    std::cout << " }";
}

void StmtAST::DumpIR() const {
    std::cout << "  ret ";
    number->DumpIR();
    std::cout << std::endl;
}

void NumberAST::Dump() const {
    std::cout << "NumberAST { " << val << " }";
}

void NumberAST::DumpIR() const {
    std::cout << val;
}