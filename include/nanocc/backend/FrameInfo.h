#pragma once

#include "koopa.h"

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <unordered_map>

class FrameInfo {
public:
  FrameInfo() : current_offset_(0), total_frame_size_(0), has_call_(false) {}
  ~FrameInfo() = default;

  void Init(int max_call_args, bool has_call) {
    has_call_ = has_call;
    int args_size = std::max(max_call_args - 8, 0) * 4;
    current_offset_ = args_size;
  }

  void AllocSlot(koopa_raw_value_t value, size_t size) {
    offset_[value] = current_offset_;
    current_offset_ += size;
  }

  size_t GetOffset(koopa_raw_value_t value) { return offset_.at(value); }

  void Finalize() {
    size_t s_plus_a = current_offset_;
    size_t r = has_call_ ? 4 : 0;
    size_t total = s_plus_a + r;
    total_frame_size_ = (total + 15) & ~15;
  }

  size_t GetStackSize() { return total_frame_size_; }
  bool HasCall() const { return has_call_; }

private:
  size_t current_offset_;
  size_t total_frame_size_;
  bool has_call_;
  std::unordered_map<koopa_raw_value_t, size_t> offset_;
};