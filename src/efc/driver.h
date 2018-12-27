#pragma once
#include "declutils.h"
#include <string>
#include <memory>

#include "astforwards.h"
namespace yy {
  class Parser; 
  class location; 
}
class ErrorHandler;
class Env;
class ParserExt;
class SemanticAnalizer;
class IrGen;
class ExecutionEngineApater;
class Scanner;
class TokenFilter;

/* Hosts the scanner, parser, semantic analyizer, IR builder etc. and drives
those. The scanner has no own class; it is driven by it's global function
yylex.*/
class Driver {
public:
  Driver(const std::string& fileName, std::basic_ostream<char>* ostream = nullptr);
  virtual ~Driver();
  
  Scanner& scanner();
  ErrorHandler& errorHandler();

  void compile();
  /** Guarantess to return non-null */
  std::unique_ptr<AstNode> scanAndParse();
  void doSemanticAnalysis(AstNode& ast);
  void generateIr(AstNode& ast);
  int jitExecMain();

  // referenced by scanner, parser etc.
  // --------------------------------------------------
  void warning(const yy::location& loc, const std::string& msg);
  void error(const yy::location& loc, const std::string& msg);
  void exitInternError(const yy::location& loc, const std::string& msg);
  void exitInternError(const std::string& msg);

  std::string& fileName() { return m_fileName; }
  bool gotError() const { return m_gotError; }
  bool gotWarning() const { return m_gotWarning; }

private:
  friend class TestingDriver;

  NEITHER_COPY_NOR_MOVEABLE(Driver);

  /** Guarantess to return non-null */
  std::unique_ptr<AstObject> addImplicitMain(std::unique_ptr<AstNode> ast);
  std::basic_ostream<char>& print(const yy::location& loc);

  /** The name of the file being parsed */
  std::string m_fileName;
  /** Guaranteed to be non-null */
  std::unique_ptr<ErrorHandler> m_errorHandler;
  /** Guaranteed to be non-null */
  std::unique_ptr<Env> m_env;
  bool m_gotError;
  bool m_gotWarning;
  std::basic_ostream<char>& m_ostream;
  /** Guaranteed to be non-nullptr */
  std::unique_ptr<Scanner> m_scanner;
  /** Guaranteed to be non-nullptr */
  std::unique_ptr<TokenFilter> m_tokenFilter;
  std::unique_ptr<AstNode> m_astRootFromParser;
  /** Guaranteed to be non-nullptr */
  std::unique_ptr<ParserExt> m_parserExt;
  /** Guaranteed to be non-nullptr */
  std::unique_ptr<yy::Parser> m_parser;
  /** Guaranteed to be non-nullptr */
  std::unique_ptr<IrGen> m_irGen;
  /** Guaranteed to be non-nullptr */
  std::unique_ptr<SemanticAnalizer> m_semanticAnalizer;
  std::unique_ptr<ExecutionEngineApater> m_executionEngine;
};
