#include <cassert>
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include "ast.h"
#include "codegen/riscv.h"
#include "irgen/ir_gen.h"
#include "koopa.h"

using namespace std;

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

  // 1. 打开输入文件
  yyin = fopen(input, "r");
  assert(yyin);

  // 2. 解析 AST
  unique_ptr<BaseAST> ast;
  auto parse_ret = yyparse(ast);
  assert(!parse_ret);

  // 3. 生成 IR
  IRGenerator ir_gen;
  ir_gen.Visit(ast.get());
  std::string ir = ir_gen.GetIR();

  if (mode == "-koopa") {
    FILE *out = fopen(output, "w");
    assert(out);
    fprintf(out, "%s", ir.c_str());
    fclose(out);
    return 0;
  }

  // 4. 获取 raw program
  koopa_raw_program_t raw = ir_gen.GetProgram();

  // 处理 raw program
  if (mode == "-riscv") {
    freopen(output, "w", stdout);
    Visit(raw);
    fclose(stdout);
  }

  return 0;
}