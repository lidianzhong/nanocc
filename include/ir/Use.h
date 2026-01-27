#pragma once

namespace ldz {

class User;
class Value;

class Use {
public:
  Use(const Use &U) = delete;

private:
  /// Constructor
  Use(User *Parent) : parent_(Parent) {}

public:
  operator Value *() const { return val_; }
  Value *get() const { return val_; }

  /// Returns the User that contains this Use.
  ///
  /// For an instruction operand, for example, this will return the
  /// instruction.
  User *getUser() const { return parent_; };

  void set(Value *V);

  Value *operator=(Value *RHS);
  const Use &operator=(const Use &RHS);

  Value *operator->() { return val_; }
  const Value *operator->() const { return val_; }

  Use *getNext() const { return next_; }

private:
  Value *val_ = nullptr;
  Use *next_ = nullptr;
  Use **prev_ = nullptr;
  User *parent_ = nullptr;

  friend class Value;
  friend class User;

  void addToList(Use **List) {
    next_ = *List;
    if (next_)
      next_->prev_ = &next_;
    prev_ = List;
    *prev_ = this;
  }

  void removeFromList() {
    if (prev_) {
      *prev_ = next_;
      if (next_) {
        next_->prev_ = prev_;
        next_ = nullptr;
      }

      prev_ = nullptr;
    }
  }
};

} // namespace ldz