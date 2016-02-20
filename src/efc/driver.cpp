#include "driver.h"
#include "gensrc/parser.hpp"
#include "parserext.h"
#include "scanner.h"
#include "tokenfilter.h"
#include "ast.h"
#include "errorhandler.h"
#include "env.h"
#include "semanticanalizer.h"
#include "irgen.h"
#include "executionengineadapter.h"
#include "gensrc/parser.hpp"
#include "llvm/IR/Module.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdexcept>
using namespace std;
using namespace yy;

extern FILE* yyin;
extern void yyrestart(FILE*);
extern void yyinitializeParserLoc(string* filename);

/** \param osstream caller keeps ownership */
Driver::Driver(const string& fileName, std::basic_ostream<char>* ostream) :
  m_fileName(fileName),
  m_errorHandler(make_unique<ErrorHandler>()),
  m_env(make_unique<Env>()),
  m_gotError(false),
  m_gotWarning(false),
  m_ostream(ostream ? *ostream : cerr),
  m_scanner(make_unique<Scanner>(*this)),
  m_tokenFilter(make_unique<TokenFilter>(*m_scanner.get())),
  m_parserExt(make_unique<ParserExt>(*m_env, *m_errorHandler)),
  m_parser(make_unique<Parser>(*m_tokenFilter.get(), *this, *m_parserExt, m_astRootFromParser)),
  m_irGen(make_unique<IrGen>(*m_errorHandler)),
  m_semanticAnalizer(make_unique<SemanticAnalizer>(*m_env, *m_errorHandler)) {

  assert(m_errorHandler);
  assert(m_env);
  assert(m_scanner);
  assert(m_tokenFilter);
  assert(m_parser);
  assert(m_parserExt);
  assert(m_irGen);
  assert(m_semanticAnalizer);

  // Ctor/Dtor must RAII yyin and m_parser

  if (m_fileName.empty() || m_fileName == "-") {
    yyin = stdin;
  } else if (!(yyin = fopen(m_fileName.c_str(), "r"))) {
    exitInternError("cannot open " + m_fileName + ": " + strerror(errno));
  }
  yyinitializeParserLoc(&m_fileName);
  yyrestart(yyin);
}

Driver::~Driver() {
  fclose(yyin);
}

Scanner& Driver::scanner() {
  return *m_scanner;
}

ErrorHandler& Driver::errorHandler() {
  return *m_errorHandler;
}

/** Compile = scann & parse & do semantic analysis & generate IR. */
void Driver::compile() {
  try {
    auto astAfterParse = scanAndParse();

    // It's currently implied that the module wants an implicit main method
    const auto astAfterImplicitMain = addImplicitMain(move(astAfterParse));

    doSemanticAnalysis(*astAfterImplicitMain);

    generateIr(*astAfterImplicitMain);
  }
  catch (BuildError&) {
    // nop -- BuildError exception is handled below by printing any errors
  }
  if (!m_errorHandler->errors().empty()) {
    m_ostream << "Build error(s): " << *m_errorHandler << "\n";
  }
}

unique_ptr<AstNode> Driver::scanAndParse() {
  // the parser internally drives the scanner
  const auto errorCode = m_parser->parse(); // as a side-effect, sets m_astRootFromParser
  if (errorCode) {
    throw runtime_error("parse failed");
  }
  assert(m_astRootFromParser);
  return move(m_astRootFromParser);
}

unique_ptr<AstObject> Driver::addImplicitMain(unique_ptr<AstNode> ast) {
  assert(ast);
  auto astAfterParseAsObject =
    unique_ptr<AstObject>(
      dynamic_cast<AstObject*>(ast.release()));

  auto astAfterImplicitMain =
    unique_ptr<AstObject>(
      m_parserExt->mkMainFunDef(astAfterParseAsObject.release()));
  assert(astAfterImplicitMain);
  return astAfterImplicitMain;
}

void Driver::doSemanticAnalysis(AstNode& ast) {
  m_semanticAnalizer->analyze(ast);
}

void Driver::generateIr(AstNode& ast) {
  auto module = m_irGen->genIr(ast);
  m_executionEngine = std::make_unique<ExecutionEngineApater>(move(module));
}

int Driver::jitExecMain() {
  assert(m_executionEngine);
  return m_executionEngine->jitExecFunction(".main");
}

basic_ostream<char>& Driver::print(const location& loc) {
  return m_ostream << *loc.begin.filename << ":" <<
    loc.begin.line << ":" << 
    loc.begin.column << ": ";
}

void Driver::warning(const location& loc, const string& msg) {
  print(loc) << "warning: " << msg << "\n";
  m_gotWarning = true;
}

void Driver::error(const location& loc, const string& msg) {
  print(loc) << "error: " << msg << "\n";
  m_gotError = true;
}

void Driver::exitInternError(const location& loc, const string& msg) {
  print(loc) << "internal error: " << msg << "\n";
  exit(1);
}

void Driver::exitInternError(const string& msg) {
  m_ostream << "internal error: " << msg << "\n";
  exit(1);
}
