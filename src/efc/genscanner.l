/* flex definitions section
----------------------------------------------------------------------*/
%option noyywrap nounput batch noinput 8bit full

%{
  #include "../scanner.h"
  #include "../parser.h"
  #include "../errorhandler.h"
  #include <cerrno>
  #include <climits>
  #include <cfloat>
  #include <cstdlib>
  #include <string>

 // Work around an incompatibility in flex (at least versions
 // 2.5.31 through 2.5.33): it generates code that does
 // not conform to C89.  See Debian bug 333231
 // <http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=333231>.
 #undef yywrap
 #define yywrap() 1

 // YY_USER_ACTION is always executed prior to the respective matched rule's
 // action. Here we advance the end point of the location as many chars as there
 // are in the current match. Note that in case the match contains newlines,
 // from a first point of view that is not correct. But the action of the
 // respective rule below will then modify loc correctly.
 #define YY_USER_ACTION  loc.columns(yyleng);

 using namespace std;
 using namespace yy;
 Location loc;

 int charCount(const char*, char ch);
%}

  /* identifier */
ID [a-zA-Z_][a-zA-Z_0-9]*

/* rules section
----------------------------------------------------------------------*/
%%

  // Code which is to be executed whenever the scanning routine is entered. If
  // the action of a rule does not return, we stay within the scanning routine,
  // i.e. the following code is not executed again. Thus they need to call
  // loc.step().
%{
  loc.step();
%}

  /* comments and blanks. They are all skiped and thus produce no token. */
