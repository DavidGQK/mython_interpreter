#include "lexer.h"

#include <algorithm>
#include <charconv>
#include <unordered_map>

using namespace std::literals;

namespace util {
    extern std::unordered_map<std::string, parse::Token> KeyWords;
    extern std::unordered_map<std::string, parse::Token> DualSymbols;
}

namespace parse {

bool operator==(const Token& lhs, const Token& rhs) {
    using namespace token_type;

    if (lhs.index() != rhs.index()) {
        return false;
    }
    if (lhs.Is<Char>()) {
        return lhs.As<Char>().value == rhs.As<Char>().value;
    }
    if (lhs.Is<Number>()) {
        return lhs.As<Number>().value == rhs.As<Number>().value;
    }
    if (lhs.Is<String>()) {
        return lhs.As<String>().value == rhs.As<String>().value;
    }
    if (lhs.Is<Id>()) {
        return lhs.As<Id>().value == rhs.As<Id>().value;
    }
    return true;
}

bool operator!=(const Token& lhs, const Token& rhs) {
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& os, const Token& rhs) {
    using namespace token_type;

#define VALUED_OUTPUT(type) \
    if (auto p = rhs.TryAs<type>()) return os << #type << '{' << p->value << '}';

    VALUED_OUTPUT(Number);
    VALUED_OUTPUT(Id);
    VALUED_OUTPUT(String);
    VALUED_OUTPUT(Char);

#undef VALUED_OUTPUT

#define UNVALUED_OUTPUT(type) \
    if (rhs.Is<type>()) return os << #type;

    UNVALUED_OUTPUT(Class);
    UNVALUED_OUTPUT(Return);
    UNVALUED_OUTPUT(If);
    UNVALUED_OUTPUT(Else);
    UNVALUED_OUTPUT(Def);
    UNVALUED_OUTPUT(Newline);
    UNVALUED_OUTPUT(Print);
    UNVALUED_OUTPUT(Indent);
    UNVALUED_OUTPUT(Dedent);
    UNVALUED_OUTPUT(And);
    UNVALUED_OUTPUT(Or);
    UNVALUED_OUTPUT(Not);
    UNVALUED_OUTPUT(Eq);
    UNVALUED_OUTPUT(NotEq);
    UNVALUED_OUTPUT(LessOrEq);
    UNVALUED_OUTPUT(GreaterOrEq);
    UNVALUED_OUTPUT(None);
    UNVALUED_OUTPUT(True);
    UNVALUED_OUTPUT(False);
    UNVALUED_OUTPUT(Eof);

#undef UNVALUED_OUTPUT

    return os << "Unknown token :("sv;
}

Lexer::Lexer(std::istream& input) : input_{input} {
    ReadNextToken();
}

const Token& Lexer::CurrentToken() const {
    return current_token_;
}

Token Lexer::NextToken() {
    ReadNextToken();
    return current_token_;
}

void Lexer::ReadNextToken() {
    char ch = input_.peek();
    if (ch == std::ios::traits_type::eof()) { // have reached the end of the file
        ParseEOF();
    }
    else if (ch == '\n') { // reached the end of the line
        ParseLineEnd();
    }
    else if (ch == '#') { // reached the commentary
        ParseComment();
    }
    else if (ch == ' ') { // reached the space bar.
        ParseSpaces();
    }
    // if there are indents at the beginning of the line - indent until the current indent is equal to the indent in the line
    else if (start_of_line_ && (current_indent_ != line_indent_)) {
        ParseIndent();
    }
    else { // there must be a meaningful token next
        ParseToken();
        start_of_line_ = false; // after the first significant token we consider that we are no longer at the beginning of the line
    }
}

void Lexer::NextLine() {
    util::ReadLine(input_);
    start_of_line_ = true;
    line_indent_ = 0;
}

void Lexer::ParseComment() {
    char c;
    while (input_.get(c)) {
        if (c == '\n') {
            input_.putback(c);
            break;
        }
    }
    ReadNextToken();
}

void Lexer::ParseEOF() {
    if (!start_of_line_) { // if the end of the file is at the end of a non-empty line - return Newline and go to the next line
        NextLine();
        current_token_ = token_type::Newline{};
    } else { // Otherwise we return Eof with the unclosed indents closed beforehand
        if (current_indent_ > 0) {
            --current_indent_;
            current_token_ = token_type::Dedent{};
        } else {
            current_token_ = token_type::Eof{};
        }
    }
}

void Lexer::ParseLineEnd() {
    if (start_of_line_) { // If the end of the line is on an empty line - skip the line and read again
        NextLine();
        ReadNextToken();
    } else { // Otherwise we return Newline
        NextLine();
        current_token_ = token_type::Newline{};
    }
}

void Lexer::ParseSpaces() {
    auto spaces_count = util::CountSpaces(input_);
    if (start_of_line_) { // if this is the beginning of the line - write an indent
        line_indent_ = spaces_count / 2;
    }
    ReadNextToken(); // skip the blanks and read again
}

void Lexer::ParseIndent() {
    if (current_indent_ < line_indent_) {
        ++current_indent_;
        current_token_ = token_type::Indent{};
    } else if (current_indent_ > line_indent_) {
        --current_indent_;
        current_token_ = token_type::Dedent{};
    }
}

void Lexer::ParseToken() {
    char ch = input_.peek();
    if (util::IsNum(ch)) {   // if the next token is a number
        current_token_ = token_type::Number{util::ReadNumber(input_)};
    } else if (util::IsAlNumLL(ch)) {    // If the next token is a name
        ParseName();
    } else if (ch == '\"' || ch == '\'') { // if the next token string
        current_token_ = token_type::String{util::ReadString(input_)};
    } else { // in all other cases, consider that the next token is the symbol
        ParseChar();
    }
}

void Lexer::ParseName() {
    auto name = util::ReadName(input_);
    if (util::KeyWords.count(name) != 0) { // if it is a keyword - assign the appropriate token
        current_token_ = util::KeyWords.at(name);
    } else { // otherwise consider it Id
        current_token_ = token_type::Id{name};
    }
}

void Lexer::ParseChar() {
    std::string sym_pair;
    sym_pair += input_.get();
    sym_pair += input_.peek();
    if (util::DualSymbols.count(sym_pair)) { // if a pair of characters in a row is a token, then we assign
        current_token_ = util::DualSymbols.at(sym_pair);
        input_.get();
    } else { // otherwise we take only one character
      current_token_ = token_type::Char{sym_pair[0]};
    }
}

}  // namespace parse

