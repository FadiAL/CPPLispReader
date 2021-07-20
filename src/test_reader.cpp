#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"

#include "reader.hpp"

#include <sstream>

using lisp_reader::Tokenizer;
using lisp_reader::Token;
using lisp_reader::TokenType;
using lisp_reader::TokenValue;

TEST_CASE("Can read standalone parenthesis", "[reader]") {
  {
    std::istringstream iss("(");
    Tokenizer tokenizer(iss);
    REQUIRE(tokenizer.canRead());
    REQUIRE(tokenizer.read() == Token{TokenType::OPEN_PARENTHESIS, std::optional<TokenValue>()});
    REQUIRE(!tokenizer.canRead());
  }

  {
    std::istringstream iss(")");
    Tokenizer tokenizer(iss);
    REQUIRE(tokenizer.canRead());
    REQUIRE(tokenizer.read() == Token{TokenType::CLOSE_PARENTHESIS, std::optional<TokenValue>()});
    REQUIRE(!tokenizer.canRead());
  }
}

TEST_CASE("Can read several consecutive parenthesis", "[reader]") {
  {
    std::istringstream iss("()");
    Tokenizer tokenizer(iss);
    REQUIRE(tokenizer.canRead());
    REQUIRE(tokenizer.read() == Token{TokenType::OPEN_PARENTHESIS, std::optional<TokenValue>()});
    REQUIRE(tokenizer.read() == Token{TokenType::CLOSE_PARENTHESIS, std::optional<TokenValue>()});
    REQUIRE(!tokenizer.canRead());
  }
  {
    std::istringstream iss("())()");
    Tokenizer tokenizer(iss);
    REQUIRE(tokenizer.canRead());
    REQUIRE(tokenizer.read() == Token{TokenType::OPEN_PARENTHESIS, std::optional<TokenValue>()});
    REQUIRE(tokenizer.read() == Token{TokenType::CLOSE_PARENTHESIS, std::optional<TokenValue>()});
    REQUIRE(tokenizer.read() == Token{TokenType::CLOSE_PARENTHESIS, std::optional<TokenValue>()});
    REQUIRE(tokenizer.read() == Token{TokenType::OPEN_PARENTHESIS, std::optional<TokenValue>()});
    REQUIRE(tokenizer.read() == Token{TokenType::CLOSE_PARENTHESIS, std::optional<TokenValue>()});
    REQUIRE(!tokenizer.canRead());
  }
}

TEST_CASE("Can read standalone string literals", "[reader]") {
  {
    std::istringstream iss("\"Hello, World\"");
    Tokenizer tokenizer(iss);
    REQUIRE(tokenizer.canRead());
    REQUIRE(tokenizer.read() == Token{TokenType::STRING, std::string("Hello, World")});
    REQUIRE(!tokenizer.canRead());
  }
  {
    std::istringstream iss("\"\"");
    Tokenizer tokenizer(iss);
    REQUIRE(tokenizer.canRead());
    REQUIRE(tokenizer.read() == Token{TokenType::STRING, std::string("")});
    REQUIRE(!tokenizer.canRead());
  }
}

TEST_CASE("Can read more complex input", "[reader]") {
  {
    std::istringstream iss("(\"Hello, World\")");
    Tokenizer tokenizer(iss);
    REQUIRE(tokenizer.canRead());
    REQUIRE(tokenizer.read() == Token{TokenType::OPEN_PARENTHESIS, std::optional<TokenValue>()});
    REQUIRE(tokenizer.read() == Token{TokenType::STRING, std::string("Hello, World")});
    REQUIRE(tokenizer.read() == Token{TokenType::CLOSE_PARENTHESIS, std::optional<TokenValue>()});
  }
}
