#pragma once
#include <string>

/** Represents an entity in the target program. Multiple AstNode s may refer
to the same Entity. */
class Entity {
public:
  virtual std::string toStr() const =0;
};
