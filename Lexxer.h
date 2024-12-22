#pragma once
#include <memory>
#include <string>
#include <boost/locale/utf.hpp>

using code_point = boost::locale::utf::code_point;

class Token
{
public:
    std::string_view Range() const;    
protected:    
    std::string_view mRange;
};

using integer_t = std::int64_t;

class IntegerLiteralToken : public Token
{
public:
    IntegerLiteralToken(integer_t value, std::string_view position);
    integer_t Value();
};


class OperatorToken : public Token
{
public:

};

class Lexxer
{
public:
    explicit Lexxer(std::string_view path, std::string_view content);

    std::unique_ptr<Token> NextToken();

private:
    code_point NextGraphemeClusterFirstCodePoint();
    void PutBack();
    std::unique_ptr<Token> ReadIntegerLiteralToken(code_point);
    std::unique_ptr<Token> ReadOperatorToken(code_point);
    std::string MakeErrorMsg(std::string_view msgCore);

private:
    std::string_view mPath;
    std::string_view mContent;
    std::string_view::iterator miNextToken;
    std::string_view::iterator miCurrentToken; ///< Start of current token
    std::string_view::iterator miPreviousToken;
    size_t mLine = 0;
    size_t mColumn = 0;
};
