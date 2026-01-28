#pragma once

#include "ir/Instruction.h"
#include "ir/Value.h"
#include <functional>
#include <list>
#include <memory>
#include <string>

namespace nanocc {

class Function;

class BasicBlock final : public Value {
public:
  using InstListType = std::list<std::unique_ptr<Instruction>>;

  explicit BasicBlock(Function &parent, const std::string &name = "");

  ~BasicBlock() = default;

  Function *getParent() { return &parent_.get(); }
  const Function &getParent() const { return parent_; }

  InstListType &getInstList() { return instList_; }
  const InstListType &getInstList() const { return instList_; }

  static BasicBlock *create(Function &parent, const std::string &name = "");

  static bool classof(const Value *V) {
    return V->getValueID() == BasicBlockVal;
  }

  std::string getName() const { return name_; }

private:
  std::reference_wrapper<Function> parent_;
  InstListType instList_;
  std::string name_;
};

} // namespace nanocc
