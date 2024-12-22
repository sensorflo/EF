#include "Lexxer.h"
#include <boost/locale/encoding_utf.hpp>
#include <stdexcept>

namespace
{
    bool IsWhite(code_point codePoint) {
        return codePoint == 0;
    }

    bool IsLineBreak(code_point codePoint) {
        return codePoint == 0;
    }

    bool IsModifier(code_point codePoint) {
        return codePoint == 0;
    }

    bool IsZeroWidth(code_point codePoint) {
        return codePoint == 0;
    }
}

std::unique_ptr<Token> Lexxer::NextToken() {
    while (true)
    {
        if (miNextToken == mContent.end())
            return nullptr;

        const auto codePoint = NextGraphemeClusterFirstCodePoint();

        if (IsWhite(codePoint))
            continue;        
        if (codePoint >= '0' && codePoint <= '9')
            return ReadIntegerLiteralToken(codePoint);
        if (codePoint == '+')
            return ReadOperatorToken(codePoint);
    }
}

std::unique_ptr<Token> Lexxer::ReadIntegerLiteralToken(code_point codePoint)
{
    integer_t value = 0;
    while (true)
    {
        value += codePoint - '0';
        codePoint = NextGraphemeClusterFirstCodePoint();
        if (!(codePoint >= '0' && codePoint <= '9'))
        {
            PutBack();
            break;
        }
        value *= 10;
    }
    return std::make_unique<IntegerLiteralToken>(value, std::string_view{miCurrentToken, miNextToken});
}

std::unique_ptr<Token> Lexxer::ReadOperatorToken(code_point )
{
    return nullptr;
}

code_point Lexxer::NextGraphemeClusterFirstCodePoint()
{
    using namespace boost::locale::utf;
    code_point firstCodePoint = 0;
    bool isFirstIter = true;
    miPreviousToken = miCurrentToken;
    miCurrentToken = miNextToken;
    do 
    {
        const auto codePoint = utf_traits<char>::decode(miNextToken, mContent.end());
        if (codePoint == illegal)
            throw std::logic_error{MakeErrorMsg("Invalid UTF8 character encountered")};
        if (isFirstIter)
            firstCodePoint = codePoint, isFirstIter = false;
        if (IsModifier(codePoint) || IsZeroWidth(codePoint))
            continue;
        break;
    } while(true);

    if (IsLineBreak(firstCodePoint))
        mColumn = 0, mLine++;
    else
        mColumn++;

    return firstCodePoint;
}

void Lexxer::PutBack()
{
    miNextToken = miCurrentToken;
    miCurrentToken = miPreviousToken;
}

std::string Lexxer::MakeErrorMsg(std::string_view msgCore)
{
    std::string msg{mPath};
    msg += ':';
    msg += std::to_string(mLine);
    msg += ':';
    msg += std::to_string(mColumn);
    msg += ':';
    msg += ' ';
    msg += msgCore;
    return msg;
}