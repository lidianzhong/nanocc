#pragma once

#include "ir/IR.h"

#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

typedef enum {
  SYMBOL_TYPE_CONSTANT,
  SYMBOL_TYPE_VARIABLE,
} symbol_type_t;

struct symbol_t {
  std::string name;
  symbol_type_t type;
  Value value; // 常量值或变量地址

  symbol_t(const std::string &name, symbol_type_t type, Value value)
      : name(name), type(type), value(std::move(value)) {}
};

class SymbolTable {
public:
  SymbolTable() = default;
  ~SymbolTable() = default;

  void Define(const std::string &name, symbol_type_t type, Value value);
  std::optional<symbol_t> Lookup(const std::string &name) const;

  void EnterScope();
  void ExitScope();

private:
  std::size_t current_scope_level_ = 0;
  std::vector<std::unordered_map<std::string, symbol_t>> scopes_;
};
