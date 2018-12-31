#pragma once
#include "declutils.h"
#include "parser.h"

class TokenStream {
public:
  virtual ~TokenStream() = default;

  /** Removes and returns front token */
  virtual Parser::symbol_type pop() = 0;

protected:
  TokenStream() = default;

private:
  NEITHER_COPY_NOR_MOVEABLE(TokenStream);
};
