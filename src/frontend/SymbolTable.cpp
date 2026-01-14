#include "frontend/SymbolTable.h"

#include <cassert>
#include <stdexcept>
#include <string>

SymbolTable::SymbolTable() { scopes_.emplace_back(); }

void SymbolTable::Define(const std::string &name, const symbol_t &symbol) {
  auto &cur_scope = scopes_.back();
  if (cur_scope.find(name) != cur_scope.end()) {
    throw std::runtime_error("[Semantic Error]: Duplicate symbol " + name);
  }
  cur_scope.insert_or_assign(name, symbol);
}

void SymbolTable::DefineGlobal(const std::string &name,
                               const symbol_t &symbol) {
  if (scopes_.front().find(name) != scopes_.front().end()) {
    throw std::runtime_error("[Semantic Error]: Duplicate global symbol " +
                             name);
  }
  scopes_.front().insert_or_assign(name, symbol);
}

void SymbolTable::Define(const std::string &name, symbol_type_t type,
                         std::variant<int, std::string, func_info_t> value) {
  symbol_t symbol(name, type, std::move(value));
  Define(name, symbol);
}

void SymbolTable::Define(const std::string &name, const std::string &ret_type) {
  func_info_t func_info(ret_type);
  DefineGlobal(name, {name, SYMBOL_TYPE_FUNCTION, func_info});
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
