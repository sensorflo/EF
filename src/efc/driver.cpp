#include "driver.h"
#include "env.h"
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

Driver::Driver(const std::string& fileName) :
  m_fileName(fileName),
  m_gotError(false),
  m_gotWarning(false),
  m_astRoot(NULL),
  m_parserExt(m_env),
  m_parser(new Parser(*this, m_parserExt, m_astRoot)) {
  // Ctor/Dtor must RAII yyin and m_parser

  if (!m_parser) { cerr << "Out of memory"; exit(1); }
  
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

  // generate IR code and JIT execute it
  // It's assumed that the module wants an implicit main method, thus
  // a cast to AstValue is required
  IrBuilderAst irBuilderAst(m_env);
  cout << irBuilderAst.buildAndRunModule(*dynamic_cast<AstValue*>(astAfterParse)) << "\n";
}

int Driver::scannAndParse(AstNode*& ast) {
  // the parser internally drives the scanner
  int ret = m_parser->parse(); // as a side-effect, sets m_astRoot
  ast = m_astRoot;
  return ret;
}

static std::basic_ostream<char>& print(std::basic_ostream<char>& os,
  const location& loc) {
  return os << *loc.begin.filename << ":" <<
    loc.begin.line << ":" << 
    loc.begin.column << ": ";
}

void Driver::warning(const location& loc, const string& msg) {
  print(cerr,loc) << "warning: " << msg << "\n";
  m_gotWarning = true;
}

void Driver::error(const location& loc, const string& msg) {
  print(cerr,loc) << "error: " << msg << "\n";
  m_gotError = true;
}

void Driver::exitInternError(const location& loc, const string& msg) {
  print(cerr,loc) << "internal error: " << msg << "\n";
  exit(1);
}

void Driver::exitInternError(const string& msg) {
  cerr << "internal error: " << msg << "\n";
  exit(1);
}
