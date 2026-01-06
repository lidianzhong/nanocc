#include "frontend/SymbolTable.h"

#include <cassert>
#include <string>

void SymbolTable::Define(const std::string &name, symbol_type_t type,
                         Value value) {
  if (scopes_.empty()) {
    scopes_.emplace_back();
  }
  scopes_.back().emplace(name, symbol_t(name, type, std::move(value)));
}

std::optional<symbol_t> SymbolTable::Lookup(const std::string &name) const {
  // 从内层作用域向外层查找
  for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
    auto found = it->find(name);
    if (found != it->end()) {
      return found->second;
    }
  }
  return std::nullopt;
}

void SymbolTable::EnterScope() {
  scopes_.emplace_back();
  ++current_scope_level_;
}

void SymbolTable::ExitScope() {
  assert(current_scope_level_ > 0);
  scopes_.pop_back();
  --current_scope_level_;
}
