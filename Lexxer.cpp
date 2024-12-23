#include "Lexxer.h"

#include "Exception.h"

#include <stdexcept>

Lexxer::Lexxer(std::string_view content)
: mContent{content}
, miCurrentCodePoint{}
, miNextCodePoint{content.begin()}
, mi2ndNextCodePoint{miNextCodePoint}
, mNextCodePoint{ReadCodePoint(mi2ndNextCodePoint, mContent.end())}
, mNextToken{PopTokenInner()}
{
}

auto Lexxer::PopCodePoint() -> CodePoint {
    const auto returnValue = PeekCodePoint();

    miCurrentCodePoint = miNextCodePoint;
    miNextCodePoint = mi2ndNextCodePoint;
    mNextCodePoint = ReadCodePoint(mi2ndNextCodePoint, mContent.end());

    return returnValue;
}

auto Lexxer::PeekCodePoint() -> CodePoint {
    if (mNextCodePoint == CodePointIllegal || mNextCodePoint == CodePointIncomplete)
        throw Exception{"Invalid UTF-8 sequence or unexpected end of file encountered", miNextCodePoint};
    return mNextCodePoint;
}

auto Lexxer::PopToken() -> Token {
    auto returnValue = mNextToken;
    mNextToken = PopTokenInner();
    return returnValue;
}

auto Lexxer::PeekToken() -> const Token& {
    return mNextToken;
}

auto Lexxer::PopTokenInner() -> Token {
    while (true) {
        const auto codePoint = PopCodePoint();
        if (IsAsciiWhite(codePoint)) {
            EatWhite();
            continue;
        }
        if (codePoint >= '0' && codePoint <= '9')
            return ReadIntegerLiteralToken();
        if (codePoint == '=' || codePoint == '(' || codePoint == ')')
            return Token{TokenType::Symbol, miCurrentCodePoint, miNextCodePoint};
        if (codePoint == CodePointEof)
            return Token{TokenType::EndOfFile, miNextCodePoint, miNextCodePoint};
        throw Exception{"Unexpected character encountered", miCurrentCodePoint};
    }
}

auto Lexxer::ReadIntegerLiteralToken() -> Token {
    auto begin = miCurrentCodePoint;
    while (true) {
        const auto codePoint = PeekCodePoint();
        if (codePoint >= '0' && codePoint <= '9') {
            PopCodePoint();
            continue;
        }
        break;
    }
    return Token{TokenType::IntegerLiteral, begin, miNextCodePoint};
}

auto Lexxer::EatWhite() -> void {
    while (true) {
        const auto codePoint = PeekCodePoint();
        if (IsAsciiWhite(codePoint)) {
            PopCodePoint();
            continue;
        }
        return;
    }
}
