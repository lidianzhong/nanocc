#include "backend/CodeGen.h"
#include "frontend/AST.h"
#include "frontend/DumpVisitor.h"
#include "ir/IRGenVisitor.h"
#include "ir/IRSerializer.h"
#include "ir/Module.h"

#include <cassert>
#include <cstdio>
#include <iostream>
#include <string>

using namespace std;
using namespace nanocc;

// 声明 lexer 的输入, 以及 parser 函数
// 为什么不引用 sysy.tab.hpp 呢? 因为首先里面没有 yyin 的定义
// 其次, 因为这个文件不是我们自己写的, 而是被 Bison 生成出来的
// 你的代码编辑器/IDE 很可能找不到这个文件, 然后会给你报错 (虽然编译不会出错)
// 看起来会很烦人, 于是干脆采用这种看起来 dirty 但实际很有效的手段
extern FILE *yyin;
extern int yyparse(unique_ptr<BaseAST> &ast);

int main(int argc, const char *argv[]) {
  assert(argc == 5);

  string mode(argv[1]);  // 模式: -koopa or -riscv
  auto input = argv[2];  // 输入文件
  auto output = argv[4]; // 输出文件

  if (mode != "-koopa" && mode != "-riscv") {
    cerr << "Error: Unsupported mode " << mode << endl;
    return 1;
  }

  // open source file
  yyin = fopen(input, "r");
  assert(yyin);

  // parse
  unique_ptr<BaseAST> ast;
  auto parse_ret = yyparse(ast);
  assert(!parse_ret);
  fclose(yyin);

  // dump AST
  DumpVisitor dumper;
  ast->Accept(dumper);

  // AST -> Koopa IR
  Module module;
  IRGenVisitor irgen(module);
  ast->Accept(irgen);

  // Code generation
  if (mode == "-koopa") {
    // 生成 Koopa IR 文本
    FILE *out = fopen(output, "w");
    assert(out);
    std::string ir = IRSerializer::ToIR(module);
    fprintf(out, "%s", ir.c_str());
    fclose(out);
  } else if (mode == "-riscv") {
    // 生成 RISC-V 汇编
    freopen(output, "w", stdout);
    ProgramCodeGen codegen;
    codegen.Emit(IRSerializer::ToProgram(module));
    fclose(stdout);
  }

  return 0;
}
