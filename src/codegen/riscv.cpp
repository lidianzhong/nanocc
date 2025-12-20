#include "codegen/riscv.h"
#include <cassert>
#include <iostream>

void Visit(const koopa_raw_program_t& program) {
    // 声明 .text 段
    std::cout << "  .text" << std::endl;

    // 访问所有全局变量
    Visit(program.values);

    // 访问所有函数
    Visit(program.funcs);
}

void Visit(const koopa_raw_slice_t& slice) {
    for (size_t i = 0; i < slice.len; ++i) {
        auto ptr = slice.buffer[i];
        switch (slice.kind) {
            case KOOPA_RSIK_FUNCTION:
                // 访问函数
                Visit(reinterpret_cast<koopa_raw_function_t>(ptr));
                break;
            case KOOPA_RSIK_BASIC_BLOCK:
                // 访问基本块
                Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
                break;
            case KOOPA_RSIK_VALUE:
                // 访问指令
                Visit(reinterpret_cast<koopa_raw_value_t>(ptr));
                break;
            default:
                assert(false);
        }
    }
}

void Visit(const koopa_raw_function_t& func) {
    // 声明全局符号
    // Koopa IR 中的函数名包含 @ 前缀, 需要去掉
    std::string name = func->name;
    if (name.length() > 0 && name[0] == '@') {
        name = name.substr(1);
    }

    std::cout << "  .globl " << name << std::endl;
    std::cout << name << ":" << std::endl;

    // 访问函数的基本块
    Visit(func->bbs);
}

void Visit(const koopa_raw_basic_block_t& bb) {
    // do something

    // 访问基本块的指令
    Visit(bb->insts);
}

void Visit(const koopa_raw_value_t& value) {
    const auto &kind = value->kind;
    switch (kind.tag) {
        case KOOPA_RVT_RETURN:
            // 访问 return 指令的返回值
            Visit(value->kind.data.ret.value);
            break;
        case KOOPA_RVT_INTEGER:
            // 访问 integer 指令
            Visit(kind.data.integer);
            break;
        default:
            assert(false);
    }
}

void Visit(const koopa_raw_integer_t& integer) {
    int32_t int_val = integer.value;
    std::cout << "  li a0, " << int_val << std::endl;
    std::cout << "  ret" << std::endl;
}