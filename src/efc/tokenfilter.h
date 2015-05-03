#pragma once
#include "tokenstream.h"
#include "gensrc/parser.hpp"

class TokenFilterX {
public:
  TokenFilterX(TokenStream& input);
  void foo();

  yy::Parser::symbol_type& A();
  yy::Parser::symbol_type B();

private:
  TokenStream& m_input;
};
