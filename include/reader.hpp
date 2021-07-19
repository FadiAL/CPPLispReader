#ifndef CPPLISPREADER_READER_HPP
#define CPPLISPREADER_READER_HPP

#include <iostream>
#include <string>
#include <optional>
#include <variant>
#include <exception>

namespace lisp_reader {
  // Represent different types of tokens
  // Although both numeric and string are literals (also atoms) we split them here to distinguish them better
  // Every TokenType after COMMENT (INT-STRING) is a literal
  enum class TokenType { OPEN_PARENTHESIS, CLOSE_PARENTHESIS, SYMBOL, COMMENT, INT, DOUBLE, FLOAT, STRING };
  // The value of a token, can be any one of these
  typedef TokenValue std::variant<std::string, int, float, double>;

  // The list of symbols we are looking for when parsing, they have the same name as the TokenType above
  namespace token_chars {
    constexpr char OPEN_PARENTHESIS  = '(';
    constexpr char CLOSE_PARENTHESIS = ')';
    constexpr char STRING            = '"';
    constexpr char COMMENT           = ';';
  } // token_chars

  // Represent both a type and whatever value it may contain, some tokens may not have any value
  typedef std::pair<TokenType, std::optional<TokenValue> > Token;

  // Takes in a stream and produces tokens for consumption
  class Tokenizer {
  public:
    Tokenizer(std::istream &is)
      : _is(is) {}

    // Check if we can (or have) any more tokens to read
    bool canRead() {return _is.good();}

    // Reads a token from the input stream
    // NOTE: Undefined behavior if read without checking canRead() first
    Token read() {
      // To be returned
      Token ret;

      // Otherwise, read a single character
      char c;
      _is >> c;
      // Based on the first character we find, the rest of the characters must be parsed accordingly
      switch(c) {
      case token_chars::OPEN_PARENTHESIS:
	ret.first = TokenType::OPEN_PARENTHESIS;
	// Keep TokenValue None
	break;
      case token_chars::CLOSE_PARENTHESIS:
	ret.first = TokenType::CLOSE_PARENTHESIS;
	// Keep TokenValue None
	break;
      case token_chars::STRING:
	ret.first = TokenType::STRING;
	// Put back the double-quotes we found
	is.putback(token_chars::STRING);
	// Read a string into ret
	ret.second = _readStr();
	break;
      }

      // Return the token here
      return ret;
    }
  }
    Token peek();
private:
  std::istream &_is;

  // A list of private helper methods
  std::string _readStr() {
    char c;
    _is >> c;
    if(c != token_chars::STRING)
      throw "Missing double-quotes at start of string literal";

    // Consume characters until we hit a "
    // We don't really know how large the string will be, so we can't reliably reserve size
    std::string ret;
    _is >> c;
    while(c != token_chars::STRING && !_is.eof()) {
      ret.push_back(c);
      _is >> c;
    }
    if(c != token_chars::STRING)
      throw "Missing closing double-quotes for string literal";

    // If we got here we got the full string
    return ret;
  }
  std::string _readCmt();
  std::string _readSym();
  int         _readInt();
  double      _readDbl();
  float       _readFlt();
};
};				// lisp_reader

#endif // CPPLISPREADER_READER_HPP
