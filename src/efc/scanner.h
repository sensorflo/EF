#pragma once
#include "tokenstream.h"
#include <memory>

class Driver;

/** \internal See also yylex and yylex_raw */
class Scanner : public TokenStream {
public:
  Scanner(Driver& driver);

  yy::Parser::symbol_type& front() override;
  yy::Parser::symbol_type pop() override;

private:
  void shift();
  void ensureThereIsAFront();
  Driver& m_driver;
  std::unique_ptr<yy::Parser::symbol_type> m_front;
};

