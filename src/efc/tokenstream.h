#pragma once
#include "declutils.h"
#include "gensrc/parser.hpp"

class TokenStream {
public:
  virtual ~TokenStream() = default;

  /** Removes and returns front token */
  virtual yy::Parser::symbol_type pop() = 0;

protected:
  TokenStream() = default;

private:
  NEITHER_COPY_NOR_MOVEABLE(TokenStream);
};
