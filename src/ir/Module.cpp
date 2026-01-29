
#include "nanocc/ir/Module.h"
#include "nanocc/ir/Function.h"
#include "nanocc/ir/GlobalVariable.h"
#include "nanocc/ir/Type.h"

namespace nanocc {

Function *Module::getFunction(const std::string &name) const {
  auto *F = valSymTab_.lookup(name);
  if (F && dynamic_cast<Function *>(F)) {
    return static_cast<Function *>(F);
  }
  return nullptr;
}

template <typename... ArgsTy>
Function *Module::getOrInsertFunction(const std::string &name, Type *retTy,
                                      ArgsTy... args) {
  Function *F = getFunction(name);
  if (F) {
    return F;
  }

  FunctionType *FT = FunctionType::get(retTy, {args...});
  F = Function::create(FT, Function::InternalLinkage, name, *this);
  functionList_.push_back(F);
  valSymTab_.insert(name, F);
  return F;
}

GlobalVariable *Module::getGlobalVariable(const std::string &name) {
  auto *GV = valSymTab_.lookup(name);
  if (GV && dynamic_cast<GlobalVariable *>(GV)) {
    return static_cast<GlobalVariable *>(GV);
  }
  return nullptr;
}

GlobalVariable *Module::getOrInsertGlobal(const std::string &name, Type *ty,
                                          bool isConstant) {
  GlobalVariable *GV = getGlobalVariable(name);
  if (GV) {
    return GV;
  }

  GV = GlobalVariable::create(ty, name, this, nullptr, isConstant);
  globalList_.push_back(GV);
  valSymTab_.insert(name, GV);
  return GV;
}

} // namespace nanocc