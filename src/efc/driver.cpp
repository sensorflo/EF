#include "driver.h"
using namespace std;
using namespace yy;

Driver::Driver() :
  m_traceScanning(false),
  m_traceParsing(false) {
}

void Driver::error(const location& loc, const string& msg) {
  cerr << loc << ": " << msg << endl;
}

void Driver::error(const string& msg) {
  cerr << msg << endl;
}
