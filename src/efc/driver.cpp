#include "driver.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
using namespace std;
using namespace yy;

extern FILE *yyin;

Driver::Driver(const std::string& fileName) {
  // Ctor/Dtor must RAII yyin

  m_fileName = fileName;
  if (m_fileName.empty() || m_fileName == "-") {
    yyin = stdin;
  } else if (!(yyin = fopen(m_fileName.c_str(), "r"))) {
    error("cannot open " + m_fileName + ": " + strerror(errno));
    exit(EXIT_FAILURE);
  }
}

Driver::~Driver() {
  fclose(yyin);
}

void Driver::error(const location& loc, const string& msg) {
  cerr << loc << ": " << msg << endl;
}

void Driver::error(const string& msg) {
  cerr << msg << endl;
}

// yy_flex_debug = true;


// yy::ef_parser parser(*this);
// parser.set_debug_level(/*m_traceParsing*/true);

