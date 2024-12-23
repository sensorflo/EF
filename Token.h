#pragma once
#include "ExchangeTypes.h"

#include <optional>
#include <string>

enum class TokenType {
    IntegerLiteral,
    Identifier,
    Symbol,
    EndOfFile
};

class Token {
public:
    Token(TokenType, std::string_view::iterator, std::string_view::iterator);
    virtual ~Token() = default;

    auto Text() const -> std::string_view;
    auto Type() const -> TokenType;
    auto IntegerValue() const -> std::optional<Integer>;

protected:
    TokenType mType;
    std::string_view mText;
};
