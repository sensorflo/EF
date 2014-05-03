#ifndef DRIVER_H
#define DRIVER_H
#include "gensrc/parser.hpp"
#include <string>

namespace yy {
  class Parser; 
}
class AstNode;

#define YY_DECL yy::Parser::symbol_type yylex(Driver& driver)
YY_DECL;

/* Hosts the scanner and parser, and drives the parser which in turn drives
the scanner. The scanner has no own class; it is driven by it's global
function yylex.*/
class Driver {
public:
  Driver(const std::string& fileName);
  virtual ~Driver();
  
  int parse(AstNode*& astRoot);

  // referenced by both Scanner and Parser
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
  /** The name of the file being parsed */
  std::string m_fileName;
  bool m_gotError;
  bool m_gotWarning;
  AstNode* m_astRoot;
  /** We're the owner. Guaranteed to be non-NULL */
  yy::Parser* m_parser;
};
  
#endif
