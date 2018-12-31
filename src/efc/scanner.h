#pragma once

#include "tokenstream.h"

#include <memory>

class ErrorHandler;

/** \internal See also yylex and yylex_raw */
class Scanner : public TokenStream {
public:
  Scanner(ErrorHandler& errorHandler);

  Parser::symbol_type pop() override;

private:
  ErrorHandler& m_errorHandler;
};
