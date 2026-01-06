#pragma once

#include "koopa.h"

#include <cstddef>
#include <iostream>
#include <unordered_map>

class FrameInfo {
public:
  FrameInfo() = default;
  ~FrameInfo() = default;

  void AllocSlot(koopa_raw_value_t value) {
    size_t size = 4; // TODO: support other types
    offset_[value] = stack_size_;
    stack_size_ += size;
  }

  size_t GetOffset(koopa_raw_value_t value) { return offset_.at(value); }

  size_t GetStackSize() { return stack_size_; }

  void Align() { stack_size_ = (stack_size_ + 15) & ~15; }

private:
  size_t stack_size_ = 0;
  std::unordered_map<koopa_raw_value_t, size_t> offset_;
};