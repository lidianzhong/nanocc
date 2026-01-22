#pragma once

#include "ir/IR.h"
#include "ir/IRModule.h"

#include <memory>
#include <unordered_map>
#include <string>

class IRBuilder {
public:
  IRBuilder(IRModule *module) : module_(module) {}
  ~IRBuilder() = default;

  void SetCurrentFunction(Function *func);
  void EndFunction();

  BasicBlock *CreateBlock(const std::string &name);
  void SetInsertPoint(BasicBlock *bb);
  
  // Helper to append instruction to current block
  void Insert(Instruction *inst);

  Value *CreateAlloca(Type *type, const std::string &var_name = "");
  Value *CreateGlobalAlloc(const std::string &var_name, Type *type, Value *init_val);
  
  // CreateGetElemPtr: base_addr must be a pointer type
  Value *CreateGetElemPtr(Value *base_addr, Value *index);
  
  // CreateGetPtr: base_ptr must be a pointer type
  Value *CreateGetPtr(Value *base_ptr, Value *index);
  
  Value *CreateLoad(Value *addr);
  void CreateStore(Value *value, Value *addr);

  void CreateBranch(Value *cond, BasicBlock *then_bb, BasicBlock *else_bb);
  void CreateJump(BasicBlock *target_bb);
  
  void CreateReturn(Value *value);
  void CreateReturn();

  Value *CreateUnaryOp(Opcode op, Value *value);
  Value *CreateBinaryOp(Opcode op, Value *lhs, Value *rhs);

  Value *CreateCall(Function *func, const std::vector<Value*> &args);
  Value *CreateCall(const std::string &func_name, const std::vector<Value*> &args, Type *ret_type);

public:
  IRModule *module_ = nullptr;
  Function *cur_func_ = nullptr;
  BasicBlock *cur_bb_ = nullptr;

  // Helpers for temporary naming
  std::string NewTempRegName();
  std::string NewTempAddrName(const std::string &name);
  std::string NewTempLabelName(const std::string &prefix);

private:
  int temp_reg_id_ = 0;
  std::unordered_map<std::string, int> temp_addr_counters_;
  std::unordered_map<std::string, int> temp_label_counters_;
};

