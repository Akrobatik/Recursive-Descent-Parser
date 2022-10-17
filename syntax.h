#pragma once

// #include "syntax-exception.h"
#include "syntax-exception.h"

// #include <map>
#include <map>
// #include <string>
#include <string>

class SyntaxNode {
 public:
  virtual double eval() const = 0;

  virtual int GetIdentCount() const = 0;
  virtual int GetNumberCount() const = 0;
  virtual int GetOperatorCount() const = 0;
};

class SyntaxBinaryOperator : public SyntaxNode {
 public:
  SyntaxBinaryOperator() { SetChild(nullptr, nullptr); }

  ~SyntaxBinaryOperator() {
    if (left_) delete (left_);
    if (right_) delete (right_);
  }

  virtual double eval() const = 0;

  virtual int GetIdentCount() const { return left_->GetIdentCount() + right_->GetIdentCount(); }
  virtual int GetNumberCount() const { return left_->GetNumberCount() + right_->GetNumberCount(); }
  virtual int GetOperatorCount() const { return left_->GetOperatorCount() + right_->GetOperatorCount() + 1; }

  virtual void SetChild(SyntaxNode* left, SyntaxNode* right) {
    left_ = left;
    right_ = right;
  }

 protected:
  SyntaxNode* left_;
  SyntaxNode* right_;
};

class SyntaxIdent : public SyntaxNode {
 public:
  explicit SyntaxIdent(const std::string& ident, std::map<std::string, double>& variables)
      : ident_(ident), variables_(variables) {}

  virtual double eval() const { return get(); }

  virtual int GetIdentCount() const { return 1; }
  virtual int GetNumberCount() const { return 0; }
  virtual int GetOperatorCount() const { return 0; }

  double get() const {
    if (variables_.find(ident_) == variables_.end()) {
      throw SyntaxException(SyntaxError::UNDEFINED_IDENTIFIER, ident_);
    }

    return variables_.find(ident_)->second;
  }

  void set(double value) const {
    if (variables_.find(ident_) == variables_.end()) {
      variables_.erase(ident_);
    }

    variables_.insert(std::make_pair(ident_, value));
  }

 private:
  std::string ident_;
  std::map<std::string, double>& variables_;
};

class SyntaxNumber : public SyntaxNode {
 public:
  explicit SyntaxNumber(double value)
      : value_(value) {}

  virtual double eval() const { return value_; }

  virtual int GetIdentCount() const { return 0; }
  virtual int GetNumberCount() const { return 1; }
  virtual int GetOperatorCount() const { return 0; }

 private:
  double value_;
};

class SyntaxMul : public SyntaxBinaryOperator {
 public:
  explicit SyntaxMul() { SetChild(nullptr, nullptr); }
  explicit SyntaxMul(SyntaxNode* left, SyntaxNode* right) { SetChild(left, right); }

  virtual double eval() const { return left_->eval() * right_->eval(); }
};

class SyntaxDiv : public SyntaxBinaryOperator {
 public:
  explicit SyntaxDiv() { SetChild(nullptr, nullptr); }
  explicit SyntaxDiv(SyntaxNode* left, SyntaxNode* right) { SetChild(left, right); }

  virtual double eval() const {
    if (fabs(right_->eval()) < DBL_EPSILON) {
      throw SyntaxException(SyntaxError::DIVIDE_BY_ZERO);
    }

    return left_->eval() / right_->eval();
  }
};

class SyntaxAdd : public SyntaxBinaryOperator {
 public:
  explicit SyntaxAdd() { SetChild(nullptr, nullptr); }
  explicit SyntaxAdd(SyntaxNode* left, SyntaxNode* right) { SetChild(left, right); }

  virtual double eval() const { return left_->eval() + right_->eval(); }
};

class SyntaxSub : public SyntaxBinaryOperator {
 public:
  explicit SyntaxSub() { SetChild(nullptr, nullptr); }
  explicit SyntaxSub(SyntaxNode* left, SyntaxNode* right) { SetChild(left, right); }

  virtual double eval() const { return left_->eval() - right_->eval(); }
};

class SyntaxAssign : public SyntaxBinaryOperator {
 public:
  explicit SyntaxAssign() { SetChild(nullptr, nullptr); }
  explicit SyntaxAssign(SyntaxIdent* left, SyntaxNode* right) { SetChild(left, right); }

  virtual double eval() const {
    ((SyntaxIdent*)left_)->set(right_->eval());
    return ((SyntaxIdent*)left_)->get();
  }

  virtual int GetIdentCount() const { return left_->GetIdentCount() + right_->GetIdentCount(); }
  virtual int GetNumberCount() const { return left_->GetNumberCount() + right_->GetNumberCount(); }
  virtual int GetOperatorCount() const { return left_->GetOperatorCount() + right_->GetOperatorCount(); }

  virtual void SetChild(SyntaxIdent* left, SyntaxNode* right) {
    left_ = left;
    right_ = right;
  }
};