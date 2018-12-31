#pragma once

#include "astforwards.h"
#include "declutils.h"

#include <memory>
#include <string>

class Parser;
class Location;
class ErrorHandler;
class Env;
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
  Driver(std::string fileName, std::basic_ostream<char>* ostream = nullptr);
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
  void warning(const Location& loc, const std::string& msg);
  void error(const Location& loc, const std::string& msg);
  void exitInternError(const Location& loc, const std::string& msg);
  void exitInternError(const std::string& msg);

  std::string& fileName() { return m_fileName; }
  bool gotError() const { return m_gotError; }
  bool gotWarning() const { return m_gotWarning; }

private:
  friend class TestingDriver;

  NEITHER_COPY_NOR_MOVEABLE(Driver);

  std::basic_ostream<char>& print(const Location& loc);

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
  /** Guaranteed to be non-nullptr */
  std::unique_ptr<Parser> m_parser;
  /** Guaranteed to be non-nullptr */
  std::unique_ptr<IrGen> m_irGen;
  /** Guaranteed to be non-nullptr */
  std::unique_ptr<SemanticAnalizer> m_semanticAnalizer;
  std::unique_ptr<ExecutionEngineApater> m_executionEngine;
};
