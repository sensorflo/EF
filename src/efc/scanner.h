#pragma once
#include "gensrc/parser.hpp"
#include <memory>

class Driver;

/** \internal See also yylex and yylex_raw */
class Scanner {
public:
  Scanner(Driver& driver);

  /** Removes and returns front token */
  yy::Parser::symbol_type pop();

private:
  Driver& m_driver;
};


/** Defines signature for the lex familiy of functions. This signature is
reduntaly defined by YYLEX_SIGNATURE and by %lex-param of bison's .yy input
file */
#define YYLEX_SIGNATURE(fun_name) \
  yy::Parser::symbol_type fun_name(Driver& driver)

/* Same as driver.scanner().pop(). Required only because the generated bison
parser expects a free function called 'yylex'. */
YYLEX_SIGNATURE(yylex);
