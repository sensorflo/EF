#pragma once
#include <iostream>
#include <cassert>

/** Access to an an Object. */
enum class Access {
  /** For Access variables which's proper value is not yet defined. */
  eYetUndefined,
  /** read the object's value, that is read from the underlying memory */
  eRead,
  /** analogous to eRead. Initialization does _not_ count as write */
  eWrite,
  eTakeAddress,
  eIgnoreValueAndAddr
};

inline std::basic_ostream<char>& operator<<(
  std::basic_ostream<char>& os, Access access) {
  switch (access) {
  case Access::eYetUndefined: return os << "eYetUndefined";
  case Access::eRead: return os << "eRead";
  case Access::eWrite: return os << "eWrite";
  case Access::eTakeAddress: return os << "eTakeAddress";
  case Access::eIgnoreValueAndAddr: return os << "eIgnoreValueAndAddr";
  default: assert(false); return os;
  }
}
