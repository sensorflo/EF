#pragma once
#include <string>

/** Fully qualified name provider */
class FQNameProvider {
public:
  virtual const std::string& fqName() const =0;
};
