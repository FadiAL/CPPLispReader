#ifndef CPPLISPREADER_READER_HPP
#define CPPLISPREADER_READER_HPP

#include <iostream>
#include <algorithm>
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
  inline typename TokenTypeValue<T>::ValType &getTokenVal(TokenValue &t) {
    return std::get<typename TokenTypeValue<T>::ValType>(t);
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

  // For symbol tokens, these characters MUST be escaped
  const std::array<char, 10> RESERVED_SYM_CHARS{ '(', ')', '"', '\'', '`', ',', ':', ';', '\\', '|' };

  // Different ways of escaping a sequence
  enum class Escapes{NONE, BACKSLASH, PIPE};

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
      // Reset the token value to a symbol with empty string
      _ret = Token{TokenType::SYMBOL, ""};

      char c;
      c = _is.peek();
      // Based on the first character we find, the rest of the characters must be parsed accordingly
      switch(c) {
      case token_chars::OPEN_PARENTHESIS: // Parse Open Parenthesis
	_ret.first = TokenType::OPEN_PARENTHESIS;
	_ret.second = std::nullopt;
	// Consume the character
	_is >> c;
	break;
      case token_chars::CLOSE_PARENTHESIS: // Parse Close Parenthesis
	_ret.first = TokenType::CLOSE_PARENTHESIS;
	_ret.second = std::nullopt;
	// Consume the character
	_is >> c;
	break;
      case token_chars::STRING:	// Parse String
	_ret.first = TokenType::STRING;
	// Read a string into ret
	_readStr();
	break;
      case token_chars::COMMENT: // Parse Comment
	_ret.first = TokenType::COMMENT;
	// Read a comment into ret
	_readCmt();
	break;
      default:			// Can be either a symbol or a number here
	_statefulRead();
	break;
      }

      // Return the token here
      return _ret;
    }
    Token peek() const;
  private:
    std::istream &_is;
    // The current token being constructed, we fill this up while parsing
    Token _ret;

    // A list of private helper methods
    void _readStr() {
      char c;
      _is >> c;
      if(c != token_chars::STRING)
	throw "Missing double-quotes at start of string literal";

      // Consume characters until we hit a "
      _is >> c;
      std::string &val = getTokenVal<TokenType::STRING>(*_ret.second);
      while(c != token_chars::STRING && !_is.eof()) {
        val.push_back(c);
	_is >> c;
      }
      if(c != token_chars::STRING)
	throw "Missing closing double-quotes for string literal";
    }
    void _readCmt() {
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
      std::string &val = getTokenVal<TokenType::COMMENT>(*_ret.second);
      // Append the string
      val.insert(val.end(), ret.begin(), ret.end());
    }

    // Checks if a character is a valid start to a number (is +/- or a digit)
    bool _isValidNumStart(char c) {
      return (c == '-' || c == '+' || _isDigit(c));
    }

    // Will attempt to determine whether a space-delimited word is a numeric type or symbol
    void _statefulRead() {
      char c = _is.peek();

      // A flag indicating if something is escaped or not
      Escapes escp = Escapes::NONE;

      // Add to string
      std::string &val = getTokenVal<TokenType::STRING>(*_ret.second);

      // Since this is the first character, we can quickly decide the initial type
      // Assume float
      if(c == '.')
	_ret.first = TokenType::FLOAT;
      // Assume int
      else if(_isValidNumStart(c))
	_ret.first = TokenType::INT;
      // Assume symbol
      else
	_ret.first = TokenType::SYMBOL;

      // Begin the state machine to keep reading and refine the above
      while(_is >> c  && (c != ' ')) {
	// Check if it is escaped
	if(c == '\\') {
	  // Read one extra character
	  if(!(_is >> c)) throw "Cannot end symbol with unescaped backslash";

	  val.push_back(c);
	  continue;
	}
	else if(c == '|') {
	  // Read until we hit another pipe
	  while(_is >> c && c != '|')
	    val.push_back(c);

	  if(c != '|') throw "Unclosed pipe character found";
	  continue;
	}

	// Check if it is a reserved character
	else if(std::find(RESERVED_SYM_CHARS.begin(), RESERVED_SYM_CHARS.end(), c) != RESERVED_SYM_CHARS.end())
	  throw "Unescaped illegal character in symbol";

	// Add character to current token
	val.push_back(c);

	// If something is a digit, no change to the state will occur
	if(_isDigit(c)) continue;

	switch(_ret.first) {
	case TokenType::INT:
	  switch(c) {
	  case '/':
	    if(!_isDigit(_is.peek()))
	      _ret.first = TokenType::SYMBOL;
	    else
	      _ret.first = TokenType::FRACTION;
	    break;
	  case '.':
	    // Still an int, unless the next character is a digit
	    if(_isDigit(_is.peek()))
	      _ret.first = TokenType::FLOAT;
	    break;
	  case 'e':
	    // If the next character is not a sign or a number, this is a symbol
	    if(!_isValidNumStart(_is.peek()))
	      _ret.first = TokenType::SYMBOL;
	    else
	      _ret.first = TokenType::FLOAT;
	    break;
	  case 'd':
	    // If the next character is not a sign or a number, this is a symbol
	    if(!_isValidNumStart(_is.peek()))
	      _ret.first = TokenType::SYMBOL;
	    else
	      _ret.first = TokenType::DOUBLE;
	    break;
	  default:		// None of these characters work, it is a symbol
	    if(!(_isValidNumStart(c) && val.size() == 1))
	      _ret.first = TokenType::SYMBOL;
	    break;
	  }
	  break;
	case TokenType::FRACTION:
	  _ret.first = TokenType::SYMBOL;
	  break;
	case TokenType::FLOAT:
	  if(c == 'e') {	// Ok, we found an exponential
	    // If there was one previously, this is wrong
	    if(std::find(val.begin(), val.end() - 1, 'e') != val.end() - 1)
	      _ret.first = TokenType::SYMBOL;
	    // If there is no further numeric character or (+/-), that is also wrong
	    else if(!_isValidNumStart(_is.peek()))
	      _ret.first = TokenType::SYMBOL;
	    // Otherwise, we are still a float
	  }
	  else if(c == 'd') {	// We have a double
	    // If there was one previously, this is wrong
	    if(std::find(val.begin(), val.end() - 1, 'd') != val.end() - 1)
	      _ret.first = TokenType::SYMBOL;
	    // If there is no further numeric character or (+/-), that is also wrong
	    else if(!_isValidNumStart(_is.peek()))
	      _ret.first = TokenType::SYMBOL;
	    // Otherwise, it is a double
	    _ret.first = TokenType::DOUBLE;
	  }
	  // If this is a +/- with one previous character not being an 'e'
	  else if((c == '+' || c == '-') && val[val.size() - 2] != 'e')
	    _ret.first = TokenType::SYMBOL;
	  // If this is a '.' with a previous e
	  else if(c == '.' && std::find(val.begin(), val.end() - 1, 'e') != val.end() - 1)
	    _ret.first = TokenType::SYMBOL;
	  break;
	case TokenType::DOUBLE:
	  // If the previous character was a 'd', and the current one is +/-, this is fine
	  // Otherwise, it is a symbol
	  if(!(val.size() >= 2 && val[val.size() - 2] == 'd' && (c == '+' || c == '-')))
	    _ret.first = TokenType::SYMBOL;
	default:		// This is a symbol, we can just keep reading as usual here
	  break;
	}

      }

      // If the last character is a + or - or e or d, it is a symbol
      char last = val.back();
      if(last == '+' || last == '-' || last == 'e' || last == 'd') {
	_ret.first = TokenType::SYMBOL;
	return;
      }
      // Parse the read value and check
      switch(_ret.first) {
      case TokenType::SYMBOL:
	{
	  // Check if it is a valid symbol
	  std::size_t dotCnt = 0;
	  for(char c : val)
	    dotCnt += (c == '.');
	  if(dotCnt == val.size())
	    throw "Too many dots";
	}
	break;
      case TokenType::INT:
	_ret.second = std::stoi(val);
	break;
      case TokenType::FLOAT:
	_ret.second = std::stof(val);
	break;
      case TokenType::DOUBLE:
	_ret.second = std::stod(val.replace(val.find('d'), 1, "e"));
	break;
      case TokenType::FRACTION:
	{
	  // Split into substrings, parse each as an int
	  int divLoc = val.find('/');
	  std::string_view lhs(&val[0], divLoc);
	  std::string_view rhs(&val[divLoc + 1], val.size() - (divLoc + 1));

	  // Now, convert both to int's and create a fraction
	  int num = std::stoi(lhs.data());
	  int den = std::stoi(rhs.data());
	  Fraction f(num, den);
	  if(f.isInt()) {
	    _ret.first = TokenType::INT;
	    _ret.second = f.getNum();
	  }
	  else
	    _ret.second = f;
	}
	break;
      }
    }

    bool _isDigit(char c) {
      return c >= 48 && c <= 57;
    }
  };
};				// lisp_reader

#endif // CPPLISPREADER_READER_HPP
