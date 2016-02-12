#pragma once
#include "declutils.h"
#include <string>
#include <ostream>

/** Represents an entity in the target program. Multiple AstNode s may refer
to the same Entity. */
class Entity {
public:
  virtual ~Entity() = default;

  // -- pure virtual functions
  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>& os) const = 0;

  // -- misc
  std::string toStr() const;

protected:
  Entity() = default;

private:
  NEITHER_COPY_NOR_MOVEABLE(Entity);
};

std::basic_ostream<char>& operator<<(std::basic_ostream<char>& os,
  const Entity& ot);
