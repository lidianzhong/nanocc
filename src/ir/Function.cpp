#include "ir/Function.h"
#include "ir/Module.h"

namespace ldz {

Function::Function(FunctionType *FT, LinkageTypes linkage,
                   const std::string &name)
    : GlobalValue(FunctionVal, FT), name_(name), linkage_(linkage) {

  for (unsigned i = 0; i < FT->getParamTypes().size(); ++i) {
    Type *argTy = FT->getParamTypes()[i];
    auto *arg = new Argument(argTy, "", this, i);
    arguments_.push_back(arg);
  }
}

Function::~Function() {
  for (auto BB : basicBlockList_) {
    delete BB;
  }
  for (auto Arg : arguments_) {
    delete Arg;
  }
}

Function *Function::create(FunctionType *T, LinkageTypes linkage,
                           const std::string &name, Module &M) {
  Function *F = new Function(T, linkage, name);

  ValueSymbolTable &ST = M.getValueSymbolTable();
  ST.insert(name, F);
  M.getFunctionList().push_back(F);

  return F;
}

std::string Function::getUniqueName(const std::string &name) {
  if (name.empty())
    return "";

  std::string baseName = name_ + "_" + name;

  if (nameCounts_.find(baseName) == nameCounts_.end()) {
    nameCounts_[baseName] = 1;
    return baseName;
  } else {
    std::string newName = baseName + std::to_string(nameCounts_[baseName]++);
    while (nameCounts_.count(newName)) {
      newName = baseName + std::to_string(nameCounts_[baseName]++);
    }
    return newName;
  }
}

} // namespace ldz