namespace util {

bool IsNum(char ch) {
    return std::isdigit(static_cast<unsigned char>(ch));
}

bool IsAlpha(char ch) {
    return std::isalpha(static_cast<unsigned char>(ch));
}

bool IsAlNum(char ch) {
    return std::isalnum(static_cast<unsigned char>(ch));
}

bool IsAlNumLL(char ch) {
    return IsAlNum(ch) || ch == '_';
}

std::string ReadString(std::istream& input) {
    std::string line;
    // read the stream character by character, to the end of the line or to an unshielded closing quotation mark
    char first = input.get(); // first quote : ' or "
    char c;
    while (input.get(c)) {
        if (c == '\\') {
            char next;
            input.get(next);
            if (next == '\"') {
                line += '\"';
            } else if (next == '\'') {
                line += '\'';
            } else if (next == 'n') {
                line += '\n';
            } else if (next == 't') {
                line += '\t';
            }
        } else {
            if (c == first) {
                break;
            }
            line += c;
        }
    }
    // If the last character was not a closing hex, then the string is not composed correctly
    if (c != first) {
        throw std::runtime_error("Failed to parse string : "s + line);
    }
    return line;
}

std::string ReadName(std::istream &input) {
    std::string line;
    char c;
    while (input.get(c)) {
        if (IsAlNumLL(c)) {
            line += c;
        } else {
            input.putback(c);
            break;
        }
    }
    return line;
}

size_t CountSpaces(std::istream &input) {
    size_t result = 0;
    char c;
    while (input.get(c)) {
        if (c == ' ') {
            ++result;
        } else {
            input.putback(c);
            break;
        }
    }
    return result;
}

std::string ReadLine(std::istream &input) {
    std::string s;
    getline(input, s);
    return s;
}

int ReadNumber(std::istream &input) {
    std::string num;
    char c;
    while(input.get(c)) {
        if (IsNum(c)) {
            num += c;
        } else {
            input.putback(c);
            break;
        }
    }
    if (num.empty()) {
        return 0;
    }
    return std::stoi(num);
}

} // namespace util
