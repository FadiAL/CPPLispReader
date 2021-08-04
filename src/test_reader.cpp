#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"

#include "reader.hpp"

#include <sstream>

using lisp_reader::Tokenizer;
using lisp_reader::Token;
using lisp_reader::TokenType;
using lisp_reader::TokenValue;

TEST_CASE("Knows when it can and cannot read anymore", "[reader]") {
  {
    std::istringstream iss("");
    Tokenizer tokenizer(iss);
    REQUIRE(!tokenizer.canRead());
  }
  {
    std::istringstream iss("Non-empty");
    Tokenizer tokenizer(iss);
    REQUIRE(tokenizer.canRead());
  }
}

TEST_CASE("Can read standalone parenthesis", "[reader]") {
  {
    std::istringstream iss("(");
    Tokenizer tokenizer(iss);
    REQUIRE(tokenizer.read() == Token{TokenType::OPEN_PARENTHESIS, std::optional<TokenValue>()});
    REQUIRE(!tokenizer.canRead());
  }

  {
    std::istringstream iss(")");
    Tokenizer tokenizer(iss);
    REQUIRE(tokenizer.read() == Token{TokenType::CLOSE_PARENTHESIS, std::optional<TokenValue>()});
    REQUIRE(!tokenizer.canRead());
  }
}

TEST_CASE("Can read several consecutive parenthesis", "[reader]") {
  {
    std::istringstream iss("()");
    Tokenizer tokenizer(iss);
    REQUIRE(tokenizer.read() == Token{TokenType::OPEN_PARENTHESIS, std::optional<TokenValue>()});
    REQUIRE(tokenizer.read() == Token{TokenType::CLOSE_PARENTHESIS, std::optional<TokenValue>()});
    REQUIRE(!tokenizer.canRead());
  }
  {
    std::istringstream iss("())()");
    Tokenizer tokenizer(iss);
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
    REQUIRE(tokenizer.read() == Token{TokenType::STRING, std::string("Hello, World")});
    REQUIRE(!tokenizer.canRead());
  }
  {
    std::istringstream iss("\"\"");
    Tokenizer tokenizer(iss);
    REQUIRE(tokenizer.read() == Token{TokenType::STRING, std::string("")});
    REQUIRE(!tokenizer.canRead());
  }
}

TEST_CASE("Can read standalone comments", "[reader]") {
  {
    std::istringstream iss("; Test Comment");
    Tokenizer tokenizer(iss);
    REQUIRE(tokenizer.read() == Token{TokenType::COMMENT, std::string("Test Comment")});
    REQUIRE(!tokenizer.canRead());
  }
  {
    std::istringstream iss(";Test Comment");
    Tokenizer tokenizer(iss);
    REQUIRE(tokenizer.read() == Token{TokenType::COMMENT, std::string("Test Comment")});
    REQUIRE(!tokenizer.canRead());
  }
  {
    std::istringstream iss(";;; Test Comment ;;");
    Tokenizer tokenizer(iss);
    REQUIRE(tokenizer.read() == Token{TokenType::COMMENT, std::string("Test Comment ;;")});
    REQUIRE(!tokenizer.canRead());
  }
  {
    std::istringstream iss(";;;Test Comment;;  ");
    Tokenizer tokenizer(iss);
    REQUIRE(tokenizer.read() == Token{TokenType::COMMENT, std::string("Test Comment;;  ")});
    REQUIRE(!tokenizer.canRead());
  }
}

TEST_CASE("Can read standalone symbols", "[reader]") {
  {
    std::istringstream iss("Test");
    Tokenizer tokenizer(iss);
    REQUIRE(tokenizer.read() == Token{TokenType::SYMBOL, std::string("Test")});
    REQUIRE(!tokenizer.canRead());
  }
  {
    std::istringstream iss("23abc");
    Tokenizer tokenizer(iss);
    REQUIRE(tokenizer.read() == Token{TokenType::SYMBOL, std::string("23abc")});
    REQUIRE(!tokenizer.canRead());
  }
  {
    std::istringstream iss("ABC\\\"BCA");
    Tokenizer tokenizer(iss);
    REQUIRE(tokenizer.read() == Token{TokenType::SYMBOL, std::string("ABC\"BCA")});
    REQUIRE(!tokenizer.canRead());
  }
  {
    std::istringstream iss("ABC\\ BCA");
    Tokenizer tokenizer(iss);
    REQUIRE(tokenizer.read() == Token{TokenType::SYMBOL, std::string("ABC BCA")});
    REQUIRE(!tokenizer.canRead());
  }
  {
    std::istringstream iss("ds.ds");
    Tokenizer tokenizer(iss);
    REQUIRE(tokenizer.read() == Token{TokenType::SYMBOL, std::string("ds.ds")});
    REQUIRE(!tokenizer.canRead());
  }
  {
    std::istringstream iss("|dsa dsa|");
    Tokenizer tokenizer(iss);
    REQUIRE(tokenizer.read() == Token{TokenType::SYMBOL, std::string("dsa dsa")});
    REQUIRE(!tokenizer.canRead());
  }
  {
    std::istringstream iss("+");
    Tokenizer tokenizer(iss);
    REQUIRE(tokenizer.read() == Token{TokenType::SYMBOL, std::string("+")});
    REQUIRE(!tokenizer.canRead());
  }
  {
    std::istringstream iss("-");
    Tokenizer tokenizer(iss);
    REQUIRE(tokenizer.read() == Token{TokenType::SYMBOL, std::string("-")});
    REQUIRE(!tokenizer.canRead());
  }
  {
    std::istringstream iss("-+-+");
    Tokenizer tokenizer(iss);
    REQUIRE(tokenizer.read() == Token{TokenType::SYMBOL, std::string("-+-+")});
    REQUIRE(!tokenizer.canRead());
  }
  {
    std::istringstream iss("123-456");
    Tokenizer tokenizer(iss);
    REQUIRE(tokenizer.read() == Token{TokenType::SYMBOL, std::string("123-456")});
    REQUIRE(!tokenizer.canRead());
  }
  {
    std::istringstream iss("43.4e-34.4");
    Tokenizer tokenizer(iss);
    REQUIRE(tokenizer.read() == Token{TokenType::SYMBOL, std::string("43.4e-34.4")});
    REQUIRE(!tokenizer.canRead());
  }
}

