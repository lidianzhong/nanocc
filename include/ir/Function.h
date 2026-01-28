#pragma once

#include "ir/BasicBlock.h"
#include "ir/GlobalValue.h"
#include "ir/Module.h"
#include "ir/Type.h"
#include "ir/Value.h"
#include "ir/ValueSymbolTable.h"
#include <list>
#include <map>
#include <string>
#include <vector>

namespace nanocc {

class Type;
class Module;

class Argument : public Value {
public:
  Argument(Type *ty, const std::string &name = "", Function *parent = nullptr,
           unsigned argNo = 0)
      : Value(ArgumentVal, ty), name_(name), parent_(parent), argNo_(argNo) {}

  Function *getParent() { return parent_; }
  unsigned getArgNo() const { return argNo_; }
  const std::string &getName() const { return name_; }
  void setName(const std::string &name) { name_ = name; }

  static bool classof(const Value *V) { return V->getValueID() == ArgumentVal; }

private:
  std::string name_;
  Function *parent_;
  unsigned argNo_;
};

class Function : public GlobalValue {
private:
  using BasicBlockListType = std::list<BasicBlock *>;
  using ArgumentListType = std::vector<Argument *>;

  /// 基本块以链表形式存储在Function
  BasicBlockListType basicBlocks_;

  /// Function 中存有参数列表
  ArgumentListType argumentList_;

  /// BasicBlock name counter
  std::map<std::string, int> nameCounts_;

public:
  /// 使用LinkageTypes区分声明和定义
  enum LinkageTypes {
    ExternalLinkage,
    InternalLinkage,
  };

  Function(FunctionType *FT, LinkageTypes linkage, const std::string &name);

  ~Function();

  std::list<BasicBlock *> &getBasicBlockList() { return basicBlockList_; }
  const std::list<BasicBlock *> &getBasicBlockList() const {
    return basicBlockList_;
  }

  std::string getName() const { return name_; }

  const std::vector<Argument *> &getArgs() const { return arguments_; }

  void addBasicBlock(BasicBlock *bb) { basicBlockList_.push_back(bb); }

  std::string getUniqueName(const std::string &name);

  static Function *create(FunctionType *T, LinkageTypes linkage,
                          const std::string &name, Module &M);

  static bool classof(const Value *V) { return V->getValueID() == FunctionVal; }

private:
  std::string name_;
  LinkageTypes linkage_;
  std::list<BasicBlock *> basicBlockList_;
  std::vector<Argument *> arguments_;
};

} // namespace nanocc
