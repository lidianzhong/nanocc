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
  SYMBOL_TYPE_ARRAY,
  SYMBOL_TYPE_POINTER,
  SYMBOL_TYPE_FUNCTION
} symbol_type_t;

struct func_info_t {
  std::string ret_type;
  func_info_t(const std::string &ret_type) : ret_type(ret_type) {}
};

struct array_info_t {
  std::string addr;
  std::vector<int> dims;
  array_info_t(const std::string &addr, const std::vector<int> &dims)
      : addr(addr), dims(dims) {}
};

struct symbol_t {
  std::string name;
  symbol_type_t type;
  std::variant<int, std::string, func_info_t, array_info_t> value; // 常量值/变量名/函数信息/数组信息

  symbol_t(std::string name, symbol_type_t type,
           std::variant<int, std::string, func_info_t, array_info_t> value)
      : name(std::move(name)), type(type), value(std::move(value)) {}
};

class SymbolTable {
public:
  SymbolTable();
  ~SymbolTable() = default;

  void DefineConstant(const std::string &name, int value);
  void DefineVariable(const std::string &name, const std::string &reg_or_addr);
  void DefineArray(const std::string &name, const std::string &addr,
                   const std::vector<int> &dims);
  void DefinePointer(const std::string &name, const std::string &reg);

  void DefineGlobalConstant(const std::string &name, int value);
  void DefineGlobalVariable(const std::string &name, const std::string &reg_or_addr);
  void DefineGlobalArray(const std::string &name, const std::string &addr,
                        const std::vector<int> &dims);
  void DefineFunction(const std::string &name,
                      const std::string &ret_type);

  const symbol_t *Lookup(const std::string &name) const;

  void EnterScope();
  void ExitScope();

  bool IsGlobal() const { return scopes_.size() == 1; }

private:
  void Define(const symbol_t &symbol);
  void DefineGlobal(const symbol_t &symbol);

private:
  std::vector<std::unordered_map<std::string, symbol_t>> scopes_;
};
