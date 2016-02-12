#pragma once
#include "declutils.h"
#include <string>
#include <ostream>

class EnvNode {
public:
  virtual ~EnvNode() = default;

  // -- pure virtual functions
  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>& os) const = 0;

  // -- misc
  std::string toStr() const;

protected:
  EnvNode() = default;

private:
  NEITHER_COPY_NOR_MOVEABLE(EnvNode);
};

std::basic_ostream<char>& operator<<(std::basic_ostream<char>& os,
  const EnvNode& ot);
