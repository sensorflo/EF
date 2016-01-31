#pragma once
#include <iostream>
#include <cassert>

enum class Access { eUndefined, eRead, eWrite, eTakeAddress, eIgnoreValueAndAddr };

inline std::basic_ostream<char>& operator<<(std::basic_ostream<char>& os,
  Access access) {
  switch (access) {
  case Access::eUndefined: return os << "eUndefined";
  case Access::eRead: return os << "eRead";
  case Access::eWrite: return os << "eWrite";
  case Access::eTakeAddress: return os << "eTakeAddress";
  case Access::eIgnoreValueAndAddr: return os << "eIgnoreValueAndAddr";
  default: assert(false); return os;
  }
}
