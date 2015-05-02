#pragma once

#include "gensrc/parser.hpp"

class TokenStream {
public:
  virtual ~TokenStream() {};
  /** Returns reference to front token without removing it */
  virtual yy::Parser::symbol_type& front() =0;
  /** Removes and returns front token */
  virtual yy::Parser::symbol_type pop() =0;
};
