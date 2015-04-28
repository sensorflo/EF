#ifndef DRIVER_H
#define DRIVER_H
#include <string>
#include <memory>

namespace yy {
  class Parser; 
  class location; 
}
class AstNode;
class ErrorHandler;
class Env;
class ParserExt;
class SemanticAnalizer;
class IrGen;
class ExecutionEngineApater;

/* Hosts the scanner, parser, semantic analyizer, IR builder etc. and drives
those. The scanner has no own class; it is driven by it's global function
yylex.*/
class Driver {
public:
  Driver(const std::string& fileName, std::basic_ostream<char>* ostream = NULL);
  virtual ~Driver();
  
  void compile();
  int scannAndParse(AstNode*& ast);
  void doSemanticAnalysis(AstNode& ast);
  void generateIr(AstNode& ast);
  int jitExecMain();

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
  friend class TestingDriver;

  std::basic_ostream<char>& print(const yy::location& loc);
  
  /** The name of the file being parsed */
  std::string m_fileName;
  ErrorHandler& m_errorHandler;
  Env& m_env;
  bool m_gotError;
  bool m_gotWarning;
  /** We're _not_ the owner */
  std::basic_ostream<char>& m_ostream;
  AstNode* m_astRoot;
  ParserExt& m_parserExt;
  /** We're the owner. Guaranteed to be non-NULL */
  yy::Parser* m_parser;
  IrGen& m_irGen;
  SemanticAnalizer& m_semanticAnalizer;
  std::unique_ptr<ExecutionEngineApater> m_executionEngine;
};
  
#endif
