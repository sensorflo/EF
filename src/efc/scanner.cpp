#include "scanner.h"

/** _declare_ the function specified by YY_DECL, see YY_DECL. */
YY_DECL;

Scanner::Scanner(ErrorHandler& errorHandler) : m_errorHandler(errorHandler) {
}

Parser::symbol_type Scanner::pop() {
  // see YY_DECL
  return yylex_raw(m_errorHandler);
}