TEST_CASE("Can read multiple consecutive symbols", "[reader]") {
  {
    std::istringstream iss("Hello World");
    Tokenizer tokenizer(iss);
    REQUIRE(tokenizer.read() == Token{TokenType::SYMBOL, std::string("Hello")});
    REQUIRE(tokenizer.read() == Token{TokenType::SYMBOL, std::string("World")});
    REQUIRE(!tokenizer.canRead());
  }
  {
    std::istringstream iss("Hello |Hello World| World");
    Tokenizer tokenizer(iss);
    REQUIRE(tokenizer.read() == Token{TokenType::SYMBOL, std::string("Hello")});
    REQUIRE(tokenizer.read() == Token{TokenType::SYMBOL, std::string("Hello World")});
    REQUIRE(tokenizer.read() == Token{TokenType::SYMBOL, std::string("World")});
    REQUIRE(!tokenizer.canRead());
  }
}

TEST_CASE("Can read standalone integers", "[reader]") {
  {
    std::istringstream iss("1234");
    Tokenizer tokenizer(iss);
    REQUIRE(tokenizer.read() == Token{TokenType::INT, 1234});
    REQUIRE(!tokenizer.canRead());
  }
  {
    std::istringstream iss("+1");
    Tokenizer tokenizer(iss);
    REQUIRE(tokenizer.read() == Token{TokenType::INT, 1});
    REQUIRE(!tokenizer.canRead());
  }
  {
    std::istringstream iss("-3");
    Tokenizer tokenizer(iss);
    REQUIRE(tokenizer.read() == Token{TokenType::INT, -3});
    REQUIRE(!tokenizer.canRead());
  }
  {
    std::istringstream iss("-032");
    Tokenizer tokenizer(iss);
    REQUIRE(tokenizer.read() == Token{TokenType::INT, -32});
    REQUIRE(!tokenizer.canRead());
  }
}

TEST_CASE("Can read standalone floats") {
  {
    std::istringstream iss("32.321");
    Tokenizer tokenizer(iss);
    REQUIRE(tokenizer.read() == Token{TokenType::FLOAT, 32.321f});
    REQUIRE(!tokenizer.canRead());
  }
  {
    std::istringstream iss("-43.2");
    Tokenizer tokenizer(iss);
    REQUIRE(tokenizer.read() == Token{TokenType::FLOAT, -43.2f});
    REQUIRE(!tokenizer.canRead());
  }
  {
    std::istringstream iss("32e-3");
    Tokenizer tokenizer(iss);
    REQUIRE(tokenizer.read() == Token{TokenType::FLOAT, 32e-3f});
    REQUIRE(!tokenizer.canRead());
  }
  {
    std::istringstream iss("32.4e4");
    Tokenizer tokenizer(iss);
    REQUIRE(tokenizer.read() == Token{TokenType::FLOAT, 32.4e4f});
    REQUIRE(!tokenizer.canRead());
  }
}

TEST_CASE("Can read more complex input", "[reader]") {
  {
    std::istringstream iss("(\"Hello, World\")");
    Tokenizer tokenizer(iss);
    REQUIRE(tokenizer.read() == Token{TokenType::OPEN_PARENTHESIS, std::optional<TokenValue>()});
    REQUIRE(tokenizer.read() == Token{TokenType::STRING, std::string("Hello, World")});
    REQUIRE(tokenizer.read() == Token{TokenType::CLOSE_PARENTHESIS, std::optional<TokenValue>()});
    REQUIRE(!tokenizer.canRead());
  }
}
