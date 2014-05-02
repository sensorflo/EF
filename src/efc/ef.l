/* flex definitions section
----------------------------------------------------------------------*/
%option noyywrap nounput batch debug noinput

%{
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <string>
#include "../driver.h"
#include "parser.hpp"

// Work around an incompatibility in flex (at least versions
// 2.5.31 through 2.5.33): it generates code that does
// not conform to C89.  See Debian bug 333231
// <http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=333231>.
#undef yywrap
#define yywrap() 1

// an action which is always executed prior to the matched rule's action
#define YY_USER_ACTION  loc.columns(yyleng);

static yy::location loc;
%}

  /* not identifier start */
NIDS [^a-zA-Z_]
  /* identifier */
ID [a-zA-Z_][a-zA-Z_0-9]*

/* rules section
----------------------------------------------------------------------*/
%%

  // code which is to be executed whenever the scanning routine is entered
%{
  loc.step();
%}

[ \t]+              loc.step();
if/{NIDS}           return yy::Parser::make_IF(loc);
else/{NIDS}         return yy::Parser::make_ELSE(loc);
{ID}                return yy::Parser::make_ID(loc);
<<EOF>>             return yy::Parser::make_END_OF_FILE(loc);
.                   driver.error(loc, "invalid character");

%%
/* user code section
----------------------------------------------------------------------*/

void Driver::beginScan() {
  yy_flex_debug = m_traceScanning;
  if (m_fileName.empty() || m_fileName == "-")
    yyin = stdin;
  else if (!(yyin = fopen(m_fileName.c_str(), "r"))) {
    error ("cannot open " + m_fileName + ": " + strerror(errno));
    exit(EXIT_FAILURE);
  }
}

void Driver::endScan() {
  fclose(yyin);
}
