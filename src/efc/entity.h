#pragma once
#include <string>

class Entity {
public:
  virtual std::string toStr() const =0;
};
