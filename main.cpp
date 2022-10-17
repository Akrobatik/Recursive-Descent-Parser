// never include <winnt.h>
// TOKEN_INFORMATION_CLASS::TokenType 와 TokenType 충돌

// #include <fstream>
#include <fstream>
// #include <iostream>
#include <iostream>
// #include <map>
#include <map>
// #include <regex>
#include <regex>
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

  PRIORITY_TO_NUMBER,  // +, - 의 우선순위 숫자 > 연산자
};

static const std::regex REGEX_IDENTIFIER("^[a-zA-Z_][a-zA-Z0-9_]*");
static const std::regex REGEX_NUMBER("^[-+]?[0-9]+(\\.[0-9]+)?");

std::ifstream input_file;
std::map<std::string, double> variables;  // 식별자 테이블 (값 포함) <- 정상적인 프로그램일 때만 유효
std::set<std::string> identifiers;        // 식별자 테이블 (식별자 이름만 저장) <- 항상 유효

bool program_parse_success;  // 정상적인 프로그램인지 저장 (예외 throw 시 false)

TokenType next_token;
std::string token_string;

std::string token_raw_string;  // statement 출력을 위해 white-space 를 포함한 token_string
std::string statement_raw_string;
std::vector<std::string> exception_resolve_insert_list;  // statement 오류 복구 중 추가된 terminal 리스트
std::vector<std::string> exception_resolve_remove_list;  // statement 오류 복구 중 삭제된 terminal 리스트

// statement 를 출력하는 함수
void printStatement(const std::string& statement_string);
// ID, CONST, OP 출력하는 함수
void printTerminalList(int ident_count, int number_count, int operator_count);
// throw 된 오류없이 파싱된 결과 출력함수 (OK, Warning)
void printParseSuccess(const std::vector<std::string>& insert_list, const std::vector<std::string>& remove_list);
// throw 된 오류가 있는 결과 출력함수 (Error)
void printParseError(const std::string& error_message);

// ASCII 코드 0 ~ 32 사이 값인 지 반환
bool isWhiteSpace(int c);
// identifier 시작 문자 [a-zA-Z_] 가 맞는 지 반환
bool isIdentifier(int c);
// number 시작 문자 [0-9] 가 맞는 지 반환
bool isNumber(int c);
// lexical 파싱 - allow_exception_throw 은 예외 발생시 throw 가능한지 여부
void lexical(bool allow_exception_throw = true);

// next_token 와 매개변수 token_type 이 같은 지 반환하는 함수
bool match(TokenType token_type);

SyntaxNode* terminal();
SyntaxNode* factor();
SyntaxNode* term();
SyntaxNode* expression();
SyntaxNode* statement();

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

  // Grammar
  // <program> → <statements>
  // <statements> → <statement> | <statement><semi_colon><statements>
  program_parse_success = true;

  lexical();
  while (!match(TokenType::END)) {
    // statement 분석 결과를 저장하는 변수 초기화
    statement_raw_string.clear();
    exception_resolve_insert_list.clear();
    exception_resolve_remove_list.clear();

    SyntaxNode* statement_node = nullptr;

    // statement 구문 분석 시작 전 파일 포인터 저장
    std::streampos file_pointer_backup = input_file.tellg();
    file_pointer_backup -= token_string.length();

    // statement 구문 분석
    try {
      statement_node = statement();
      if (match(TokenType::SEMICOLON)) {
        terminal();
      } else {  // 세미콜론이 없는 경우 예외처리
        exception_resolve_insert_list.push_back(";");
      }
    } catch (const std::exception& e) {  // statement 분석 도중 오류
      // parsing tree 를 순환하지 않고 token 만 처리

      // 다시 처리하기 위해 파일 포인터 복구
      input_file.clear();
      input_file.seekg(file_pointer_backup);

      // statement string 저장 변수 초기화
      statement_raw_string.clear();
      token_raw_string.clear();

      // terminal 개수 저장 변수 선언
      int ident_count = 0;
      int number_count = 0;
      int operator_count = 0;

      do {
        lexical(false);

        if (match(TokenType::IDENT)) {
          ident_count++;
        } else if (match(TokenType::NUMBER)) {
          number_count++;
        } else if (match(TokenType::MUL) || match(TokenType::DIV) || match(TokenType::ADD) || match(TokenType::SUB)) {
          operator_count++;
        }
      } while (!match(TokenType::SEMICOLON) && !match(TokenType::END));

      printStatement(statement_raw_string);
      printTerminalList(ident_count, number_count, operator_count);
      printParseError(e.what());

      program_parse_success = false;
    }

    // statement 구문 분석 성공시 evaluate
    if (statement_node) {
      try {
        statement_node->eval();

        printStatement(statement_raw_string);
        printTerminalList(statement_node->GetIdentCount(), statement_node->GetNumberCount(), statement_node->GetOperatorCount());
        printParseSuccess(exception_resolve_insert_list, exception_resolve_remove_list);
      } catch (const std::exception& e) {  // statement evaluate 도중 오류
        printStatement(statement_raw_string);
        // printTerminalList(statement_node->GetIdentCount(), statement_node->GetNumberCount(), statement_node->GetOperatorCount());
        printParseError(e.what());

        program_parse_success = false;
      }
    }

    std::cout << std::endl;
  }

  std::cout << "Result ==> ";
  if (program_parse_success) {  // 정상적인 프로그램일 때
    for (auto& variable : variables) {
      std::cout << variable.first << ": " << variable.second << "; ";
    }
  } else {  // 비정상적인 프로그램일 때
    for (auto& identifier : identifiers) {
      std::cout << identifier << ": Unknown; ";
    }
  }

  return 0;
}

