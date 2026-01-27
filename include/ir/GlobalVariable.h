#pragma once

#include "ir/Constant.h"
#include "ir/Module.h"
#include "ir/User.h"
#include "ir/ValueSymbolTable.h"

namespace ldz {

class Type;

class GlobalVariable : public User {
public:
  GlobalVariable(Type *ty, const std::string &name, Constant *init)
      : User(Value::GlobalVariableVal, Type::getPointerTy(ty)), name_(name),
        initVal_(init) {
    if (init)
      addOperand(init);
  }

  static GlobalVariable *create(Type *ty, const std::string &name, Module *M,
                                Constant *init = nullptr) {
    auto *GV = new GlobalVariable(ty, name, init);

    ValueSymbolTable &ST = M->getValueSymbolTable();
    ST.insert(name, GV);
    M->getGlobalList().push_back(GV);

    return GV;
  }

  std::string getName() const { return name_; }
  Constant *getInit() const { return initVal_; }

private:
  std::string name_;
  Constant *initVal_;
};

} // namespace ldz