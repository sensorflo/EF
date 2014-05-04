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

 using namespace std;
 using namespace yy;
 static yy::location loc;
%}

  /* not identifier character */
NIDC [^a-zA-Z_0-9]
  /* identifier */
ID [a-zA-Z_][a-zA-Z_0-9]*

/* rules section
----------------------------------------------------------------------*/
%%

  // code which is to be executed whenever the scanning routine is entered
%{
  loc.step();
%}
 
  // bug: keywords and numbers are not recognized when at the very
  // end of the file
[ \t]+              loc.step();
if/{NIDC}           return yy::Parser::make_IF(loc);
else/{NIDC}         return yy::Parser::make_ELSE(loc);

[+-]?[ \t\n\r]*[0-9]+{ID}?  {
  errno = 0;
  char* suffix = NULL;
  long int number = strtol(yytext, &suffix, 10);
  if (errno) {
    driver.exitInternError(loc, string("Scanner's rule"
     "matched a number, but strtol doesn't recognize a number. ") +
     strerror(errno));

  // currently no suffixes are recognized
  } else if (*suffix!='\0') {
    driver.error(loc, string("Unknown literal number suffix: ") + suffix);

  // TODO: compare against target's INT_MIN/INT_MAX
  } else if (number<INT_MIN || number>INT_MAX) {
    number = number<INT_MIN ? INT_MIN : INT_MAX;
    driver.error(loc, "Numeric literal to big");
  }
  return yy::Parser::make_NUMBER(number, loc);
}

","                 return yy::Parser::make_COMMA(loc);

{ID}                return yy::Parser::make_ID(loc);
<<EOF>>             return yy::Parser::make_END_OF_FILE(loc);
.                   driver.error(loc, "invalid character");

%%
/* user code section
----------------------------------------------------------------------*/