\/\/[^\n]*          loc.step();
#![^\n]*            loc.step();
\/\*[^*]*(\*+[^#*]+)*\*+\/  loc.step(); loc.lines(charCount(yytext, '\n'));
[ \t]+              loc.step();
[\n]+               loc.lines(yyleng); loc.step(); return Parser::make_NEWLINE(loc);
\\[ \t]*((\/\/|#!)[^\n]*)?\n		loc.lines(1); loc.step();

  /* non-type keywords */
if\(                return Parser::make_IF_LPAREN(loc);
if                  return Parser::make_IF(loc);
then                return Parser::make_THEN(loc);
elif                return Parser::make_ELIF(loc);
else                return Parser::make_ELSE(loc);
while\(             return Parser::make_WHILE_LPAREN(loc);
while               return Parser::make_WHILE(loc);
do                  return Parser::make_DO(loc);
fun\(               return Parser::make_FUN_LPAREN(loc);
fun                 return Parser::make_FUN(loc);
val\(               return Parser::make_VAL_LPAREN(loc);
val                 return Parser::make_VAL(loc);
var\(               return Parser::make_VAR_LPAREN(loc);
var                 return Parser::make_VAR(loc);
end                 return Parser::make_END(loc);
endof               return Parser::make_ENDOF(loc);
not                 return Parser::make_NOT(loc);
and                 return Parser::make_AND(loc);
or                  return Parser::make_OR(loc);
raw_new\(           return Parser::make_RAW_NEW_LPAREN(loc);
raw_new             return Parser::make_RAW_NEW(loc);
raw_delete\(        return Parser::make_RAW_DELETE_LPAREN(loc);
raw_delete          return Parser::make_RAW_DELETE(loc);
nop                 return Parser::make_NOP(loc);
return\(            return Parser::make_RETURN_LPAREN(loc);
return              return Parser::make_RETURN(loc);
mut                 return Parser::make_MUT(loc);
is                  return Parser::make_IS(loc);
static              return Parser::make_STATIC(loc);
local               return Parser::make_LOCAL(loc);
noinit              return Parser::make_NOINIT(loc);


  /* type keywords */
void                return Parser::make_FUNDAMENTAL_TYPE(ObjTypeFunda::eVoid, loc);
char                return Parser::make_FUNDAMENTAL_TYPE(ObjTypeFunda::eChar, loc);
int                 return Parser::make_FUNDAMENTAL_TYPE(ObjTypeFunda::eInt, loc);
bool                return Parser::make_FUNDAMENTAL_TYPE(ObjTypeFunda::eBool, loc);
double              return Parser::make_FUNDAMENTAL_TYPE(ObjTypeFunda::eDouble, loc);
infer               return Parser::make_FUNDAMENTAL_TYPE(ObjTypeFunda::eInfer, loc);


  /* literal bool */
false               return Parser::make_NUMBER(NumberToken(0, ObjTypeFunda::eBool), loc);
true                return Parser::make_NUMBER(NumberToken(1, ObjTypeFunda::eBool), loc);


  /* literal char */
'.'                 return Parser::make_NUMBER(NumberToken(yytext[1], ObjTypeFunda::eChar), loc);


  /* literal integral / floatingpoint number  */
[0-9]+(\.[0-9]*)?(e[0-9]+)?{ID}?  {
  errno = 0;

  // convert the literal's text to long double, i.e. a type which is larger than
  // any type EF currently supports.
  char* suffix = nullptr; // after the call to strold, points past the end
                          // of the literal number/text
  const auto number = strtold(yytext, &suffix);

  const auto dotPos = strchr(yytext, '.');
  const auto containsDot = nullptr!=dotPos && dotPos<suffix;

  const auto ePos = strchr(yytext, 'e');
  const auto containsE = nullptr!=ePos && ePos<suffix;

  if (errno) {
    Error::throwError(errorHandler, Error::eInternalError,
      string("Scanner's rule matched a number, but strtol doesn't recognize a number. ")
      + strerror(errno));

  // currently no suffixes are recognized
  } else if (*suffix!='\0') {
    Error::throwError(errorHandler, Error::eUnknownIntegralLiteralSuffix,
      string("Unknown literal number suffix: ") + suffix);
  }

  // It's an floating point literal. Since we don't have suffixes yet its always
  // of type double
  else if (containsDot || containsE) {
    if (number<DBL_MIN || number>DBL_MAX) {
      Error::throwError(errorHandler, Error::eLiteralOutOfValidRange,
        "Floating point number literal is out of range");
    }
    // todo: also check for underflow / too many digits behind dot
    return Parser::make_NUMBER(
      NumberToken(static_cast<GeneralValue>(number), ObjTypeFunda::eDouble), loc);
  }

  // it's an integral literal. Since we don't have suffixes yet its always of
  // type int.
  else {
    if (number<INT_MIN || number>INT_MAX) {
      Error::throwError(errorHandler, Error::eLiteralOutOfValidRange,
        "Integer number literal is out of range");
    }
    return Parser::make_NUMBER(
      NumberToken(static_cast<GeneralValue>(number), ObjTypeFunda::eInt), loc);
  }
}

  /* punctuation */
":="                return Parser::make_COLON_EQUAL(loc);
"->"                return Parser::make_ARROW(loc);
"&&"                return Parser::make_AMPER_AMPER(loc);
"||"                return Parser::make_PIPE_PIPE(loc);
"=="                return Parser::make_EQUAL_EQUAL(loc);
"=<"                return Parser::make_EQUAL_LESS(loc);
"(="                return Parser::make_LPAREN_EQUAL(loc);
op([-+*/!]|&&|"||"|"=="|"_and"|"_or"|"_not"|"_new"|"_delete") {
  char* start = yytext + 2;     // skip "op"
  if (*start == '_') ++start;   // skip '_'
  return Parser::make_OP_NAME(start, loc);
}

";"                 return Parser::make_SEMICOLON(loc);
"$"                 return Parser::make_DOLLAR(loc);
":"                 return Parser::make_COLON(loc);
","                 return Parser::make_COMMA(loc);
"+"                 return Parser::make_PLUS(loc);
"-"                 return Parser::make_MINUS(loc);
"*"                 return Parser::make_STAR(loc);
"&"                 return Parser::make_AMPER(loc);
"/"                 return Parser::make_SLASH(loc);
"!"                 return Parser::make_EXCL(loc);
"="                 return Parser::make_EQUAL(loc);
"("                 return Parser::make_LPAREN(loc);
")"                 return Parser::make_RPAREN(loc);
"{"                 return Parser::make_LBRACE(loc);
"}"                 return Parser::make_RBRACE(loc);

  /* misc */
{ID}                return Parser::make_ID(yytext, loc);
<<EOF>>             return Parser::make_END_OF_FILE(loc);
.                   Error::throwError(errorHandler, Error::eInternalError, string("Scanner: unexpected character '") + yytext + "'");


%%
/* user code section
----------------------------------------------------------------------*/
int charCount(const char* p, char ch) {
  int cnt = 0;
  for ( /*nop*/; *p!='\0'; ++p) {
    if (*p==ch) {
      ++cnt;
    }
  }
  return cnt;
}

Location& locOfGenScanner() { return loc; }