void printStatement(const std::string& statement_string) {
  // 문자열 앞 white-space 제거
  std::string trim_string = statement_string;
  trim_string.erase(trim_string.begin(), std::find_if_not(trim_string.begin(), trim_string.end(), isWhiteSpace));

  std::cout << trim_string << std::endl;
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

void lexical(bool allow_exception_throw) {
  // 문자열 출력을 위한 변수 처리
  statement_raw_string.append(token_raw_string);
  token_raw_string.clear();

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
      token_raw_string += c;

      continue;
    }

    // 식별자 token 인지 확인 후 처리
    if (isIdentifier(c)) {
      // 파일 포인터 처리
      input_file.unget();                               // 문자열로 읽기 위해 파일 포인터 -1
      std::streampos datum_point = input_file.tellg();  // 파일 포인터 처리를 위한 기준점 저장

      // 문자열 읽기
      std::string input_file_string;
      std::getline(input_file, input_file_string);

      // 정규식을 이용해 식별자 전체 확인
      std::smatch match_identifier;
      std::regex_search(input_file_string, match_identifier, REGEX_IDENTIFIER);

      // 파일 포인터를 기준점에서 식별자 길이만큼 이동
      input_file.seekg(datum_point);
      input_file.seekg(match_identifier[0].str().length(), std::ios::cur);

      // 토큰 관련 변수 세팅
      next_token = TokenType::IDENT;
      token_string = match_identifier[0].str();
      token_raw_string += token_string;

      // 식별자 테이블에 추가
      identifiers.insert(token_string);

      return;
    }

    // number token 인지 확인 후 처리
    if (isNumber(c)) {
      // 위 식별자 처리와 동일
      input_file.unget();
      std::streampos datum_point = input_file.tellg();

      std::string input_file_string;
      std::getline(input_file, input_file_string);

      std::smatch match_number;
      std::regex_search(input_file_string, match_number, REGEX_NUMBER);

      input_file.seekg(datum_point);
      input_file.seekg(match_number[0].str().length(), std::ios::cur);

      next_token = TokenType::NUMBER;
      token_string = match_number[0].str();
      token_raw_string += token_string;

      return;
    }

    // 단일 문자가 token string 인 경우 처리
    switch (c) {
      case '(':
        next_token = TokenType::LPAREN;
        token_string = "(";
        token_raw_string += token_string;
        break;

      case ')':
        next_token = TokenType::RPAREN;
        token_string = ")";
        token_raw_string += token_string;
        break;

      case '*':
        next_token = TokenType::MUL;
        token_string = "*";
        token_raw_string += token_string;
        break;

      case '/':
        next_token = TokenType::DIV;
        token_string = "/";
        token_raw_string += token_string;
        break;

      case '+':
        if (match(TokenType::IDENT) || match(TokenType::NUMBER) || match(TokenType::RPAREN)) {  // 연산자로 처리하는 것이 우선순위인 경우
          next_token = TokenType::ADD;
          token_string = "+";
          token_raw_string += token_string;
        } else {  // 숫자로 처리하는 것이 우선순위인 경우
          input_file.unget();
          std::streampos datum_point = input_file.tellg();

          std::string input_file_string;
          std::getline(input_file, input_file_string);

          std::smatch match_number;
          if (std::regex_search(input_file_string, match_number, REGEX_NUMBER)) {  // 숫자로 처리
            input_file.seekg(datum_point);
            input_file.seekg(match_number[0].str().length(), std::ios::cur);

            next_token = TokenType::NUMBER;
            token_string = match_number[0].str();
            token_raw_string += token_string;
          } else {  // 연산자로 처리
            input_file.seekg(datum_point);
            input_file.get();

            next_token = TokenType::ADD;
            token_string = "+";
            token_raw_string += token_string;
          }
        }
        break;

      case '-':
        // + 처리와 동일
        if (match(TokenType::IDENT) || match(TokenType::NUMBER) || match(TokenType::RPAREN)) {
          next_token = TokenType::SUB;
          token_string = "-";
          token_raw_string += token_string;
        } else {
          input_file.unget();
          std::streampos datum_point = input_file.tellg();

          std::string input_file_string;
          std::getline(input_file, input_file_string);

          std::smatch match_number;
          if (std::regex_search(input_file_string, match_number, REGEX_NUMBER)) {
            input_file.seekg(datum_point);
            input_file.seekg(match_number[0].str().length(), std::ios::cur);

            next_token = TokenType::NUMBER;
            token_string = match_number[0].str();
            token_raw_string += token_string;
          } else {
            input_file.seekg(datum_point);
            input_file.get();

            next_token = TokenType::SUB;
            token_string = "-";
            token_raw_string += token_string;
          }
        }
        break;

      case ':':
        if (input_file.peek() == '=') {  // := 연산자가 맞는 경우
          input_file.get();              // 파일 포인터 '=' 만큼 이동

          next_token = TokenType::ASSIGN;
          token_string = ":=";
          token_raw_string += token_string;
        } else {  // := 연산자가 아닌 경우
          // 처리 불가능한 예외로 판단
          if (allow_exception_throw) {
            throw SyntaxException(SyntaxError::UNKNOWN_TERMINAL);
          }
        }
        break;

      case ';':
        next_token = TokenType::SEMICOLON;
        token_string = ";";
        token_raw_string += token_string;
        break;

      default:
        // 처리 불가능한 예외로 판단
        token_raw_string += c;
        if (allow_exception_throw) {
          throw SyntaxException(SyntaxError::UNKNOWN_TERMINAL);
        }
        break;
    }

    return;
  }
}

