
#include "nanocc/ir/ValueSymbolTable.h"
#include <stdexcept>

namespace nanocc {

void ValueSymbolTable::enterScope() { layers_.emplace_back(); }

void ValueSymbolTable::exitScope() {
  if (layers_.empty()) {
    throw std::runtime_error("No scope to exit");
    return;
  }
  layers_.pop_back();
}

bool ValueSymbolTable::insert(const std::string &name, Value *val) {
  if (layers_.empty()) {
    throw std::runtime_error("No scope to insert symbol into");
    return false;
  }

  auto &currentScope = layers_.back();
  if (currentScope.find(name) != currentScope.end()) {
    return false; // Symbol already exists in current scope
  }
  currentScope[name] = val;
  return true;
}

Value *ValueSymbolTable::lookup(const std::string &name) const {
  for (auto it = layers_.rbegin(); it != layers_.rend(); ++it) {
    auto found = it->find(name);
    if (found != it->end()) {
      return found->second;
    }
  }
  return nullptr; // Not found
}

bool ValueSymbolTable::isGlobal() const { return layers_.size() == 1; }

} // namespace nanocc