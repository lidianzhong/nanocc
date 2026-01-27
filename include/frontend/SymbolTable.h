#pragma once

#include "ir/Value.h"
#include <map>
#include <string>
#include <vector>

class SymbolTable {
public:
  using SymbolMap = std::map<std::string, Value *>;

  SymbolTable() {
    // 初始化全局作用域
    EnterScope();
  }

  // 进入一个新的作用域
  void EnterScope() { layers_.emplace_back(); }

  // 退出当前作用域
  void ExitScope() {
    if (!layers_.empty()) {
      layers_.pop_back();
    }
  }

  // 在当前作用域插入符号
  bool insert(const std::string &name, Value *val) {
    auto &curr_scope = layers_.back();
    if (curr_scope.find(name) != curr_scope.end()) {
      return false; // 重复定义
    }
    curr_scope[name] = val;
    return true;
  }

  // 查找符号 (从内层向外层查找)
  Value *lookup(const std::string &name) const {
    for (auto it = layers_.rbegin(); it != layers_.rend(); ++it) {
      auto iter = it->find(name);
      if (iter != it->end()) {
        return iter->second;
      }
    }
    return nullptr; // 未找到
  }

  // 判断当前是否在全局作用域
  bool isGlobal() const { return layers_.size() == 1; }

private:
  std::vector<SymbolMap> layers_;
};
