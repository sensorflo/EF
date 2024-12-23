#include "Token.h"

#include "Exception.h"

auto Token::Text() const -> std::string_view {
    return mText;
}

auto Token::Type() const -> TokenType {
    return mType;
}

auto Token::IntegerValue() const -> std::optional<Integer> {
    if (mType != TokenType::IntegerLiteral)
        return std::nullopt;
    Integer value = 0;
    for (const auto digit : mText) {
        value *= 10;
        value += digit - '0';
    }
    // to-do: throw on overflow
    return value;
}
