#pragma once

#include "nanocc/ir/GlobalValue.h"
#include "nanocc/ir/Module.h"

namespace nanocc {

class Type;

/// Global variable class
class GlobalVariable : public GlobalValue {
public:
  GlobalVariable(Type *ty, const std::string &name, Constant *initializer,
                 bool isConstant)
      : GlobalValue(Value::GlobalVariableVal, Type::getPointerTy(ty)),
        name_(name), initializer_(initializer), isConstantGlobal_(isConstant) {
    if (initializer)
      addOperand(initializer);
  }

  /// Create a GlobalVariable and insert it into Module's global list and symbol
  /// table.
  /// @param ty Type of the global variable
  /// @param name Name of the global variable
  /// @param M Module to insert the global variable into
  /// @param init Initializer constant (can be nullptr)
  static GlobalVariable *create(Type *ty, const std::string &name, Module *M,
                                Constant *initializer = nullptr,
                                bool isConstant = false) {
    auto *GV = new GlobalVariable(ty, name, initializer, isConstant);

    ValueSymbolTable &ST = M->getValueSymbolTable();
    ST.insert(name, GV);
    M->getGlobalList().push_back(GV);

    return GV;
  }

  /// Get the name of the global variable
  std::string getName() const { return name_; }

  /// Get the initializer constant
  Constant *getInit() const { return initializer_; }

  /// Check if the global variable is constant
  bool isConstant() const { return isConstantGlobal_; }

private:
  std::string name_;                ///< Name of the global variable
  Constant *initializer_ = nullptr; ///< Initializer constant
  bool isConstantGlobal_;           ///< Is the global variable constant
};

} // namespace nanocc