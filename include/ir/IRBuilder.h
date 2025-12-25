#pragma once

#include <sstream>
#include <string>
#include <unordered_map>

#include "koopa.h"

class IRBuilder {
public:
  IRBuilder() = default;
  ~IRBuilder() = default;

  koopa_raw_program_t Build();
  std::string GetIR() const { return buffer_.str(); }

  void StartFunction(const std::string &name, const std::string &ret_type);
  void EndFunction();
  void CreateBasicBlock(const std::string &label = "entry");

  std::string CreateAlloca(const std::string &type = "i32",
                           const std::string &var_name = "");
  std::string CreateLoad(const std::string &alloc);
  void CreateStore(const std::string &value, const std::string &alloc);

  std::string CreateUnaryOp(const std::string &op, const std::string &value);
  std::string CreateBinaryOp(const std::string &op, const std::string &lhs,
                             const std::string &rhs);
  std::string CreateNumber(int value);
  void CreateReturn(const std::string &value = "");

  std::string NewTempReg_(const std::string &var_name = "");

public:
  std::stringstream buffer_;
  int temp_reg_id_ = 0;
  std::unordered_map<std::string, int> var_reg_counters_;
};