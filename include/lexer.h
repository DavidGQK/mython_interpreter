#pragma once

#include <iosfwd>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>

namespace parse {

namespace token_type {
struct Number {  // Lexeme "number"
    int value;   // number
};

struct Id {             // lexeme "identificator"
    std::string value;  // Name of the identifier
};

struct Char {    // lexeme "symbol"
    char value;  // symbol code
};

struct String {  // lexeme "string constant"
    std::string value;
};

struct Class {};    // lexeme "class"
struct Return {};   // lexeme "return"
struct If {};       // lexeme "if"
struct Else {};     // lexeme "else"
struct Def {};      // lexeme "def"
struct Newline {};  // lexeme "end of line"
struct Print {};    // lexeme "print"
struct Indent {};   // lexeme "indentation increase", corresponds to two spaces
struct Dedent {};   // lexeme "indentation reduction"
struct Eof {};      // lexeme "end of file"
struct And {};      // lexeme "and"
struct Or {};       // lexeme "or"
struct Not {};      // lexeme "not"
struct Eq {};       // lexeme "=="
struct NotEq {};    // lexeme "!="
struct LessOrEq {};     // lexeme «<=»
struct GreaterOrEq {};  // lexeme «>=»
struct None {};         // lexeme «None»
struct True {};         // lexeme «True»
struct False {};        // lexeme «False»
}  // namespace token_type



using TokenBase
    = std::variant<token_type::Number, token_type::Id, token_type::Char, token_type::String,
                   token_type::Class, token_type::Return, token_type::If, token_type::Else,
                   token_type::Def, token_type::Newline, token_type::Print, token_type::Indent,
                   token_type::Dedent, token_type::And, token_type::Or, token_type::Not,
                   token_type::Eq, token_type::NotEq, token_type::LessOrEq, token_type::GreaterOrEq,
                   token_type::None, token_type::True, token_type::False, token_type::Eof>;

struct Token : TokenBase {
    using TokenBase::TokenBase;

    template <typename T>
    [[nodiscard]] bool Is() const {
        return std::holds_alternative<T>(*this);
    }

    template <typename T>
    [[nodiscard]] const T& As() const {
        return std::get<T>(*this);
    }

    template <typename T>
    [[nodiscard]] const T* TryAs() const {
        return std::get_if<T>(this);
    }
};

bool operator==(const Token& lhs, const Token& rhs);
bool operator!=(const Token& lhs, const Token& rhs);

std::ostream& operator<<(std::ostream& os, const Token& rhs);

class LexerError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class Lexer {
public:
    explicit Lexer(std::istream& input);

    // Returns a reference to the current token or token_type::Eof if the token stream has ended
    [[nodiscard]] const Token& CurrentToken() const;

    // Returns the next token, or token_type::Eof if the token stream has ended
    Token NextToken();

    // If the current token is of type T, the method returns a reference to it.
    // Otherwise, the method throws a LexerError exception
    template <typename T>
    const T& Expect() const {
        using namespace std::literals;
        if (current_token_.Is<T>()) {
            return current_token_.As<T>();
        }
        throw LexerError("Not implemented"s);
    }

    // The method checks that the current token is of type T and that the token itself contains value.
    // Otherwise, the method throws a LexerError exception
    template <typename T, typename U>
    void Expect(const U &value) const {
        using namespace std::literals;
        if (current_token_.Is<T>() &&
            current_token_.As<T>().value == value) {
            return;
        }
        throw LexerError("Not implemented"s);
    }

    // If the next token is of type T, the method returns a reference to it.
    // Otherwise the method throws a LexerError exception
    template <typename T>
    const T& ExpectNext() {
        NextToken();
        return Expect<T>();
    }

    // The method checks that the next token is of type T and that the token itself contains value.
    // Otherwise the method throws a LexerError exception
    template <typename T, typename U>
    void ExpectNext(const U &value) {
        NextToken();
        Expect<T>(value);
    }

private:
    std::istream &input_;
    bool start_of_line_ = true; // Is the current token the first token on the line
    uint32_t current_indent_ = 0; // current indent
    uint32_t line_indent_ = 0; // the number of indents at the beginning of the current line
    Token current_token_; // current token value

    // reads the next token from the stream and stores it in current_token_
    void ReadNextToken();
    // move to the next line
    void NextLine();
    // processes the comment
    void ParseComment();
    // handles the end of the file
    void ParseEOF();
    // handles the end of the line
    void ParseLineEnd();
    // handles gaps
    void ParseSpaces();
    // handles the indent at the beginning of the line
    void ParseIndent();
    // processes a meaningful token
    void ParseToken();
    // processes the name (keyword or identifier)
    void ParseName();
    // processes the symbol
    void ParseChar();
};

}  // namespace parse

namespace util {

static std::unordered_map<std::string, parse::Token> KeyWords =
{
 {"class",   parse::token_type::Class{}},
 {"return",  parse::token_type::Return{}},
 {"if",      parse::token_type::If{}},
 {"else",    parse::token_type::Else{}},
 {"def",     parse::token_type::Def{}},
 {"print",   parse::token_type::Print{}},
 {"and",     parse::token_type::And{}},
 {"or",      parse::token_type::Or{}},
 {"not",     parse::token_type::Not{}},
 {"None",    parse::token_type::None{}},
 {"True",    parse::token_type::True{}},
 {"False",   parse::token_type::False{}}
};

static std::unordered_map<std::string, parse::Token> DualSymbols =
{
 {"==",  parse::token_type::Eq{}},
 {"!=",  parse::token_type::NotEq{}},
 {">=",  parse::token_type::GreaterOrEq{}},
 {"<=",  parse::token_type::LessOrEq{}}
};

// checks if the character is a number
bool IsNum(char ch);
// checks if the character is a letter
bool IsAlpha(char ch);
// checks if the character is a number or a letter
bool IsAlNum(char ch);
// checks if a character is a letter or an underscore character
bool IsAlNumLL(char ch);
// reads the string from the opening quote to the closing quote, taking into account the escaped characters
std::string ReadString(std::istream &input);
// reads an identifier consisting of letters, numbers and underscores
std::string ReadName(std::istream &input);
// reads an integer
int ReadNumber(std::istream &input);
// counts all consecutive spaces in the string and returns the number of spaces
size_t CountSpaces(std::istream &input);
// reads the whole string
std::string ReadLine(std::istream &input);

} // namespace util
