#include "scanner.h"

#include "errorhandler.h"

#include <cerrno>
#include <cstdio>
#include <stdexcept>
#include <string.h>
#include <utility>

using namespace std;

/** _declare_ the function specified by YY_DECL, see YY_DECL. */
YY_DECL;

// defined purely by generated parser
extern FILE* yyin;
// defined purely by generated parser
extern void yyrestart(FILE*);

// defined in configuration of generated scanner (genscanner.l).
extern Location& locOfGenScanner();

std::weak_ptr<Scanner> Scanner::sm_instance{};

std::shared_ptr<Scanner> Scanner::create(
  std::string fileName, ErrorHandler& errorHandler) {
  assert(sm_instance.expired());
  auto instance =
    shared_ptr<Scanner>{new Scanner{move(fileName), errorHandler}, Deleter()};
  sm_instance = instance;
  return instance;
}

Scanner::Scanner(std::string fileName, ErrorHandler& errorHandler)
  : m_fileName{move(fileName)}
  , m_errorHandler(errorHandler)
  , m_opened_yyin{false} {
  if (m_fileName.empty() || m_fileName == "-") { yyin = stdin; }
  else {
    if (!(yyin = fopen(m_fileName.c_str(), "r"))) {
      Error::throwError(m_errorHandler, Error::eCantOpenFileForReading,
        "cannot open '" + m_fileName + "' for reading (" + strerror(errno) +
          ")");
    }
    m_opened_yyin = true;
  }
  locOfGenScanner().initialize(&m_fileName);
  yyrestart(yyin);
}

Parser::symbol_type Scanner::pop() {
  // see YY_DECL
  return yylex_raw(m_errorHandler);
}

Scanner::~Scanner() {
  if (m_opened_yyin) { fclose(yyin); }

  // not required, but it makes it more explicit that the location is no longer
  // valid.
  locOfGenScanner().initialize(nullptr);
}
