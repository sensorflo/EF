#pragma once

#include "gensrc/parser.hpp"

class TokenStream {
public:
  virtual ~TokenStream() {};
  /** Removes and returns front token */
  virtual yy::Parser::symbol_type pop() =0;
};
