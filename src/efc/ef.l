/* flex definitions section
----------------------------------------------------------------------*/
%option noyywrap nounput batch noinput 8bit full

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
 yy::location loc;
 void yyinitializeParserLoc(string* filename) {
   loc.initialize(filename);
 }
%}

  /* identifier */
ID [a-zA-Z_][a-zA-Z_0-9]*

/* rules section
----------------------------------------------------------------------*/
%%

  // code which is to be executed whenever the scanning routine is entered
%{
  loc.step();
%}

  /* comments and blanks. They are all skiped and thus produce no token. */
\/\/[^\n]*          loc.step();
#![^\n]*            loc.step();
\/\*[^*]*(\*+[^#*]+)*\*+\/   loc.step();
[ \t]+              loc.step();
[\n]+               loc.lines(yyleng); loc.step();

  /* non-type keywords */
if                  return yy::Parser::make_IF(loc);
elif                return yy::Parser::make_ELIF(loc);
else                return yy::Parser::make_ELSE(loc);
fun                 return yy::Parser::make_FUN(loc);
val                 return yy::Parser::make_VAL(loc);
var                 return yy::Parser::make_VAR(loc);
decl                return yy::Parser::make_DECL(loc);
end                 return yy::Parser::make_END(loc);
not                 return yy::Parser::make_NOT(loc);
and                 return yy::Parser::make_AND(loc);
or                  return yy::Parser::make_OR(loc);
raw_new             return yy::Parser::make_RAW_NEW(loc);
raw_delete          return yy::Parser::make_RAW_DELETE(loc);
false               return yy::Parser::make_NUMBER(NumberToken(0, new ObjTypeFunda(ObjTypeFunda::eBool)), loc);
true                return yy::Parser::make_NUMBER(NumberToken(1, new ObjTypeFunda(ObjTypeFunda::eBool)), loc);


  /* type keywords */
int                 return yy::Parser::make_FUNDAMENTAL_TYPE(ObjTypeFunda::eInt, loc);
bool                return yy::Parser::make_FUNDAMENTAL_TYPE(ObjTypeFunda::eBool, loc);


  /* literal number */
[0-9]+{ID}?  {
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
  ObjType* objType = new ObjTypeFunda(ObjTypeFunda::eInt);
  return yy::Parser::make_NUMBER(NumberToken(number, objType), loc);
}

  /* punctuation */
":="                return yy::Parser::make_COLON_EQUAL(loc);
"->"                return yy::Parser::make_ARROW(loc);
"&&"                return yy::Parser::make_AMPER_AMPER(loc);
"||"                return yy::Parser::make_PIPE_PIPE(loc);
([-+*/!]|&&|"||"|"and"|"or"|"not"|"new"|"delete")[ \t]*"("    {   /*todo: allow comments and newlines intbetween*/
  char* p = yytext;
  for (/*nop*/; *p && *p!=' ' && *p!='\t' && *p!='('; ++p ) /*nop*/;
  return yy::Parser::make_OP_LPAREN(string(yytext,p), loc);
}

";"                 return yy::Parser::make_SEMICOLON(loc);
"$"                 return yy::Parser::make_DOLLAR(loc);
":"                 return yy::Parser::make_COLON(loc);
","                 return yy::Parser::make_COMMA(loc);
"+"                 return yy::Parser::make_PLUS(loc);
"-"                 return yy::Parser::make_MINUS(loc);
"*"                 return yy::Parser::make_STAR(loc);
"/"                 return yy::Parser::make_SLASH(loc);
"!"                 return yy::Parser::make_EXCL(loc);
"="                 return yy::Parser::make_EQUAL(loc);
"("                 return yy::Parser::make_LPAREN(loc);
")"                 return yy::Parser::make_RPAREN(loc);
"{"                 return yy::Parser::make_LBRACE(loc);
"}"                 return yy::Parser::make_RBRACE(loc);

  /* keyword-punctuation mixed */
"g("                return yy::Parser::make_G_LPAREN(loc);

  /* misc */
{ID}                return yy::Parser::make_ID(yytext, loc);
<<EOF>>             return yy::Parser::make_END_OF_FILE(loc);
.                   driver.error(loc, "invalid character");

%%
/* user code section
----------------------------------------------------------------------*/
