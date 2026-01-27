#include "ir/BasicBlock.h"
#include "ir/Function.h"

namespace ldz {

BasicBlock::BasicBlock(Function &parent, const std::string &name)
    : Value(BasicBlockVal, nullptr), parent_(parent),
      name_(parent.getUniqueName(name)) {}

BasicBlock *BasicBlock::create(Function &parent, const std::string &name) {
  return new BasicBlock(parent, name);
}

} // namespace ldz
