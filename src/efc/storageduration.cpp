#include "storageduration.h"

std::basic_ostream<char>& operator<<(std::basic_ostream<char>& os,
  StorageDuration sd) {
  switch (sd) {
  case StorageDuration::eYetUndefined: return os << "yetundefined";
  case StorageDuration::eUnknown: return os << "unknown";
  case StorageDuration::eLocal: return os;
  case StorageDuration::eStatic: return os << "static";
  case StorageDuration::eMember: return os << "member";
  }
}


