#pragma once
#include "tokenstream.h"
#include <memory>

class Driver;

/** \internal See also yylex and yylex_raw */
class Scanner : public TokenStream {
public:
  Scanner(Driver& driver);

  yy::Parser::symbol_type pop() override;

private:
  Driver& m_driver;
};
