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
  enum class TokenType { OPEN_PARENTHESIS, CLOSE_PARENTHESIS, SYMBOL, COMMENT, INT, DOUBLE, FLOAT, FRACTION, STRING, END };
  // Labels for each of the above Token Types
  const std::array<std::string, static_cast<int>(TokenType::END)> tokenTypeLabels{
    "OPEN_PARENTHESIS", "CLOSE_PARENTHESIS", "SYMBOL", "COMMENT", "INT", "DOUBLE", "FLOAT", "FRACTION", "STRING"
      };

  // Function that returns the label for a given TokenType, used to contain the static_cast's
  std::string_view getLabel(TokenType tt) {
    return tokenTypeLabels[static_cast<int>(tt)];
  }

  // FRACTION Tokens are represented by a tuple (num, den)
  // This class allows printing, and keeps fraction simplified
  class Fraction {
  public:
    Fraction(int num, int den)
      : _num(num), _den(den) {
      _simplify();
    }

    // Returns whether the fraction can be represented by an integer (denominator = 1)
    bool isInt() const {return _den == 1;}

    int getNum() const {return _num;}
    int getDen() const {return _den;}

    void setNum(int num) {_num = num; _simplify();}
    void setDen(int den) {_den = den; _simplify();}

    // Need to overload this for the std::visit function
    bool operator==(const Fraction &rhs) const {
      return _num == rhs._num && _den == rhs._den;
    }
  private:
    int _num;
    int _den;

    // Helper function, mainly used by _simplify
    int _gcd(int a, int b) {
      while(b != 0) {
	int t = b;
	b = a % b;
	a = t;
      }

      return a;
    }

    void _simplify() {
      int gcd = _gcd(_num, _den);

      _num /= gcd;
      _den /= gcd;
    }
  };

  std::ostream &operator<<(std::ostream &os, const Fraction &frac) {
    os << frac.getNum() << '/' << frac.getDen();
  }

  // The value of a token, can be any one of these
  typedef std::variant<std::string, int, float, double, Fraction> TokenValue;
  // Use a bit of type traits to define what each TokenType maps to in the variant
  // By default it contains a string
  template <TokenType T>
  struct TokenTypeValue { typedef std::string ValType; };
  template <>
  struct TokenTypeValue<TokenType::INT> { typedef int ValType; };
  template <>
  struct TokenTypeValue<TokenType::DOUBLE> { typedef double ValType; };
  template <>
  struct TokenTypeValue<TokenType::FLOAT> { typedef double ValType; };
  template <>
  struct TokenTypeValue<TokenType::FRACTION> { typedef Fraction ValType; };

  // Helper method that accesses the value of a token with a given TokenType using the type traits define above
  template <TokenType T>
  inline typename TokenTypeValue<T>::ValType getTokenVal(const TokenValue &t) {
    return std::get<TokenTypeValue<T>::ValType>(t);
  }

  // The list of symbols we are looking for when parsing, they have the same name as the TokenType above
  namespace token_chars {
    constexpr char OPEN_PARENTHESIS  = '(';
    constexpr char CLOSE_PARENTHESIS = ')';
    constexpr char STRING            = '"';
    constexpr char COMMENT           = ';';
  } // token_chars

  // Represent both a type and whatever value it may contain, some tokens may not have any value
  typedef std::pair<TokenType, std::optional<TokenValue> > Token;

  // Helper function for printing a type-value token pair
  inline std::ostream &operator<<(std::ostream &os, const Token &t) {
    // If it's a literal type add this for extra information
    if(t.first >= TokenType::INT)
      os << "Literal ";

    // Print out the label of this token type using static_cast
    os << getLabel(t.first) << ": ";

    // Get the type of this token and print that out, we use the previously defined type traits here
    switch(t.first) {
    case TokenType::OPEN_PARENTHESIS:
      os << '(';
      break;
    case TokenType::CLOSE_PARENTHESIS:
      os << ')';
      break;
    default:			// For all other cases we just do a visit and print the second element
      std::visit([&os](auto &val) {os << val;}, *t.second);
      break;
    }
    return os;
  }

  // Takes in a stream and produces tokens for consumption
  class Tokenizer {
  public:
    Tokenizer(std::istream &is)
      : _is(is) {
      // Disable whitespace-skipping
      _is >> std::noskipws;
    }

    // Cannot copy/assign/move (&_is deletes those methods anyway, no need to explicitly delete here)

    // Check if we can (or have) any more tokens to read by peeking a single character ahead and checking stream state
    bool canRead() const {_is.peek(); return _is.good();}

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
	_is.putback(token_chars::STRING);
	// Read a string into ret
	ret.second = _readStr();
	break;
      }

      // Return the token here
      return ret;
    }
    Token peek() const;
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
    std::string _readCmt() {
      // Read until we stop finding ';'
      char c;
      _is >> c;

      // If the first character is not a comment character, throw an error
      if(c != token_chars::COMMENT)
	throw "Missing semicolon at start of comment";

      // Keep reading characters until we hit a non-COMMENT one
      while(c == token_chars::COMMENT & !_is.eof())
	_is >> c;
      // Put back the first non-COMMENT character we found
      _is.putback(c);

      // Start reading the actual comment, can be done quickly with getline
      std::string ret;
      std::getline(_is, ret);

      // Left-Trim to remove spaces after the double-semicolons
      ret.erase(ret.begin(), std::find_if_not(ret.begin(), ret.end(), [](unsigned char c) {
									return std::isspace(c);
								      }));

      return ret;
    }
    std::string _readSym() {
      // Some rules:
      //  1. No spaces (unless escaped)
      //  2. Not purely numeric
      //  3. .'s allowed, but not completely made of .'s
      //  4. None of the RESERVED_SYM_CHARS, unless surrounded by | or escaped with a backslash
      std::string ret;
      char c;
      _is >> c;
      // How many dots found so far
      int dotCnt = 0;
      while(_is.good() && c != ' ') {
	switch(c) {
	case '.':
	  ++dotCnt;
	  ret.push_back(c);
	  break;
	case '\\':		// Read the next character literally
	  _is >> c;
	  if(!_is.good())
	    throw "Cannot have unescaped backslash (\\) in a symbol name";
	  ret.push_back(c);
	  break;
	case '|':			// Read the next 'n' characters literally, until we hit a second |
	  _is >> c;
	  while(c != '|') {
	    if(!_is.good())
	      throw "Did not find matching vertical bar (|) in symbol name";
	    ret.push_back(c);
	    _is >> c;
	  }
	  break;
	default:			// Read the next char normally according to the above rules
	  if(std::find(RESERVED_SYM_CHARS.begin(), RESERVED_SYM_CHARS.end(), c) != RESERVED_SYM_CHARS.end())
	    throw "Unescaped illegal character in symbol";

	  // Character is legal, add it
	  ret.push_back(c);
	  break;
	}
	_is >> c;
      }

      // If the dotCnt is the same length as the whole symbol, throw an error
      if(dotCnt > 0 && dotCnt == ret.size())
	throw "Too many dots in symbol";

      return ret;
    }
  };
};				// lisp_reader

#endif // CPPLISPREADER_READER_HPP
