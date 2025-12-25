#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

typedef enum {
  SYMBOL_TYPE_CONSTANT,
  SYMBOL_TYPE_VARIABLE,
} symbol_type_t;

struct symbol_t {
  std::string name;
  symbol_type_t type;
  std::variant<int32_t, std::string> value; // 常量或偏移值

  symbol_t(const std::string &name, int32_t imm)
      : name(name), type(SYMBOL_TYPE_CONSTANT), value(imm) {}

  symbol_t(const std::string &name, std::string offset)
      : name(name), type(SYMBOL_TYPE_VARIABLE), value(std::move(offset)) {}
};

class SymbolTable {
public:
  SymbolTable() = default;
  ~SymbolTable() = default;

  void Define(const std::string &name, int32_t imm);
  void Define(const std::string &name, std::string offset);

  std::optional<symbol_t> Lookup(const std::string &name) const;

  void EnterScope();
  void ExitScope();

private:
  std::size_t current_scope_level_ = 0;
  std::unordered_map<std::string, symbol_t> scopes_;
};