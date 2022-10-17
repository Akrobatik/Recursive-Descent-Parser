#pragma once

// #include <string>
#include <string>

enum class SyntaxError {
  SEMICOLON_NOT_FOUND,
  CONSTANT_NOT_FOUND,
  IDENTIFIER_NOT_FOUND,
  LPAREN_NOT_FOUND,
  RPAREN_NOT_FOUND,
  OPERATOR_NOT_FOUND,
  FACTOR_NOT_FOUND,
  OPERATOR_ASSIGNMENT_NOT_FOUND,
  UNDEFINED_OPERATOR,
  UNDEFINED_IDENTIFIER,
  UNKNOWN_TERMINAL,
  DIVIDE_BY_ZERO,
};

class SyntaxException : public std::exception {
 public:
  SyntaxException(SyntaxError syntax_error) { SetMessage(syntax_error, ""); };
  SyntaxException(SyntaxError syntax_error, const std::string& option) { SetMessage(syntax_error, option); };

  virtual const char* what() const { return message_.c_str(); };

 private:
  void SetMessage(SyntaxError syntax_error, const std::string& option) {
    switch (syntax_error) {
      case SyntaxError::SEMICOLON_NOT_FOUND:
        message_ = "semi-colon not found";
        break;

      case SyntaxError::CONSTANT_NOT_FOUND:
      case SyntaxError::IDENTIFIER_NOT_FOUND:
      case SyntaxError::LPAREN_NOT_FOUND:
      case SyntaxError::RPAREN_NOT_FOUND:
      case SyntaxError::OPERATOR_NOT_FOUND:
      case SyntaxError::FACTOR_NOT_FOUND:
      case SyntaxError::UNDEFINED_OPERATOR:
        message_ = "invalid statement";
        break;

      case SyntaxError::OPERATOR_ASSIGNMENT_NOT_FOUND:
        message_ = "assignment operator not found";
        break;

      case SyntaxError::UNDEFINED_IDENTIFIER:
        message_ = "undefined variable (" + option + ") reference";
        break;

      case SyntaxError::UNKNOWN_TERMINAL:
        message_ = "unknown terminal found";
        break;

      case SyntaxError::DIVIDE_BY_ZERO:
        message_ = "divide by zero";
        break;

      default:
        message_ = "unknwon exception";
        break;
    }
  }

  std::string message_;
};