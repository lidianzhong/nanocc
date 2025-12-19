#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>

#include "ast.h"

using namespace std;

// 声明 lexer 的输入, 以及 parser 函数
// 为什么不引用 sysy.tab.hpp 呢? 因为首先里面没有 yyin 的定义
// 其次, 因为这个文件不是我们自己写的, 而是被 Bison 生成出来的
// 你的代码编辑器/IDE 很可能找不到这个文件, 然后会给你报错 (虽然编译不会出错)
// 看起来会很烦人, 于是干脆采用这种看起来 dirty 但实际很有效的手段
extern FILE *yyin;
extern int yyparse(unique_ptr<BaseAST> &ast);

int main(int argc, const char *argv[]) {
    // 校验参数数量
    assert(argc == 5);
    
    string mode(argv[1]);    // 模式: -koopa
    auto input = argv[2];    // 输入文件
    auto output = argv[4];   // 输出文件

    // 1. 打开输入文件
    yyin = fopen(input, "r");
    assert(yyin);

    // 2. 解析 AST
    unique_ptr<BaseAST> ast;
    auto ret = yyparse(ast);
    assert(!ret);

    // 3. 处理输出流
    // 利用 freopen 将标准输出重定向到 output 文件
    // 这样你 AST 里的 std::cout 就会直接写入到目标文件
    if (freopen(output, "w", stdout) == nullptr) {
        cerr << "Error: Cannot open output file " << output << endl;
        return 1;
    }

    // 4. 根据模式执行
    if (mode == "-koopa") {
        ast->DumpIR();
    } else {
        // Unsupported mode
        cerr << "Error: Unsupported mode " << mode << endl;
        return 1;
    }

    return 0;
}