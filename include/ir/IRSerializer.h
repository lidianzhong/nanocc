#pragma once

#include "ir/IR.h"
#include "ir/IRModule.h"
#include "koopa.h"

#include <string>

namespace IRSerializer {

// 从 IRModule 生成 Koopa IR 文本
std::string ToIR(const IRModule &module);

// 从 IR 文本生成 koopa_raw_program_t
koopa_raw_program_t ToProgram(const std::string &ir);

// 从 IRModule 直接生成 koopa_raw_program_t
koopa_raw_program_t ToProgram(const IRModule &module);

} // namespace IRSerializer
