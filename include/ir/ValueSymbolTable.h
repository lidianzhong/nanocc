#pragma once

#include <map>
#include <string>
#include <vector>

namespace ldz {

class Value;

class ValueSymbolTable {
private:
  using SymbolMap = std::map<std::string, Value *>;
  std::vector<SymbolMap> layers_;

public:
  ValueSymbolTable() { layers_.emplace_back(); }
  ~ValueSymbolTable() = default;

  /// Enter a new scope
  void enterScope();

  /// Exit the current scope
  void exitScope();

  /// Insert a symbol into the current scope
  bool insert(const std::string &name, Value *val);

  /// Lookup a symbol (searching from inner to outer scopes)
  Value *lookup(const std::string &name) const;

  /// Check if currently in the global scope
  bool isGlobal() const;
};

} // namespace ldz