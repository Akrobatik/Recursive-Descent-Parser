// never include <winnt.h>
// TOKEN_INFORMATION_CLASS::TokenType 와 TokenType 충돌

// #include <fstream>
#include <fstream>
// #include <iostream>
#include <iostream>
// #include <map>
#include <map>
// #include <set>
#include <set>
// #include <string>
#include <string>
// #include <vector>
#include <vector>

// #include "syntax.h"
#include "syntax.h"

enum class TokenType : int {
  IDENT,      // identifier
  NUMBER,     // const number
  LPAREN,     // (
  RPAREN,     // )
  MUL,        // *
  DIV,        // /
  ADD,        // +
  SUB,        // -
  ASSIGN,     // :=
  SEMICOLON,  // ;
  END,        // eof

  UNDEFINED,  // undefined
};

TokenType next_token;
std::string token_string;

TokenType backup_token;
std::string backup_token_string;

std::ifstream input_file;
std::map<std::string, double> variable_symbol_table;
std::set<std::string> identifier_symbol_table;

std::string statement_string;
std::vector<std::string> exception_resolve_insert_list;  // statement 오류 복구 중 추가된 terminal 리스트
std::vector<std::string> exception_resolve_remove_list;  // statement 오류 복구 중 삭제된 terminal 리스트

int ident_count;
int number_count;
int operator_count;

// ASCII 코드 0 ~ 32 사이 값인 지 반환
bool isWhiteSpace(int c);
// identifier 시작 문자 [a-zA-Z_] 가 맞는 지 반환
bool isIdentifier(int c);
// number 시작 문자 [0-9] 가 맞는 지 반환
bool isNumber(int c);

void lexical();
void lexicalIdentifier();
void lexicalNumber();

// next_token 와 매개변수 token_type 이 같은 지 반환하는 함수
bool match(TokenType token_type);

void errorInsert(TokenType insert_token_type, const std::string& insert_token_string);
void errorRemove();

SyntaxNode* terminal();
SyntaxNode* factor();
SyntaxNode* term();
SyntaxNode* expression();
SyntaxNode* statement();

// statement 를 출력하는 함수
void printStatement(const std::string& statement_string);
// ID, CONST, OP 출력하는 함수
void printTerminalList(int ident_count, int number_count, int operator_count);
// throw 된 오류없이 파싱된 결과 출력함수 (OK, Warning)
void printParseSuccess(const std::vector<std::string>& insert_list, const std::vector<std::string>& remove_list);
// throw 된 오류가 있는 결과 출력함수 (Error)
void printParseError(const std::string& error_message);

int main(int argc, char* argv[]) {
  // 프로세스 실행 인수로 들어온 파일 경로가 없는 경우 오류 메시지 출력
  if (argc < 2) {
    std::cout << "[Error] require file path as argument" << std::endl;
  
    return 0;
  }
  
  // 프로세스 실행 인수로 들어온 파일 open
  input_file.open(argv[1]);
  // 입력 파일이 열렸는 지 확인 후 오류 메시지 출력
  if (!input_file.is_open()) {
    std::cout << "[Error] file can not open" << std::endl;
  
    return 0;
  }

  // initialize variables
  bool normal_program = true;

  next_token = backup_token = TokenType::UNDEFINED;
  lexical();

  // Grammar
  // <program> → <statements>
  // <statements> → <statement> | <statement><semi_colon><statements>
  while (!match(TokenType::END)) {
    // statement 분석 결과를 저장하는 변수 초기화
    bool parse_error = false;
    bool eval_error = false;
    std::string error_messagse;

    statement_string.clear();
    exception_resolve_insert_list.clear();
    exception_resolve_remove_list.clear();

    ident_count = 0;
    number_count = 0;
    operator_count = 0;

    SyntaxNode* statement_node = nullptr;

    try {
      statement_node = statement();
      if (match(TokenType::SEMICOLON)) {
        terminal();
      } else {  // 세미콜론이 없는 경우 예외처리
        errorInsert(TokenType::SEMICOLON, ";");
      }
    } catch (const SyntaxException& e) {
      normal_program = false;
      parse_error = true;
      error_messagse = e.what();
    }

    // statement 구문 분석 성공시 evaluate
    if (statement_node) {
      try {
        statement_node->eval();
      } catch (const SyntaxException& e) {
        normal_program = false;
        eval_error = true;
        error_messagse = e.what();
      }
    }

    if (parse_error) {
      printStatement(statement_string);
      printTerminalList(ident_count, number_count, operator_count);
      printParseError(error_messagse);
    } else if (eval_error) {
      printStatement(statement_string);
      printTerminalList(statement_node->GetIdentCount(), statement_node->GetNumberCount(), statement_node->GetOperatorCount());
      printParseError(error_messagse);
    } else {
      printStatement(statement_string);
      printTerminalList(statement_node->GetIdentCount(), statement_node->GetNumberCount(), statement_node->GetOperatorCount());
      printParseSuccess(exception_resolve_insert_list, exception_resolve_remove_list);
    }

    std::cout << std::endl;
  }

  std::cout << "Result ==> ";
  if (normal_program) {  // 정상 프로그램
    for (auto& variable : variable_symbol_table) {
      std::cout << variable.first << ": " << variable.second << "; ";
    }
  } else {  // 비정상 프로그램
    for (auto& identifier : identifier_symbol_table) {
      std::cout << identifier << ": Unknown; ";
    }
  }

  return 0;
}

