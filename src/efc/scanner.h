#pragma once

#include "tokenstream.h"

#include <memory>
#include <string>

class ErrorHandler;

/** YY_DECL defines the signature (without trailing semicolon) of a function
returning the next token. The YY_DECL macro is used by the generated scanner to
_define_ the function specified by YY_DECL. It is named yylex_raw because it is
implemented by the generated scanner, wheras yylex, see genparser.yy, is the
function used by the generated parser, see there. */
#define YY_DECL Parser::symbol_type yylex_raw(ErrorHandler& errorHandler)

/** Wraps the generated scanner.

@todo Make it a singleton, since the generated scanner effectively is also a
      singleton */
class Scanner : public TokenStream {
public:
  Scanner(std::string fileName, ErrorHandler& errorHandler);
  ~Scanner();

  Parser::symbol_type pop() override;

private:
  std::string m_fileName;
  ErrorHandler& m_errorHandler;
  bool m_opened_yyin;
};
