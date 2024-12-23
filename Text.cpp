#include "Text.h"

#include "Exception.h"

#include <boost/locale/utf.hpp>

#include <locale>
#include <type_traits>

static_assert(boost::locale::utf::illegal == CodePointIllegal);
static_assert(boost::locale::utf::incomplete == CodePointIncomplete);

auto IsAsciiWhite(const CodePoint codePoint) -> bool {
    return
        codePoint == 0x20 /*space*/ ||
        codePoint == 0x09 /*tab*/ ||
        codePoint == 0x0A /*line feed*/ ||
        codePoint == 0x0D /*carriage return*/ ||
        codePoint == 0x0C /*form feed*/;
}

auto IsAsciiDigit(const CodePoint codePoint) -> bool {
    return codePoint >= '0' && codePoint <= '9';
}

auto IsLetter(const CodePoint codePoint) -> bool {
    const auto ucs2CodePoint = static_cast<wchar_t>(codePoint);
    if (ucs2CodePoint != codePoint)
        throw Exception{"Non-UCS2 characters not yet fully supported."};
    return std::isalpha(ucs2CodePoint);
}

auto ReadCodePoint(std::string_view::iterator& pos, const std::string_view::iterator end) -> CodePoint {
    if (pos == end)
        return CodePointEof;
    return boost::locale::utf::utf_traits<char>::decode(pos, end);
}
