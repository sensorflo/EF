#include "envnode.h"
#include <sstream>

std::string EnvNode::toStr() const {
  std::ostringstream ss;
  printTo(ss);
  return ss.str();
}

std::basic_ostream<char>& operator<<(std::basic_ostream<char>& os,
  const EnvNode& ot) {
  return ot.printTo(os);
}
