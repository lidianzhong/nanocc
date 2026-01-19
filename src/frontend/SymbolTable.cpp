#include "frontend/SymbolTable.h"

#include <cassert>
#include <stdexcept>
#include <string>

SymbolTable::SymbolTable() { scopes_.emplace_back(); }

void SymbolTable::DefineConstant(const std::string &name, int value) {
  Define({name, SYMBOL_TYPE_CONSTANT, value});
}

void SymbolTable::DefineVariable(const std::string &name,
                                 const std::string &reg_or_addr) {
  Define({name, SYMBOL_TYPE_VARIABLE, reg_or_addr});
}

void SymbolTable::DefineArray(const std::string &name, const std::string &addr,
                              const std::vector<int> &dims) {
  array_info_t array_info(addr, dims);
  Define({name, SYMBOL_TYPE_ARRAY, array_info});
}

void SymbolTable::DefinePointer(const std::string &name,
                                const std::string &reg) {
  Define({name, SYMBOL_TYPE_POINTER, reg});
}

void SymbolTable::DefineGlobalConstant(const std::string &name, int value) {
  DefineGlobal({name, SYMBOL_TYPE_CONSTANT, value});
}

void SymbolTable::DefineGlobalVariable(const std::string &name,
                                        const std::string &reg_or_addr) {
  DefineGlobal({name, SYMBOL_TYPE_VARIABLE, reg_or_addr});
}

void SymbolTable::DefineGlobalArray(const std::string &name,
                                    const std::string &addr,
                                    const std::vector<int> &dims) {
  array_info_t array_info(addr, dims);
  DefineGlobal({name, SYMBOL_TYPE_ARRAY, array_info});
}

void SymbolTable::DefineFunction(const std::string &name,
                                const std::string &ret_type) {
  func_info_t func_info(ret_type);
  DefineGlobal({name, SYMBOL_TYPE_FUNCTION, func_info});
}

void SymbolTable::Define(const symbol_t &symbol) {
  assert(IsGlobal() || "Define called in global scope, use DefineGlobal instead");
  auto &cur_scope = scopes_.back();
  if (cur_scope.find(symbol.name) != cur_scope.end()) {
    throw std::runtime_error("[Semantic Error]: Duplicate symbol " + symbol.name);
  }
  cur_scope.insert_or_assign(symbol.name, symbol);
}

void SymbolTable::DefineGlobal(const symbol_t &symbol) {
  if (scopes_.front().find(symbol.name) != scopes_.front().end()) {
    throw std::runtime_error("[Semantic Error]: Duplicate global symbol " +
                             symbol.name);
  }
  scopes_.front().insert_or_assign(symbol.name, symbol);
}

const symbol_t *SymbolTable::Lookup(const std::string &name) const {
  // 从内层作用域向外层查找
  for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
    auto found = it->find(name);
    if (found != it->end()) {
      return &found->second;
    }
  }
  return nullptr;
}

void SymbolTable::EnterScope() { scopes_.emplace_back(); }

void SymbolTable::ExitScope() {
  assert(!scopes_.empty() && "No scope to exit");
  scopes_.pop_back();
}
