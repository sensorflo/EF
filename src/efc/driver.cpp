#include "driver.h"
#include "env.h"
#include "semanticanalizer.h"
#include "irbuilderast.h"
#include "gensrc/parser.hpp"
#include <stdio.h>
#include <errno.h>
#include <string.h>
using namespace std;
using namespace yy;

extern FILE* yyin;
extern void yyrestart(FILE*);
extern void yyinitializeParserLoc(string* filename);

/** \param osstream caller keeps ownership */
Driver::Driver(const string& fileName, std::basic_ostream<char>* ostream) :
  m_fileName(fileName),
  m_gotError(false),
  m_gotWarning(false),
  m_ostream(ostream ? *ostream : cerr),
  m_astRoot(NULL),
  m_parserExt(m_env, m_errorHandler),
  m_parser(new Parser(*this, m_parserExt, m_astRoot)) {
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
  delete m_parser;
}

void Driver::buildAndRunModule() {
  // scann and parse
  AstNode* astAfterParse = NULL;
  int retParse = scannAndParse(astAfterParse);
  if (retParse) {
    throw runtime_error::runtime_error("parse failed");
  }

  // semantic analysis and transformations
  SemanticAnalizer semanticAnalizer(m_errorHandler);
  AstNode* astAfterSa = semanticAnalizer.transform(*astAfterParse);  

  // generate IR code and JIT execute it
  // It's assumed that the module wants an implicit main method, thus
  // a cast to AstValue is required
  IrBuilderAst irBuilderAst(m_env, m_errorHandler);
  cout << irBuilderAst.buildAndRunModule(*dynamic_cast<AstValue*>(astAfterSa)) << "\n";
}

int Driver::scannAndParse(AstNode*& ast) {
  // the parser internally drives the scanner
  int ret = m_parser->parse(); // as a side-effect, sets m_astRoot
  ast = m_astRoot;
  return ret;
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
