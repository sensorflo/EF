#include "storageduration.h"

std::basic_ostream<char>& operator<<(
  std::basic_ostream<char>& os, StorageDuration sd) {
  return os << toString(sd);
}

std::string toString(StorageDuration sd) {
  switch (sd) {
  case StorageDuration::eYetUndefined: return "yetundefined";
  case StorageDuration::eUnknown: return "unknown";
  case StorageDuration::eLocal: return "";
  case StorageDuration::eStatic: return "static";
  case StorageDuration::eMember: return "member";
  }
}
