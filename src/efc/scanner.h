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

/** Wraps the generated scanner. Is implemented as singleton, because the
underlying generated parser is also effectively a singleton, since it is
implemented using static variables. */
class Scanner : public TokenStream {
public:
  /** Creates a singleton instance and returns it. If currently there is already
  an singleton instance, the method asserts. */
  static std::shared_ptr<Scanner> create(
    std::string fileName, ErrorHandler& errorHandler);

  Parser::symbol_type pop() override;

private:
  class Deleter {
  public:
    void operator()(Scanner* p) const { delete p; }
  };
  friend class Deleter;

  Scanner(std::string fileName, ErrorHandler& errorHandler);
  ~Scanner();

  /** The singleton instance */
  static std::weak_ptr<Scanner> sm_instance;
  std::string m_fileName;
  ErrorHandler& m_errorHandler;
  bool m_opened_yyin;
};
