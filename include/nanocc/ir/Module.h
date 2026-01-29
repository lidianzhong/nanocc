#pragma once

#include "nanocc/ir/Type.h"
#include "nanocc/ir/Value.h"
#include "nanocc/ir/ValueSymbolTable.h"
#include <list>
#include <string>

namespace nanocc {

class GlobalVariable;
class Function;

class Module {

private:
  /// @todo support unique_ptr
  std::list<Function *> functionList_;

  /// @todo support unique_ptr
  std::list<GlobalVariable *> globalList_;

  ValueSymbolTable valSymTab_;

public:
  Module() = default;
  ~Module() = default;

  std::list<Function *> &getFunctionList() { return functionList_; }
  std::list<GlobalVariable *> &getGlobalList() { return globalList_; }

  ValueSymbolTable &getValueSymbolTable() { return valSymTab_; }

  //===--------------------------------------------------------------------===//
  // Iterators.
  //
  // auto functions() { return functionList_; }
  // auto globals() { return globalList_; }

  Function *getFunction(const std::string &name) const;

  template <typename... ArgsTy>
  Function *getOrInsertFunction(const std::string &name, Type *retTy,
                                ArgsTy... args);

  GlobalVariable *getGlobalVariable(const std::string &name);
  GlobalVariable *getOrInsertGlobal(const std::string &name, Type *ty,
                                    bool isConstant);
};

} // namespace nanocc