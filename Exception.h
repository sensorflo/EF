#pragma once

#include <stdexcept>
#include <string>

class Token;

class Exception : public std::runtime_error
{
public:
    explicit Exception(std::string_view msg);
    Exception(std::string_view msg, std::string_view::iterator pos);
    Exception(std::string_view msg, const Token& token); // Just convenience.

private:
    std::string_view::iterator mPos;
};