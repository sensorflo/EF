#pragma once
#include <iostream>
#include <cassert>

/** eIgnore: neither of eRead, eWrite or eTakeAddress, e.g. the lhs of the
sequence operator, or mayebe in a future EF version the argument to the
hypothetical typeOf() 'function'. */
enum class Access { eRead, eWrite, eTakeAddress, eIgnore };

inline std::basic_ostream<char>& operator<<(std::basic_ostream<char>& os,
  Access access) {
  switch (access) {
  case Access::eRead: return os << "eRead";
  case Access::eWrite: return os << "eWrite";
  case Access::eTakeAddress: return os << "eTakeAddress";
  case Access::eIgnore: return os << "eIgnore";
  default: assert(false); return os;
  }
}
