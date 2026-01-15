#pragma once

#include "ir/IR.h"

#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

typedef enum {
  SYMBOL_TYPE_CONSTANT,
  SYMBOL_TYPE_VARIABLE,
  SYMBOL_TYPE_FUNCTION
} symbol_type_t;

struct func_info_t {
  std::string ret_type;
  // std::vector<std::pair<std::string, std::string>> params;

  // func_info_t(const std::string &ret_type,
  //             const std::vector<std::pair<std::string, std::string>> &params)
  //     : ret_type(ret_type), params(params) {}

  func_info_t(const std::string &ret_type) : ret_type(ret_type) {}
};

struct symbol_t {
  std::string name;
  symbol_type_t type;

  std::variant<int, std::string, func_info_t> value; // 常量值/变量地址/函数信息

  symbol_t(const std::string &name, symbol_type_t type,
           std::variant<int, std::string, func_info_t> value)
      : name(name), type(type), value(std::move(value)) {}
};

class SymbolTable {
public:
  SymbolTable();
  ~SymbolTable() = default;

  void Define(const std::string &name, symbol_type_t type,
              std::variant<int, std::string, func_info_t> value);
  void DefineGlobal(const std::string &name, symbol_type_t type,
                    std::variant<int, std::string, func_info_t> value);
  void DefineGlobal(const std::string &name, const std::string &ret_type);
  const symbol_t *Lookup(const std::string &name) const;

  void EnterScope();
  void ExitScope();

  bool IsGlobal() const { return scopes_.size() == 1; }

private:
  void Define(const std::string &name, const symbol_t &symbol);
  void DefineGlobal(const std::string &name, const symbol_t &symbol);

private:
  std::vector<std::unordered_map<std::string, symbol_t>> scopes_;
};
