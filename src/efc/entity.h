#pragma once
#include "declutils.h"
#include <string>
#include <sstream>
#include <ostream>

/** Represents an entity in the target program. Multiple AstNode s may refer
to the same Entity. */
class Entity {
public:
  virtual ~Entity() = default;

  std::string toStr() const {
    std::ostringstream ss;
    printTo(ss);
    return ss.str();
  }
  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>& os) const = 0;

protected:
  Entity() = default;
};

inline std::basic_ostream<char>& operator<<(std::basic_ostream<char>& os,
  const Entity& ot) {
  return ot.printTo(os);
}