bool isWhiteSpace(int c) {
  return (0 <= c && c <= 32);
}

bool isIdentifier(int c) {
  if ('a' <= c && c <= 'z') {
    return true;
  } else if ('A' <= c && c <= 'Z') {
    return true;
  } else if (c == '_') {
    return true;
  }

  return false;
}

bool isNumber(int c) {
  return ('0' <= c && c <= '9');
}

void lexical() {
  while (1) {
    // 다음 문자 읽기
    char c = input_file.get();
    // 입력 파일이 끝났는 지 확인
    if (input_file.eof() || c == EOF) {
      next_token = TokenType::END;
      token_string.clear();

      return;
    }

    // white-space 인 경우 continue
    if (isWhiteSpace(c)) {
      continue;
    }

    // 식별자 token 인지 확인 후 처리
    if (isIdentifier(c)) {
      // 파일 포인터 처리
      input_file.unget();

      lexicalIdentifier();

      return;
    }

    // number token 인지 확인 후 처리
    if (isNumber(c)) {
      // 파일 포인터 처리
      input_file.unget();

      lexicalNumber();

      return;
    }

    // 단일 문자가 token string 인 경우 처리
    switch (c) {
      case '(':
        next_token = TokenType::LPAREN;
        token_string = "(";
        break;

      case ')':
        next_token = TokenType::RPAREN;
        token_string = ")";
        break;

      case '*':
        next_token = TokenType::MUL;
        token_string = "*";
        break;

      case '/':
        next_token = TokenType::DIV;
        token_string = "/";
        break;

      case '+':
        if (match(TokenType::IDENT) || match(TokenType::NUMBER) || match(TokenType::RPAREN)) {  // 연산자로 처리하는 것이 우선순위인 경우
          next_token = TokenType::ADD;
          token_string = "+";
        } else {
          if (isNumber(input_file.peek())) {  // 숫자
            // 파일 포인터 처리
            input_file.unget();

            lexicalNumber();
          } else {  // 연산자
            next_token = TokenType::ADD;
            token_string = "+";
          }
        }
        break;

      case '-':
        if (match(TokenType::IDENT) || match(TokenType::NUMBER) || match(TokenType::RPAREN)) {  // 연산자로 처리하는 것이 우선순위인 경우
          next_token = TokenType::SUB;
          token_string = "-";
        } else {
          if (isNumber(input_file.peek())) {  // 숫자
            // 파일 포인터 처리
            input_file.unget();

            lexicalNumber();
          } else {  // 연산자
            next_token = TokenType::SUB;
            token_string = "-";
          }
        }
        break;

      case ':':
        if (input_file.peek() == '=') {
          input_file.get();  // 파일 포인터 '=' 만큼 이동

          next_token = TokenType::ASSIGN;
          token_string = ":=";
        } else {
          next_token = TokenType::UNDEFINED;
          token_string.clear();
        }
        break;

      case ';':
        next_token = TokenType::SEMICOLON;
        token_string = ";";
        break;

      default:
        next_token = TokenType::UNDEFINED;
        token_string = c;
        break;
    }

    return;
  }
}

void lexicalIdentifier() {
  enum {
    FRONT,  // [a-zA-Z_]
    NEXT,   // [a-zA-Z0-9_]
  } step = FRONT;

  next_token = TokenType::IDENT;
  token_string.clear();

  while (1) {
    // 다음 문자 읽기
    char c = input_file.get();
    // 입력 파일이 끝났는 지 확인
    if (input_file.eof() || c == EOF) {
      next_token = TokenType::END;
      token_string.clear();

      return;
    }

    switch (step) {
      case FRONT: {
        step = NEXT;

        bool suitable = [&]() -> bool {
          if ('a' <= c && c <= 'z') {
            return true;
          } else if ('A' <= c && c <= 'Z') {
            return true;
          } else if (c == '_') {
            return true;
          }

          return false;
        }();

        if (suitable) {
          token_string += c;
        } else {
          input_file.unget();

          return;
        }
      } break;

      case NEXT: {
        bool suitable = [&]() -> bool {
          if ('a' <= c && c <= 'z') {
            return true;
          } else if ('A' <= c && c <= 'Z') {
            return true;
          } else if ('0' <= c && c <= '9') {
            return true;
          } else if (c == '_') {
            return true;
          }

          return false;
        }();

        if (suitable) {
          token_string += c;
        } else {
          input_file.unget();

          return;
        }
      } break;
    }
  }
}

