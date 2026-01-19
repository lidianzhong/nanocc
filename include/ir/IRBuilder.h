#pragma once

#include "ir/IR.h"
#include "ir/IRModule.h"

#include <memory>
#include <unordered_map>

class IRBuilder {
public:
  IRBuilder(IRModule *module) : module_(module) {}
  ~IRBuilder() = default;

  void SetCurrentFunction(Function *func);
  void EndFunction();

  BasicBlock *CreateBlock(const std::string &name);
  void SetInsertPoint(BasicBlock *bb);

  template <typename... T> void Emit(Opcode op, T &&...args) {
    cur_bb_->Append(
        std::make_unique<Instruction>(op, std::forward<T>(args)...));
  }

  Value CreateAlloca(const std::string &type, const std::string &var_name = "");
  Value CreateGlobalAlloc(const std::string &type, const std::string &var_name,
                          Value init_val);
  Value CreateArrayAlloca(const std::string &type, const std::string &var_name,
                          int size);
  Value CreateGlobalArrayAlloca(const std::string &type,
                                const std::string &var_name, int size,
                                const std::vector<Value> &init_vals);
  Value CreateGetElemPtr(const Value &base_addr, const Value &index);
  Value CreateLoad(const Value &addr);
  void CreateStore(const Value &value, const Value &addr);

  void CreateBranch(const Value &cond, BasicBlock *then_bb,
                    const std::vector<Value> &then_args, BasicBlock *else_bb,
                    const std::vector<Value> &else_args);
  void CreateJump(BasicBlock *target_bb, const std::vector<Value> &args = {});
  void CreateReturn(const Value &value);
  void CreateReturn();

  Value CreateUnaryOp(const std::string &op, const Value &value);
  Value CreateBinaryOp(const std::string &op, const Value &lhs,
                       const Value &rhs);

  Value CreateCall(const std::string &func_name, const std::vector<Value> &args,
                   bool has_return);

  void DeclareFunction(const std::string &name, const std::string &ret_type,
                       const std::vector<std::string> &param_types);

public:
  IRModule *module_ = nullptr;
  Function *cur_func_ = nullptr;
  BasicBlock *cur_bb_ = nullptr;

private:
  Value NewTempReg_();
  Value NewTempAddr_(const std::string &addr_name = "");
  std::string NewTempLabel_(const std::string &prefix = "");

  int temp_reg_id_ = 0;
  std::unordered_map<std::string, int> temp_addr_counters_;
  std::unordered_map<std::string, int> temp_label_counters_;
};
