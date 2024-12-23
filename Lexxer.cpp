#include "Lexxer.h"
#include <boost/locale/encoding_utf.hpp>
#include <stdexcept>

namespace
{
    constexpr auto eof = std::numeric_limits<uint32_t>::max();

    bool IsWhite(CodePoint codePoint) {
        return std::isspace(codePoint);
    }

    bool IsLineBreak(CodePoint codePoint) {
        return codePoint == 0;
    }

    bool IsModifier(CodePoint codePoint) {
        return codePoint == 0;
    }

    bool IsZeroWidth(CodePoint codePoint) {
        return codePoint == 0;
    }
}

auto Token::IntegerValue() -> std::optional<integer_t> const
{
    if (mType != TokenType::Literal)
        return std::nullopt;
    integer_t value = 0;
    for (const auto digit : mText) {
        value *= 10;
        value += digit - '0';
    }
    // to-do: throw on overflow
    return value;
}

Lexxer::Lexxer(std::string_view content)
: mContent{content}
, miCurrentCodePoint{}
, miNextCodePoint{content.begin()}
, mi2ndNextCodePoint{miNextCodePoint}
, mNextCodePoint{content.empty() ? eof : boost::locale::utf::utf_traits<char>::decode(mi2ndNextCodePoint, mContent.end())}
, mNextToken{PopTokenInner()}
{
}

auto Lexxer::PopCodePoint() -> CodePoint {
    using namespace boost::locale::utf;
    
    const auto returnValue = mNextCodePoint;
    if (returnValue == illegal || returnValue == incomplete)
        throw std::logic_error{"Invalid UTF-8 sequence or unexpected end of file encountered"}; // At miNextCodePoint

    miCurrentCodePoint = miNextCodePoint;
    miNextCodePoint = mi2ndNextCodePoint;
    mNextCodePoint = mi2ndNextCodePoint == mContent.end() ? eof : utf_traits<char>::decode(mi2ndNextCodePoint, mContent.end());

    return returnValue;
}

auto Lexxer::PeekCodePoint() -> CodePoint {
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
        if (IsWhite(codePoint)) {
            EatWhite();
            continue;
        }
        if (codePoint >= '0' && codePoint <= '9')
            return ReadIntegerLiteralToken();
        if (codePoint == '=' || codePoint == '(' || codePoint == ')')
            return Token{TokenType::Symbol, miCurrentCodePoint, miNextCodePoint};
        if (codePoint == eof)
            return Token{TokenType::EndOfFile, miNextCodePoint, miNextCodePoint};
        throw std::logic_error{"Unexpected character encountered"};
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
    return Token{TokenType::Literal, begin, miNextCodePoint};
}

auto Lexxer::EatWhite() -> void {
    while (true) {
        const auto codePoint = PeekCodePoint();
        if (IsWhite(codePoint)) {
            PopCodePoint();
            continue;
        }
        return;
    }
}
