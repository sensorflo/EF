#pragma once
#include <string>

using CodePoint = uint32_t;

constexpr CodePoint CodePointIllegal = 0xFFFF'FFFFu;
constexpr CodePoint CodePointIncomplete = 0xFFFF'FFFEu;
constexpr CodePoint CodePointEof = 0xFFFF'FFFDu;

auto IsAsciiWhite(CodePoint codePoint) -> bool;

auto ReadCodePoint(std::string_view::iterator& pos, std::string_view::iterator end) -> CodePoint;
