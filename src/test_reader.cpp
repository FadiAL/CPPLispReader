#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"

#include "reader.hpp"

#include <vector>
#include <sstream>

using lisp_reader::StreamTokenizer;
using lisp_reader::StringTokenizer;
using lisp_reader::Tokenizer;
using lisp_reader::Token;
using lisp_reader::TokenType;
using lisp_reader::TokenValue;

// Helper methods
template <typename T>
void checkTokenizerOutput(Tokenizer<T> &tok, const std::vector<Token> &tokens) {
  REQUIRE(tok.canRead());

  for(const Token &token : tokens) REQUIRE(tok.read() == token);

  REQUIRE(!tok.canRead());
}
void checkStringTokenizerOutput(std::string_view str, const std::vector<Token> &tokens) {
  StringTokenizer tok(str);

  checkTokenizerOutput(tok, tokens);
}


TEST_CASE("Knows when it can and cannot read anymore", "[reader]") {
  {
    StringTokenizer tokenizer("");
    REQUIRE(!tokenizer.canRead());
  }
  {
    StringTokenizer tokenizer("Non-empty");
    REQUIRE(tokenizer.canRead());
  }
}

TEST_CASE("Can read standalone parenthesis", "[reader]") {
  checkStringTokenizerOutput("(", {Token{TokenType::OPEN_PARENTHESIS, std::optional<TokenValue>()}});
  checkStringTokenizerOutput(")", {Token{TokenType::CLOSE_PARENTHESIS, std::optional<TokenValue>()}});
}

TEST_CASE("Can read several consecutive parenthesis", "[reader]") {
  checkStringTokenizerOutput("()", {Token{TokenType::OPEN_PARENTHESIS, std::optional<TokenValue>()},
				    Token{TokenType::CLOSE_PARENTHESIS, std::optional<TokenValue>()}});
  checkStringTokenizerOutput("())()", {Token{TokenType::OPEN_PARENTHESIS, std::optional<TokenValue>()},
				       Token{TokenType::CLOSE_PARENTHESIS, std::optional<TokenValue>()},
				       Token{TokenType::CLOSE_PARENTHESIS, std::optional<TokenValue>()},
				       Token{TokenType::OPEN_PARENTHESIS, std::optional<TokenValue>()},
				       Token{TokenType::CLOSE_PARENTHESIS, std::optional<TokenValue>()}});
}

TEST_CASE("Can read standalone string literals", "[reader]") {
  checkStringTokenizerOutput("\"Hello, World\"", {Token{TokenType::STRING, std::string("Hello, World")}});
  checkStringTokenizerOutput("\"\"", {Token{TokenType::STRING, std::string("\"\"")}});
}

TEST_CASE("Can read standalone comments", "[reader]") {
  checkStringTokenizerOutput("; Test Comment", {Token{TokenType::COMMENT, std::string("Test Comment")}});
  checkStringTokenizerOutput(";Test Comment", {Token{TokenType::COMMENT, std::string("Test Comment")}});
  checkStringTokenizerOutput(";;; Test Comment ;;", {Token{TokenType::COMMENT, std::string("Test Comment ;;")}});
  checkStringTokenizerOutput(";;;Test Comment;;  ", {Token{TokenType::COMMENT, std::string("Test Comment;;  ")}});
}

TEST_CASE("Can read standalone symbols", "[reader]") {
  checkStringTokenizerOutput("Test", {Token{TokenType::SYMBOL, std::string("Test")}});
  checkStringTokenizerOutput("23abc", {Token{TokenType::SYMBOL, std::string("23abc")}});
  checkStringTokenizerOutput("ABC\\\"BCA", {Token{TokenType::SYMBOL, std::string("ABC\"BCA")}});
  checkStringTokenizerOutput("ABC\\ BCA", {Token{TokenType::SYMBOL, std::string("ABC BCA")}});
  checkStringTokenizerOutput("ds.ds", {Token{TokenType::SYMBOL, std::string("ds.ds")}});
  checkStringTokenizerOutput("|dsa dsa|", {Token{TokenType::SYMBOL, std::string("dsa dsa")}});
  checkStringTokenizerOutput("+", {Token{TokenType::SYMBOL, std::string("+")}});
  checkStringTokenizerOutput("-", {Token{TokenType::SYMBOL, std::string("-")}});
  checkStringTokenizerOutput("-+-+", {Token{TokenType::SYMBOL, std::string("-+-+")}});
  checkStringTokenizerOutput("123-456", {Token{TokenType::SYMBOL, std::string("123-456")}});
  checkStringTokenizerOutput("43.4e-34.4", {Token{TokenType::SYMBOL, std::string("43.4e-34.4")}});
  checkStringTokenizerOutput("23.3e", {Token{TokenType::SYMBOL, std::string("23.3e")}});
  checkStringTokenizerOutput("23.3d", {Token{TokenType::SYMBOL, std::string("23.3d")}});
  checkStringTokenizerOutput("23232/", {Token{TokenType::SYMBOL, std::string("23232/")}});
  checkStringTokenizerOutput("32/-3", {Token{TokenType::SYMBOL, std::string("32/-3")}});
}

TEST_CASE("Can read multiple consecutive symbols", "[reader]") {
  checkStringTokenizerOutput("Hello World", {Token{TokenType::SYMBOL, std::string("Hello")},
					     Token{TokenType::SYMBOL, std::string("World")}});
  checkStringTokenizerOutput("Hello |Hello World| World", {Token{TokenType::SYMBOL, std::string("Hello")},
							   Token{TokenType::SYMBOL, std::string("Hello World")},
							   Token{TokenType::SYMBOL, std::string("World")}});
}

TEST_CASE("Can read standalone integers", "[reader]") {
  checkStringTokenizerOutput("1234", {Token{TokenType::INT, 1234}});
  checkStringTokenizerOutput("+1", {Token{TokenType::INT, 1}});
  checkStringTokenizerOutput("-3", {Token{TokenType::INT, -3}});
  checkStringTokenizerOutput("-032", {Token{TokenType::INT, -32}});
}

TEST_CASE("Can read standalone floats", "[reader]") {
  checkStringTokenizerOutput("32.321", {Token{TokenType::FLOAT, 32.321f}});
  checkStringTokenizerOutput("-43.2", {Token{TokenType::FLOAT, -43.2f}});
  checkStringTokenizerOutput("32e-3", {Token{TokenType::FLOAT, 32e-3f}});
  checkStringTokenizerOutput("32.4e4", {Token{TokenType::FLOAT, 32.4e4f}});
}

TEST_CASE("Can read standalone doubles", "[reader]") {
  checkStringTokenizerOutput("32d1", {Token{TokenType::DOUBLE, 32e1}});
  checkStringTokenizerOutput("1.2d3", {Token{TokenType::DOUBLE, 1.2e3}});
  checkStringTokenizerOutput("3.4d-4", {Token{TokenType::DOUBLE, 3.4e-4}});
}

TEST_CASE("Can read more complex input", "[reader]") {
    checkStringTokenizerOutput("(\"Hello, World\")",
			       {Token{TokenType::OPEN_PARENTHESIS, std::optional<TokenValue>()},
				Token{TokenType::STRING, std::string("Hello, World")},
				Token{TokenType::CLOSE_PARENTHESIS, std::optional<TokenValue>()}});
}
