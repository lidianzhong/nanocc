#pragma once

#include "ir/IR.h"

#include <memory>
#include <string>
#include <unordered_map>

class IRModule {
private:
  std::unordered_map<std::string, std::unique_ptr<Function>> funcs_;

public:
  // 创建并存储函数，返回裸指针供后续使用
  Function *CreateFunction(const std::string &name,
                           const std::string &ret_type) {
    auto func = std::make_unique<Function>(name, ret_type);
    Function *ptr = func.get();
    funcs_[name] = std::move(func);
    return ptr;
  }

  Function *GetFunction(const std::string &func_name) {
    auto it = funcs_.find(func_name);
    return it != funcs_.end() ? it->second.get() : nullptr;
  }

  // 提供访问所有函数的接口
  const auto &GetFunctions() const { return funcs_; }
};
