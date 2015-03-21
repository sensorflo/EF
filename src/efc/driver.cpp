#include "driver.h"
#include "errorhandler.h"
#include "env.h"
#include "semanticanalizer.h"
#include "irgen.h"
#include "gensrc/parser.hpp"
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
  m_errorHandler(*new ErrorHandler),
  m_env(*new Env),
  m_gotError(false),
  m_gotWarning(false),
  m_ostream(ostream ? *ostream : cerr),
  m_astRoot(NULL),
  m_parserExt(*new ParserExt(m_env, m_errorHandler)),
  m_parser(new Parser(*this, m_parserExt, m_astRoot)),
  m_irGen(*new IrGen(m_errorHandler)),
  m_semanticAnalizer(*new SemanticAnalizer(m_env, m_errorHandler)) {

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
  delete &m_irGen;
  delete &m_semanticAnalizer;
  delete m_parser;
  delete &m_parserExt;
  delete &m_env;
  delete &m_errorHandler;
}

/** Compile = scann & parse & do semantic analysis & generate IR. */
void Driver::compile() {
  try {
    AstNode* astAfterParse = NULL;
    int retParse = scannAndParse(astAfterParse);
    if (retParse) {
      throw runtime_error("parse failed");
    }
    assert(astAfterParse);
    doSemanticAnalysis(*astAfterParse);
    generateIr(*astAfterParse);
  }
  catch (BuildError&) {
    // nop -- BuildError exception is handled below by printing any errors
  }
  if (!m_errorHandler.errors().empty()) {
    m_ostream << "Build error(s): " << m_errorHandler << "\n";
  }
}

int Driver::scannAndParse(AstNode*& ast) {
  // the parser internally drives the scanner
  int ret = m_parser->parse(); // as a side-effect, sets m_astRoot
  ast = m_astRoot;
  return ret;
}

void Driver::doSemanticAnalysis(AstNode& ast) {
  m_semanticAnalizer.analyze(ast);
}

void Driver::generateIr(AstNode& ast) {
  // It's assumed that the module wants an implicit main method, thus
  // a cast to AstValue is required
  m_irGen.genIr( *m_parserExt.mkMainFunDef( &dynamic_cast<AstValue&>(ast)));
}

int Driver::jitExecMain() {
  return m_irGen.jitExecFunction("main");
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
