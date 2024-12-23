#pragma once
#include <memory>
#include <optional>
#include <string>
#include <boost/locale/utf.hpp>

using CodePoint = boost::locale::utf::code_point;
using integer_t = std::int64_t;

enum class TokenType {
    Literal,
    Identifier,
    Symbol,
    EndOfFile
};

class Token {
public:
    Token(TokenType, std::string_view::iterator, std::string_view::iterator);
    virtual ~Token() = default;

    auto Text() -> std::string_view const;
    auto Type() -> TokenType const;
    auto IntegerValue() -> std::optional<integer_t> const;

protected:
    TokenType mType;
    std::string_view mText;
};

class Lexxer {
public:
    explicit Lexxer(std::string_view content);

    auto PopToken() -> Token;
    auto PeekToken() -> const Token&;

private:
    auto PopTokenInner() -> Token;
    auto PopCodePoint() -> CodePoint;
    auto PeekCodePoint() -> CodePoint;

    auto ReadIntegerLiteralToken() -> Token;
    auto EatWhite() -> void;

private:
    std::string_view mContent;
    
    std::string_view::iterator miCurrentCodePoint; // I.e. the last popped.
    std::string_view::iterator miNextCodePoint; // What pop will return.
    std::string_view::iterator mi2ndNextCodePoint;
    CodePoint mNextCodePoint; // What pop will return.

    Token mNextToken; // What pop will return.
};
