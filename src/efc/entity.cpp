#include "entity.h"
#include <sstream>

std::string Entity::toStr() const {
  std::ostringstream ss;
  printTo(ss);
  return ss.str();
}

std::basic_ostream<char>& operator<<(std::basic_ostream<char>& os,
  const Entity& ot) {
  return ot.printTo(os);
}
