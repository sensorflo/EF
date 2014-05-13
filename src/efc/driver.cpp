#include "driver.h"
#include "gensrc/parser.hpp"
#include <stdio.h>
#include <errno.h>
#include <string.h>
using namespace std;
using namespace yy;

extern FILE* yyin;
extern void yyrestart(FILE*);

Driver::Driver(const std::string& fileName) :
  m_gotError(false),
  m_gotWarning(false),
  m_astRoot(NULL),
  m_parser(new Parser(*this, m_astRoot)) {
  // Ctor/Dtor must RAII yyin and m_parser

  if (!m_parser) { cerr << "Out of memory"; exit(1); }
  
  m_fileName = fileName;
  if (m_fileName.empty() || m_fileName == "-") {
    yyin = stdin;
  } else if (!(yyin = fopen(m_fileName.c_str(), "r"))) {
    exitInternError("cannot open " + m_fileName + ": " + strerror(errno));
  }
  yyrestart(yyin);
}

Driver::~Driver() {
  fclose(yyin);
  delete m_parser;
}

int Driver::parse(AstSeq*& astRoot) {
  int ret = m_parser->parse(); // as a side-effect, sets m_astRoot
  astRoot = m_astRoot;
  return ret;
}

void Driver::warning(const location& loc, const string& msg) {
  cerr << loc << ": warning: " << msg << "\n";
  m_gotWarning = true;
}

void Driver::error(const location& loc, const string& msg) {
  cerr << loc << ": error: " << msg << "\n";
  m_gotError = true;
}

void Driver::exitInternError(const location& loc, const string& msg) {
  cerr << loc << ": internal error: " << msg << "\n";
  exit(1);
}

void Driver::exitInternError(const string& msg) {
  cerr << "internal error: " << msg << "\n";
  exit(1);
}

// yy_flex_debug = true;


// yy::ef_parser parser(*this);
// parser.set_debug_level(/*m_traceParsing*/true);

