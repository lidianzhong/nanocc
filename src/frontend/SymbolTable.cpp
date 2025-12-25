#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <variant>

#include "frontend/SymbolTable.h"

void SymbolTable::Define(const std::string &name, int32_t value) {
  std::string key = '@' + name + std::to_string(current_scope_level_);
  scopes_.emplace(key, symbol_t(name, value));
}

void SymbolTable::Define(const std::string &name, std::string offset) {
  std::string key = '@' + name + std::to_string(current_scope_level_);
  scopes_.emplace(key, symbol_t(name, offset));
}

std::optional<symbol_t> SymbolTable::Lookup(const std::string &name) const {
  for (int level = current_scope_level_; level >= 0; --level) {
    std::string key = '@' + name + std::to_string(level);
    auto it = scopes_.find(key);
    if (it != scopes_.end()) {
      return it->second;
    }
  }
  return std::nullopt;
}

void SymbolTable::EnterScope() { ++current_scope_level_; }

void SymbolTable::ExitScope() {
  assert(current_scope_level_ > 0);
  --current_scope_level_;
}