void lexicalNumber() {
  enum {
    FIRST,    // - | + | 0-9
    INTEGER,  // . | 0-9
    DECIMAL,  // 0-9

  } step = FIRST;

  next_token = TokenType::NUMBER;
  token_string.clear();

  while (1) {
    // 다음 문자 읽기
    char c = input_file.get();
    // 입력 파일이 끝났는 지 확인
    if (input_file.eof() || c == EOF) {
      next_token = TokenType::END;
      token_string.clear();

      return;
    }

    switch (step) {
      case FIRST: {
        step = INTEGER;

        bool suitable = [&]() -> bool {
          if ('0' <= c && c <= '9') {
            return true;
          } else if (c == '+' || c == '-') {
            return true;
          }

          return false;
        }();

        if (suitable) {
          token_string += c;
        } else {
          input_file.unget();

          return;
        }
      } break;

      case INTEGER: {
        bool suitable = [&]() -> bool {
          if ('0' <= c && c <= '9') {
            return true;
          } else if (c == '.') {
            return true;
          }

          return false;
        }();

        if (suitable) {
          token_string += c;

          if (c == '.') {  // decimal point
            step = DECIMAL;
          }
        } else {
          input_file.unget();

          return;
        }
      } break;

      case DECIMAL: {
        bool suitable = [&]() -> bool {
          if ('0' <= c && c <= '9') {
            return true;
          }

          return false;
        }();

        if (suitable) {
          token_string += c;
        } else {
          input_file.unget();

          return;
        }
      } break;
    }
  }
}

bool match(TokenType token_type) {
  return (token_type == next_token);
}

void errorInsert(TokenType insert_token_type, const std::string& insert_token_string) {
  exception_resolve_insert_list.push_back(insert_token_string);

  backup_token = insert_token_type;
  backup_token_string = insert_token_string;

  statement_string.append(insert_token_string);
}

void errorRemove() {
  exception_resolve_remove_list.push_back(token_string);

  next_token = backup_token;
  token_string = backup_token_string;

  lexical();
}

SyntaxNode* terminal() {
  SyntaxNode* terminal_node = nullptr;

  switch (next_token) {
    case TokenType::IDENT:
      statement_string.append(token_string);
      identifier_symbol_table.insert(token_string);
      ident_count++;
      terminal_node = new SyntaxIdent(token_string, variable_symbol_table);
      break;

    case TokenType::NUMBER:
      statement_string.append(token_string);
      number_count++;
      terminal_node = new SyntaxNumber(std::stod(token_string));
      break;

    case TokenType::LPAREN:
    case TokenType::RPAREN:
      statement_string.append(token_string);
      break;

    case TokenType::MUL:
      statement_string.append(" " + token_string + " ");
      operator_count++;
      terminal_node = new SyntaxMul();
      break;

    case TokenType::DIV:
      statement_string.append(" " + token_string + " ");
      operator_count++;
      terminal_node = new SyntaxDiv();
      break;

    case TokenType::ADD:
      statement_string.append(" " + token_string + " ");
      operator_count++;
      terminal_node = new SyntaxAdd();
      break;

    case TokenType::SUB:
      statement_string.append(" " + token_string + " ");
      operator_count++;
      terminal_node = new SyntaxSub();
      break;

    case TokenType::ASSIGN:
      statement_string.append(" " + token_string + " ");
      terminal_node = new SyntaxAssign();
      break;

    case TokenType::SEMICOLON:
      statement_string.append(token_string);
      break;
  }

  lexical();  // terminal 처리 후 다음 token 확인

  return terminal_node;
}

SyntaxNode* factor() {
  // Grammar
  // <factor> → <left_paren><expression><right_paren> | <ident> | <const>

  SyntaxNode* factor_node = nullptr;

  if (match(TokenType::LPAREN)) {  // <left_paren><expression><right_paren>
    terminal();
    factor_node = expression();
    if (match(TokenType::RPAREN)) {
      terminal();
    } else {
      errorInsert(TokenType::RPAREN, ")");
    }
  } else if (match(TokenType::IDENT)) {  // <ident>
    factor_node = terminal();
  } else if (match(TokenType::NUMBER)) {  // <const>
    factor_node = terminal();
  } else {  // <factor> 에 해당하는 토큰이 없는 예외 처리
    do {
      if (match(TokenType::SEMICOLON) || match(TokenType::END)) {  // statement 나 file 이 끝난 경우 처리 불가능한 예외
        throw SyntaxException(SyntaxError::FACTOR_NOT_FOUND);
      }

      errorRemove();
    } while (!match(TokenType::LPAREN) && !match(TokenType::IDENT) && !match(TokenType::NUMBER));

    factor_node = factor();
  }

  return factor_node;
}

