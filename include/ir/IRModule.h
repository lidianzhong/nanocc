#pragma once

#include "ir/IR.h"

#include <algorithm>
#include <memory>
#include <string>

class IRModule {
private:
  std::vector<std::unique_ptr<Function>> funcs_;

public:
  std::vector<std::unique_ptr<Instruction>> globals_;

public:
  // 创建并存储函数，返回裸指针供后续使用
  Function *CreateFunction(
      const std::string &name, const std::string &ret_type,
      const std::vector<std::pair<std::string, std::string>> &params) {
    auto func = std::make_unique<Function>(name, ret_type, params);
    Function *ptr = func.get();
    funcs_.push_back(std::move(func));
    return ptr;
  }

  Function *GetFunction(const std::string &func_name) {
    auto it = std::find_if(funcs_.begin(), funcs_.end(),
                           [&](const std::unique_ptr<Function> &f) {
                             return f->name == "@" + func_name; // TODO
                           });

    return (it != funcs_.end()) ? it->get() : nullptr;
  }

  // 提供访问所有函数的接口
  const std::vector<std::unique_ptr<Function>> &GetFunctions() const {
    return funcs_;
  }
};