bool match(TokenType token_type) {
  return (token_type == next_token);
}

SyntaxNode* terminal() {
  SyntaxNode* terminal_node = nullptr;

  switch (next_token) {
    case TokenType::IDENT:
      terminal_node = new SyntaxIdent(token_string, variables);
      break;

    case TokenType::NUMBER:
      terminal_node = new SyntaxNumber(std::stod(token_string));
      break;

    case TokenType::MUL:
      terminal_node = new SyntaxMul();
      break;

    case TokenType::DIV:
      terminal_node = new SyntaxDiv();
      break;

    case TokenType::ADD:
      terminal_node = new SyntaxAdd();
      break;

    case TokenType::SUB:
      terminal_node = new SyntaxSub();
      break;

    case TokenType::ASSIGN:
      terminal_node = new SyntaxAssign();
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
      // right-paren 없는 경우 예외처리
      exception_resolve_insert_list.push_back(")");
    }
  } else if (match(TokenType::IDENT)) {  // <ident>
    factor_node = terminal();
  } else if (match(TokenType::NUMBER)) {  // <const>
    factor_node = terminal();
  } else {
    // <factor> 에 해당하는 토큰이 없는 예외 처리
    while (!match(TokenType::LPAREN) && !match(TokenType::IDENT) && !match(TokenType::NUMBER)) {
      exception_resolve_remove_list.push_back(token_string);  // 예외 복구 중 삭제된 토큰 저장

      next_token = TokenType::PRIORITY_TO_NUMBER;  // +, - 문자가 숫자로 우선 처리하기 위해 설정
      lexical();

      if (match(TokenType::SEMICOLON) || match(TokenType::END)) {  // statement 나 file 이 끝난 경우 처리 불가능한 예외
        throw SyntaxException(SyntaxError::FACTOR_NOT_FOUND);
      }
    }

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
      // 대입 연산자가 없는 예외 처리
      while (!match(TokenType::ASSIGN)) {
        exception_resolve_remove_list.push_back(token_string);  // 예외 복구 중 삭제된 토큰 저장

        lexical();

        if (match(TokenType::SEMICOLON) || match(TokenType::END)) {  // statement 나 file 이 끝난 경우 처리 불가능한 예외
          throw SyntaxException(SyntaxError::OPERATOR_ASSIGNMENT_NOT_FOUND);
        }
      }

      SyntaxBinaryOperator* assign_node = (SyntaxBinaryOperator*)terminal();
      SyntaxNode* right_child = expression();

      assign_node->SetChild(left_child, right_child);
      statement_node = assign_node;
    }
  } else {
    // <statement> 에 해당하는 token 이 없는 예외 처리
    while (!match(TokenType::IDENT)) {
      exception_resolve_remove_list.push_back(token_string);  // 예외 복구 중 삭제된 토큰 저장

      lexical();

      if (match(TokenType::SEMICOLON) || match(TokenType::END)) {  // statement 나 file 이 끝난 경우 처리 불가능한 예외
        throw SyntaxException(SyntaxError::IDENTIFIER_NOT_FOUND);
      }
    }

    statement_node = statement();
  }

  return statement_node;
}