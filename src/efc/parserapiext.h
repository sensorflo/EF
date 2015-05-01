#pragma once
#include "gensrc/parser.hpp"
#include <vector>
#include <iostream>

/** Extends the public API of the class yy::Parser.

\internal The class yy::Parser is generated by Bison. The author of
ParserApiExt did not see a way to configure Bison such that the features
provided by ParserApiExt are directly provided by yy::Parser.

ParserExt is similar, but extends the private implementation details of
Parser. */
class ParserApiExt {
public:
  static const char* tokenName(yy::Parser::token_type t);

private:
  static void initTokenNames();

  /** Redundant copy of yy::Parser::yytname_.
  \internal The author did not knew how to access the private member
  yy::Parser::yytname_ to be able to print tokens. A redundant copy seemed the
  best among all the bad solutions since the tokens do not change that
  often */
  static std::vector<const char*> m_TokenNames;
  static std::vector<char> m_OneCharTokenNames;
};

namespace yy {
std::basic_ostream<char>& operator<<(std::basic_ostream<char>& os,
  yy::Parser::token_type t);
}
