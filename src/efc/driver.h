#ifndef DRIVER_H
#define DRIVER_H
#include "gensrc/parser.hpp"
#include <string>

#define YY_DECL yy::Parser::symbol_type yylex(Driver& driver)
YY_DECL;

/* Drives the Scanner and the Parser */
class Driver {
public:
  Driver(const std::string& fileName);
  virtual ~Driver();
  
  // referenced by Parser
  // --------------------------------------------------
  void error(const yy::location& loc, const std::string& msg);
  void error(const std::string& msg);

  // referenced by both Scanner and Parser
  // --------------------------------------------------
  /** The name of the file being parsed */
  std::string m_fileName;
};
  
#endif
