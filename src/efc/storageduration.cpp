#include "storageduration.h"

std::basic_ostream<char>& operator<<(std::basic_ostream<char>& os,
  StorageDuration sd) {
  switch (sd) {
  case StorageDuration::eUnknown: return os << "unknown_sd";
  case StorageDuration::eLocal: return os;
  case StorageDuration::eStatic: return os << "static";
  default: assert(false); return os;
  }
  return os;
}


