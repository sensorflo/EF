#ifndef DRIVER_H
#define DRIVER_H
#include "parserext.h"
#include "env.h"
#include "errorhandler.h"
#include "gensrc/parser.hpp"
#include <string>

namespace yy {
  class Parser; 
}
class AstNode;

#define YY_DECL yy::Parser::symbol_type yylex(Driver& driver)
YY_DECL;

/* Hosts the scanner, parser, semantic analyizer, IR builder etc. and drives
those. The scanner has no own class; it is driven by it's global function
yylex.*/
class Driver {
public:
  Driver(const std::string& fileName, std::basic_ostream<char>* ostream = NULL);
  virtual ~Driver();
  
  void buildAndRunModule();
  int scannAndParse(AstNode*& ast);

  // referenced by scanner, parser etc.
  // --------------------------------------------------
  void warning(const yy::location& loc, const std::string& msg);
  void error(const yy::location& loc, const std::string& msg);
  void exitInternError(const yy::location& loc, const std::string& msg);
  void exitInternError(const std::string& msg);

  AstNode*& astRoot() { return m_astRoot; }
  std::string& fileName() { return m_fileName; }
  bool gotError() const { return m_gotError; }
  bool gotWarning() const { return m_gotWarning; }

private:
  std::basic_ostream<char>& print(const yy::location& loc);
  
  /** The name of the file being parsed */
  std::string m_fileName;
  ErrorHandler m_errorHandler;
  Env m_env;
  bool m_gotError;
  bool m_gotWarning;
  /** We're _not_ the owner */
  std::basic_ostream<char>& m_ostream;
  AstNode* m_astRoot;
  ParserExt m_parserExt;
  /** We're the owner. Guaranteed to be non-NULL */
  yy::Parser* m_parser;
};
  
#endif
