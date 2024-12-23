#pragma once
#include "Text.h"
#include "Token.h"

class Lexxer {
public:
    explicit Lexxer(std::string_view content);

    auto PopToken() -> Token;
    auto PeekToken() -> const Token&;

private:
    auto PopTokenInner() -> Token;
    auto PopCodePoint() -> CodePoint;
    auto PeekCodePoint() -> CodePoint;

    auto ReadIntegerLiteral() -> Token;
    auto ReadIdentifier() -> Token;
    auto EatWhite() -> void;

private:
    std::string_view mContent;
    
    std::string_view::iterator miCurrentCodePoint; // I.e. the last popped.
    std::string_view::iterator miNextCodePoint; // What pop will return.
    std::string_view::iterator mi2ndNextCodePoint;
    CodePoint mNextCodePoint; // What pop will return.

    Token mNextToken; // What pop will return.
};