SyntaxNode* term() {
  // Grammar
  // <term> → <factor><factor_tail>
  // <factor_tail> → <mult_op><factor><factor_tail> | ε
  //
  // Summary
  // <term> → <factor> | <factor><mult_op><term>

  SyntaxNode* term_node = nullptr;

  term_node = factor();                                  // <factor>
  if (match(TokenType::MUL) || match(TokenType::DIV)) {  // <mult_op><term>
    SyntaxNode* left_child = term_node;
    SyntaxBinaryOperator* mul_div_node = (SyntaxBinaryOperator*)terminal();
    SyntaxNode* right_child = term();

    mul_div_node->SetChild(left_child, right_child);
    term_node = mul_div_node;
  }

  return term_node;
}

SyntaxNode* expression() {
  // Grammar
  // <expression> → <term><term_tail>
  // <term_tail> → <add_op><term><term_tail> | ε
  //
  // Summary
  // <expression> → <term> | <term><add_op><expression>

  SyntaxNode* expression_node = nullptr;

  expression_node = term();                              // <term>
  if (match(TokenType::ADD) || match(TokenType::SUB)) {  // <add_op><expression>
    SyntaxNode* left_child = expression_node;
    SyntaxBinaryOperator* add_sub_node = (SyntaxBinaryOperator*)terminal();
    SyntaxNode* right_child = expression();

    add_sub_node->SetChild(left_child, right_child);
    expression_node = add_sub_node;
  }

  return expression_node;
}

SyntaxNode* statement() {
  // Grammar
  // <statement> → <ident><assignment_op><expression>

  SyntaxNode* statement_node = nullptr;

  if (match(TokenType::IDENT)) {
    SyntaxNode* left_child = terminal();
    if (match(TokenType::ASSIGN)) {
      SyntaxBinaryOperator* assign_node = (SyntaxBinaryOperator*)terminal();
      SyntaxNode* right_child = expression();

      assign_node->SetChild(left_child, right_child);
      statement_node = assign_node;
    } else {
      do {
        if (match(TokenType::SEMICOLON) || match(TokenType::END)) {  // statement 나 file 이 끝난 경우 처리 불가능한 예외
          throw SyntaxException(SyntaxError::OPERATOR_ASSIGNMENT_NOT_FOUND);
        }

        errorRemove();
      } while (!match(TokenType::ASSIGN));

      SyntaxBinaryOperator* assign_node = (SyntaxBinaryOperator*)terminal();
      SyntaxNode* right_child = expression();

      assign_node->SetChild(left_child, right_child);
      statement_node = assign_node;
    }
  } else {
    do {
      if (match(TokenType::SEMICOLON) || match(TokenType::END)) {  // statement 나 file 이 끝난 경우 처리 불가능한 예외
        throw SyntaxException(SyntaxError::IDENTIFIER_NOT_FOUND);
      }

      errorRemove();
    } while (!match(TokenType::IDENT));

    statement_node = statement();
  }

  return statement_node;
}

void printStatement(const std::string& statement_string) {
  std::cout << statement_string << std::endl;
}

void printTerminalList(int ident_count, int number_count, int operator_count) {
  std::cout << "ID: " << ident_count << "; CONST: " << number_count << "; OP: " << operator_count << ";" << std::endl;
}

void printParseSuccess(const std::vector<std::string>& insert_list, const std::vector<std::string>& remove_list) {
  if (insert_list.empty() && remove_list.empty()) {  // 처리된 예외가 없는 경우
    std::cout << "(OK)" << std::endl;
  } else {  // 처리된 예외가 있는 경우
    std::cout << "(Warning)";

    if (!insert_list.empty()) {
      std::cout << " required ( ";
      for (auto& inserted : insert_list) {
        std::cout << "\"" << inserted << "\" ";
      }
      std::cout << ")";
    }

    if (!remove_list.empty()) {
      std::cout << " unwanted ( ";
      for (auto& removed : remove_list) {
        std::cout << "\"" << removed << "\" ";
      }
      std::cout << ")";
    }

    std::cout << std::endl;
  }
}

void printParseError(const std::string& error_message) {
  std::cout << "(Error) " << error_message << std::endl;
}