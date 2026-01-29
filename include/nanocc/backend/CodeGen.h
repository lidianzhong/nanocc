#pragma once

#include "FrameInfo.h"
#include "koopa.h"

class ProgramCodeGen {
public:
  ProgramCodeGen() = default;
  ~ProgramCodeGen() = default;

  void Emit(const koopa_raw_program_t &program);
  static size_t CalcTypeSize(koopa_raw_type_t ty);

private:
  void EmitDataSection(const koopa_raw_slice_t &values);
  void EmitGlobalAlloc(const koopa_raw_value_t &value);
  void EmitInitializer(const koopa_raw_value_t &init);

  void EmitTextSection();
};

class FunctionCodeGen {
public:
  FunctionCodeGen() = default;
  ~FunctionCodeGen() = default;

  void EmitFunction(const koopa_raw_function_t &func);

private:
  void EmitPrologue();
  void EmitEpilogue();
  void EmitSlice(const koopa_raw_slice_t &slice);
  void EmitBasicBlock(const koopa_raw_basic_block_t &bb);
  void EmitValue(const koopa_raw_value_t &value);

  void AllocateStackSpace();

  size_t GetStackOffset(koopa_raw_value_t val);

  void EmitBlockArgs(koopa_raw_basic_block_t bb, koopa_raw_slice_t args);

  void SafeLoad(const std::string &reg, int offset);
  void SafeStore(const std::string &reg, int offset);
  void LoadReg(const std::string &reg, koopa_raw_value_t val);

  koopa_raw_function_t func_;
  FrameInfo stack_frame_;
};
