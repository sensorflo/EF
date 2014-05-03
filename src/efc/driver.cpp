#include "driver.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
using namespace std;
using namespace yy;

extern FILE *yyin;
extern void yyrestart(FILE*);

Driver::Driver(const std::string& fileName) :
  m_gotError(false),
  m_gotWarning(false) {
  // Ctor/Dtor must RAII yyin

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

