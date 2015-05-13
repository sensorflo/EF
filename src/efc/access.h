#pragma once
#include <iostream>
#include <cassert>

enum Access { eRead, eWrite, eTakeAddress, eIgnore };

inline std::basic_ostream<char>& operator<<(std::basic_ostream<char>& os,
  Access access) {
  switch (access) {
  case eRead: return os << "eRead";
  case eWrite: return os << "eWrite";
  case eTakeAddress: return os << "eTakeAddress";
  case eIgnore: return os << "eIgnore";
  default: assert(false); return os;
  }
}
