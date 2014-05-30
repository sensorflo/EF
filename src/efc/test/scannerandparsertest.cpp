#include "test.h"
#include "driverontmpfile.h"
#include "../ast.h"
#include "../gensrc/parser.hpp"
using namespace testing;
using namespace std;
using namespace yy;

string amendEfProgram(const string& efProgram) {
  return 
    string("\nEF program:\n") + 
    "--------------------\n" +
    efProgram +
    "\n--------------------\n";
}

string explainParseErrorCode(int errorCode) {
  if (errorCode==0)
    return "parser succeeded\n";
  if (errorCode==1)
    return "parser failed due to invalid input, i.e. input contains a syntax error\n";
  else if (errorCode==2)
    return "parser failed due to memory exhaustion\n";
  else
    return "parser failed for unknown reason\n";
}

void testParse(const string& efProgram, const string& expectedAst,
  const string& spec = "") {
  // setup
  DriverOnTmpFile driver(efProgram);

  // exercise
  AstSeq* actualAst = NULL;
  int res = driver.d().parse(actualAst);

  // verify
  EXPECT_FALSE( driver.d().gotError() ) <<
    "Scanner or parser reported an error to Driver\n" <<
    amendSpec(spec) <<
    amendEfProgram(efProgram);
  EXPECT_EQ( 0, res) <<
    explainParseErrorCode(res) <<
    amendSpec(spec) <<
    amendEfProgram(efProgram);
  ASSERT_TRUE( NULL != actualAst ) <<
    "parse did return NULL as actualAst\n" <<
    amendSpec(spec) <<
    amendEfProgram(efProgram);
  EXPECT_EQ( expectedAst, actualAst->toStr() ) <<
    amendSpec(spec) <<
    amendEfProgram(efProgram);

  // tear down
  if (actualAst) { delete actualAst; }
}

#define TEST_PARSE( efProgram, expectedAst, spec ) \
  { \
    SCOPED_TRACE("Test helper 'testParse' called from here"); \
    testParse(efProgram, expectedAst, spec);    \
  }

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    a_literal_number_sequence,
    parse,
    succeeds_AND_returns_correct_AST) ) {
  TEST_PARSE( "42", "seq(42)", "trivial example with only one element" );
  TEST_PARSE( "42 64 77", "seq(42 64 77)", "trivial example, blanks as separator" );
  TEST_PARSE( "42, 64, 77", "seq(42 64 77)", "trivial example, commas as separator" );
  TEST_PARSE( "42, 64 77", "seq(42 64 77)",
    "trivial example, blanks and commas mixed as seperator" );
  TEST_PARSE( "42,", "seq(42)", "trailing comma is allowed" );
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    a_math_expression,
    parse,
    succeeds_AND_returns_correct_AST) ) {
  TEST_PARSE( "42 + 77", "seq(+(42 77))", "trivial example");
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    a_math_expression,
    parse,
    it_honors_precedence_and_associativity) ) {

  // for each precedence level
  // - List the operators which belong to this precedence level. The first
  //   operator listed is the 'main' operator of this precedence level. The
  //   'main' operator is a concept only used in this test as a mean to
  //   specifiy relative precedence. Having a 'main' operator per group means
  //   that each operator's precedence only has to be specified relative to
  //   the main of its group, or if it is itself the 'main' operator, to the
  //   'main' operator of the next higher precedence level group.
  //
  // for each operator
  // - specifiy its associativity 
  // - specifiy its precedence

  // precedence is ordered from high to low


  // precedence level group: unary prefix not
  string spec = "! aka 'not' is right associative. ! and 'not' are synonyms";
  TEST_PARSE( "not not a", "seq(not(not(a)))", spec);
  TEST_PARSE( "not !   a", "seq(not(not(a)))", spec);
  TEST_PARSE( "!   not a", "seq(not(not(a)))", spec);
  TEST_PARSE( "!   !   a", "seq(not(not(a)))", spec);

  // precedence level group: binary * /
  spec = "* is left associative";
  TEST_PARSE( "a*b*c", "seq(*(*(a b) c))", spec);

  spec = "* has lower precedence than !";
  TEST_PARSE( "!a *  b", "seq(*(not(a) b))", spec);
  TEST_PARSE( " a * !b", "seq(*(a not(b)))", spec);

  spec = "/ is left associative";
  TEST_PARSE( "a/b/c", "seq(/(/(a b) c))", spec);

  spec = "/ has same precedence as *";
  TEST_PARSE( "a/b*c", "seq(*(/(a b) c))", spec);
  TEST_PARSE( "a*b/c", "seq(/(*(a b) c))", spec);

  // precedence level group: binary + -
  spec = "+ is left associative";
  TEST_PARSE( "a+b+c", "seq(+(+(a b) c))", spec);

  spec = "+ has lower precedence than *";
  TEST_PARSE( "a+b*c", "seq(+(a *(b c)))", spec);
  TEST_PARSE( "a*b+c", "seq(+(*(a b) c))", spec);

  spec = "- is left associative";
  TEST_PARSE( "a-b-c", "seq(-(-(a b) c))", spec);

  spec = "- has same precedence as +";
  TEST_PARSE( "a-b+c", "seq(+(-(a b) c))", spec);
  TEST_PARSE( "a+b-c", "seq(-(+(a b) c))", spec);

  // precedence level group: binary and &&
  spec = "&& aka 'and' is left associative. && and 'and' are synonyms.";
  TEST_PARSE( "a &&  b &&  c", "seq(and(and(a b) c))", spec);
  TEST_PARSE( "a &&  b and c", "seq(and(and(a b) c))", spec);
  TEST_PARSE( "a and b &&  c", "seq(and(and(a b) c))", spec);
  TEST_PARSE( "a and b and c", "seq(and(and(a b) c))", spec);

  spec = "&& aka 'and' has lower precedence than +";
  TEST_PARSE( "a &&  b +   c", "seq(and(a +(b c)))", spec);
  TEST_PARSE( "a and b +   c", "seq(and(a +(b c)))", spec);
  TEST_PARSE( "a +   b &&  c", "seq(and(+(a b) c))", spec);
  TEST_PARSE( "a +   b and c", "seq(and(+(a b) c))", spec);

  // precedence level group: binary or ||
  spec = "|| aka 'or' is left associative. || and 'or' are synonyms.";
  TEST_PARSE( "a || b || c", "seq(or(or(a b) c))", spec);
  TEST_PARSE( "a || b or c", "seq(or(or(a b) c))", spec);
  TEST_PARSE( "a or b || c", "seq(or(or(a b) c))", spec);
  TEST_PARSE( "a or b or c", "seq(or(or(a b) c))", spec);

  spec = "|| aka 'or' has lower precedence than && aka 'and'";
  TEST_PARSE( "a ||  b &&  c", "seq(or(a and(b c)))", spec);
  TEST_PARSE( "a ||  b and c", "seq(or(a and(b c)))", spec);
  TEST_PARSE( "a or  b &&  c", "seq(or(a and(b c)))", spec);
  TEST_PARSE( "a or  b and c", "seq(or(a and(b c)))", spec);
  TEST_PARSE( "a &&  b ||  c", "seq(or(and(a b) c))", spec);
  TEST_PARSE( "a &&  b or  c", "seq(or(and(a b) c))", spec);
  TEST_PARSE( "a and b ||  c", "seq(or(and(a b) c))", spec);
  TEST_PARSE( "a and b or  c", "seq(or(and(a b) c))", spec);
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    a_math_expression_containing_brace_grouping,
    parse,
    succeeds_AND_returns_correct_AST) ) {
  TEST_PARSE( "{1+2}*{3-4}/5", "seq(/(*(seq(+(1 2)) seq(-(3 4))) 5))",
    "{} group: 1) overwrites precedence and 2) turns sub_expr into expr (aka seq)");
  TEST_PARSE( "{1 2}", "seq(seq(1 2))",
    "{} group can contain not only a sub_expr but also an expr (aka seq)");
  TEST_PARSE( "{1 2}*3", "seq(*(seq(1 2) 3))",
    "{} group can contain not only a sub_expr but also an expr (aka seq)");
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    an_operator_in_call_syntax,
    parse,
    succeeds_AND_returns_correct_AST) ) {
  TEST_PARSE( "!(x)", "seq(not(x))", "");
  TEST_PARSE( "not(x)", "seq(not(x))", "");
  TEST_PARSE( "and(x,y,z)", "seq(and(x y z))", "");
  TEST_PARSE( "&&(x,y,z)", "seq(and(x y z))", "");
  TEST_PARSE( "or(x,y,z)", "seq(or(x y z))", "");
  TEST_PARSE( "||(x,y,z)", "seq(or(x y z))", "");
  TEST_PARSE( "+()", "seq(+())", "");
  TEST_PARSE( "-(1)", "seq(-(1))", "");
  TEST_PARSE( "*(1,2)", "seq(*(1 2))", "");
  TEST_PARSE( "/(1,2,3)", "seq(/(1 2 3))", "");
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    an_assignement_expression,
    parse,
    succeeds_AND_returns_correct_AST) ) {
  TEST_PARSE( "foo = 42", "seq(=(foo 42))", "trivial example");
  TEST_PARSE( "foo = 1+2*3", "seq(=(foo +(1 *(2 3))))", "simple example");
  TEST_PARSE( "foo = bar = 42", "seq(=(foo =(bar 42)))", "= is right associative");
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    a_function_declaration,
    parse,
    succeeds_AND_returns_correct_AST) ) {
  string spec = "example with zero arguments and explicit return type";
  //Toggling 1) optional colon after function name
  //Toggling 2) 'keyword...;' vs 'keyword(...)' syntax
  TEST_PARSE( "decl fun foo:() int;", "seq(declfun(foo () int))", spec);
  TEST_PARSE( "decl fun foo () int;", "seq(declfun(foo () int))", spec);
  TEST_PARSE( "decl(fun foo:() int)", "seq(declfun(foo () int))", spec);
  TEST_PARSE( "decl(fun foo () int)", "seq(declfun(foo () int))", spec);

  spec = "example with one argument and explicity return type";
  //Toggling 1) optional colon after function name
  //Toggling 2) 'keyword...;' vs 'keyword(...)' syntax
  TEST_PARSE( "decl fun foo:(arg1:int) int;", "seq(declfun(foo ((arg1 int-mut)) int))", spec);
  TEST_PARSE( "decl fun foo (arg1:int) int;", "seq(declfun(foo ((arg1 int-mut)) int))", spec);
  TEST_PARSE( "decl(fun foo:(arg1:int) int)", "seq(declfun(foo ((arg1 int-mut)) int))", spec);
  TEST_PARSE( "decl(fun foo (arg1:int) int)", "seq(declfun(foo ((arg1 int-mut)) int))", spec);

  spec = "example with two arguments and explicity return type\n";
  //Toggling 1) optional colon after function name
  //Toggling 2) 'keyword...;' vs 'keyword(...)' syntax
  TEST_PARSE( "decl fun foo:(arg1:int, arg2:int) int;", "seq(declfun(foo ((arg1 int-mut) (arg2 int-mut)) int))", spec);
  TEST_PARSE( "decl fun foo (arg1:int, arg2:int) int;", "seq(declfun(foo ((arg1 int-mut) (arg2 int-mut)) int))", spec);
  TEST_PARSE( "decl(fun foo:(arg1:int, arg2:int) int)", "seq(declfun(foo ((arg1 int-mut) (arg2 int-mut)) int))", spec);
  TEST_PARSE( "decl(fun foo (arg1:int, arg2:int) int)", "seq(declfun(foo ((arg1 int-mut) (arg2 int-mut)) int))", spec);

  spec = "example with implicit return type";
  //Toggling 1) optional colon after function name
  //Toggling 2) zero vs one parameter
  //Toggling 3) 'keyword...;' vs 'keyword(...)' syntax
  TEST_PARSE( "decl fun foo:          ;", "seq(declfun(foo () int))", spec);
  TEST_PARSE( "decl fun foo ()        ;", "seq(declfun(foo () int))", spec);
  TEST_PARSE( "decl fun foo:(arg1:int);", "seq(declfun(foo ((arg1 int-mut)) int))", spec);
  TEST_PARSE( "decl fun foo (arg1:int);", "seq(declfun(foo ((arg1 int-mut)) int))", spec);
  TEST_PARSE( "decl(fun foo:          )", "seq(declfun(foo () int))", spec);
  TEST_PARSE( "decl(fun foo ()        )", "seq(declfun(foo () int))", spec);
  TEST_PARSE( "decl(fun foo:(arg1:int))", "seq(declfun(foo ((arg1 int-mut)) int))", spec);
  TEST_PARSE( "decl(fun foo (arg1:int))", "seq(declfun(foo ((arg1 int-mut)) int))", spec);

  spec = "example with zero arguments, no parantheses around (empty) arg list and implicit return type";
  TEST_PARSE( "decl fun foo;", "seq(declfun(foo () int))", spec);

  spec = "should allow trailing comma in argument list";
  //Toggling 1) optional colon after function name
  //Toggling 2) optional return type
  TEST_PARSE( "decl fun foo:(arg1:int,) int;", "seq(declfun(foo ((arg1 int-mut)) int))", spec);
  TEST_PARSE( "decl fun foo (arg1:int,) int;", "seq(declfun(foo ((arg1 int-mut)) int))", spec);
  TEST_PARSE( "decl fun foo:(arg1:int,)    ;", "seq(declfun(foo ((arg1 int-mut)) int))", spec);
  TEST_PARSE( "decl fun foo (arg1:int,)    ;", "seq(declfun(foo ((arg1 int-mut)) int))", spec);
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    a_reference_to_a_symbol_within_a_function_definition,
    parse,
    succeeds_AND_returns_correct_AST) ) {
  string spec = "example with one argument and a body just returning the arg";
  TEST_PARSE( "fun foo: (x:int) int = x;", "seq(fun(declfun(foo ((x int-mut)) int) seq(x)))", spec);
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    a_function_definition,
    parse,
    succeeds_AND_returns_correct_AST) ) {

  string spec = "example with zero arguments and trivial body";
  TEST_PARSE( "fun foo:        = 42;", "seq(fun(declfun(foo () int) seq(42)))", spec);
  TEST_PARSE( "fun foo: ()     = 42;", "seq(fun(declfun(foo () int) seq(42)))", spec);
  TEST_PARSE( "fun foo: () int = 42;", "seq(fun(declfun(foo () int) seq(42)))", spec);
  TEST_PARSE( "fun(foo: () int = 42)", "seq(fun(declfun(foo () int) seq(42)))", spec);

  spec = "example with zero arguments and simple but not trivial body";
  TEST_PARSE( "fun foo:        = 42 1+2;", "seq(fun(declfun(foo () int) seq(42 +(1 2))))", spec);
  TEST_PARSE( "fun foo: ()     = 42 1+2;", "seq(fun(declfun(foo () int) seq(42 +(1 2))))", spec);
  TEST_PARSE( "fun foo: () int = 42 1+2;", "seq(fun(declfun(foo () int) seq(42 +(1 2))))", spec);
  TEST_PARSE( "fun(foo: () int = 42 1+2)", "seq(fun(declfun(foo () int) seq(42 +(1 2))))", spec);

  spec = "example with one argument and trivial body";
  TEST_PARSE( "fun foo: (arg1:int)     = 42;", "seq(fun(declfun(foo ((arg1 int-mut)) int) seq(42)))", spec);
  TEST_PARSE( "fun foo: (arg1:int)     = 42;", "seq(fun(declfun(foo ((arg1 int-mut)) int) seq(42)))", spec);
  TEST_PARSE( "fun foo: (arg1:int) int = 42;", "seq(fun(declfun(foo ((arg1 int-mut)) int) seq(42)))", spec);
  TEST_PARSE( "fun(foo: (arg1:int) int = 42)", "seq(fun(declfun(foo ((arg1 int-mut)) int) seq(42)))", spec);

  spec = "should allow trailing comma in argument list";
  TEST_PARSE( "fun foo: (arg1:int,)        = 42;", "seq(fun(declfun(foo ((arg1 int-mut)) int) seq(42)))", spec);
  TEST_PARSE( "fun foo: (arg1:int,)        = 42;", "seq(fun(declfun(foo ((arg1 int-mut)) int) seq(42)))", spec);

  spec = "example with two arguments and trivial body";
  TEST_PARSE( "fun foo: (arg1:int, arg2:int)     = 42;", "seq(fun(declfun(foo ((arg1 int-mut) (arg2 int-mut)) int) seq(42)))", spec);
  TEST_PARSE( "fun foo: (arg1:int, arg2:int)     = 42;", "seq(fun(declfun(foo ((arg1 int-mut) (arg2 int-mut)) int) seq(42)))", spec);
  TEST_PARSE( "fun foo: (arg1:int, arg2:int) int = 42;", "seq(fun(declfun(foo ((arg1 int-mut) (arg2 int-mut)) int) seq(42)))", spec);
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    a_function_call,
    parse,
    succeeds_AND_returns_correct_AST) ) {
  TEST_PARSE( "foo()", "seq(foo())", "");
  TEST_PARSE( "foo(42)", "seq(foo(42))", "");
  TEST_PARSE( "foo(42,77)", "seq(foo(42 77))", "");
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    a_data_definition,
    parse,
    succeeds_AND_returns_correct_AST) ) {

  string spec = "trivial example with explicit init value";
  //Toggling 1) type defined in 3 variants: a) explictit with ':int'
  //            b) explictit with ':bool' c) implicit with ':'
  //            d) implicit with ''  
  //Toggling 2) 'keyword...;' vs 'keyword(...)' syntax
  //Toggling 3) initializer behind id vs initializer behind type
  //Toggling 4) val vs var
  TEST_PARSE( "val foo: int  = 42;", "seq(data(decldata(foo int) 42))", spec);
  TEST_PARSE( "val foo: bool = 42;", "seq(data(decldata(foo bool) 42))", spec);
  TEST_PARSE( "val foo:      = 42;", "seq(data(decldata(foo int) 42))", spec);
  TEST_PARSE( "val foo       = 42;", "seq(data(decldata(foo int) 42))", spec);
  TEST_PARSE( "val(foo: int  = 42)", "seq(data(decldata(foo int) 42))", spec);
  TEST_PARSE( "val(foo: bool = 42)", "seq(data(decldata(foo bool) 42))", spec);
  TEST_PARSE( "val(foo:      = 42)", "seq(data(decldata(foo int) 42))", spec);
  TEST_PARSE( "val(foo       = 42)", "seq(data(decldata(foo int) 42))", spec);

  TEST_PARSE( "val foo = 42: int ;", "seq(data(decldata(foo int) 42))", spec);
  TEST_PARSE( "val foo = 42: bool;", "seq(data(decldata(foo bool) 42))", spec);
  TEST_PARSE( "val foo = 42:     ;", "seq(data(decldata(foo int) 42))", spec);
  TEST_PARSE( "val foo = 42      ;", "seq(data(decldata(foo int) 42))", spec);
  TEST_PARSE( "val(foo = 42: int )", "seq(data(decldata(foo int) 42))", spec);
  TEST_PARSE( "val(foo = 42: bool)", "seq(data(decldata(foo bool) 42))", spec);
  TEST_PARSE( "val(foo = 42:     )", "seq(data(decldata(foo int) 42))", spec);
  TEST_PARSE( "val(foo = 42      )", "seq(data(decldata(foo int) 42))", spec);

  TEST_PARSE( "var foo: int  = 42;", "seq(data(decldata(foo int-mut) 42))", spec);
  TEST_PARSE( "var foo: bool = 42;", "seq(data(decldata(foo bool-mut) 42))", spec);
  TEST_PARSE( "var foo:      = 42;", "seq(data(decldata(foo int-mut) 42))", spec);
  TEST_PARSE( "var foo       = 42;", "seq(data(decldata(foo int-mut) 42))", spec);
  TEST_PARSE( "var(foo: int  = 42)", "seq(data(decldata(foo int-mut) 42))", spec);
  TEST_PARSE( "var(foo: bool = 42)", "seq(data(decldata(foo bool-mut) 42))", spec);
  TEST_PARSE( "var(foo:      = 42)", "seq(data(decldata(foo int-mut) 42))", spec);
  TEST_PARSE( "var(foo       = 42)", "seq(data(decldata(foo int-mut) 42))", spec);

  TEST_PARSE( "var foo = 42: int ;", "seq(data(decldata(foo int-mut) 42))", spec);
  TEST_PARSE( "var foo = 42: bool;", "seq(data(decldata(foo bool-mut) 42))", spec);
  TEST_PARSE( "var foo = 42:     ;", "seq(data(decldata(foo int-mut) 42))", spec);
  TEST_PARSE( "var foo = 42      ;", "seq(data(decldata(foo int-mut) 42))", spec);
  TEST_PARSE( "var(foo = 42: int )", "seq(data(decldata(foo int-mut) 42))", spec);
  TEST_PARSE( "var(foo = 42: bool)", "seq(data(decldata(foo bool-mut) 42))", spec);
  TEST_PARSE( "var(foo = 42:     )", "seq(data(decldata(foo int-mut) 42))", spec);
  TEST_PARSE( "var(foo = 42      )", "seq(data(decldata(foo int-mut) 42))", spec);

  spec = "trivial example with implicit init value";
  //Toggling 1) 'keyword...;' vs 'keyword(...)' syntax
  //Toggling 2) val vs var
  TEST_PARSE( "val foo:int;", "seq(data(decldata(foo int)))", spec);
  TEST_PARSE( "val(foo:int)", "seq(data(decldata(foo int)))", spec);
  TEST_PARSE( "var foo:int;", "seq(data(decldata(foo int-mut)))", spec);
  TEST_PARSE( "var(foo:int)", "seq(data(decldata(foo int-mut)))", spec);

  spec = "short version with implicit type";
  TEST_PARSE( "foo:=42", "seq(data(decldata(foo int) 42))", spec);

  spec = ":= has lower precedence than * +";
  TEST_PARSE( "foo:=1+2*3",
    "seq(data(decldata(foo int) +(1 *(2 3))))", spec);

  spec = ":= is right associative";
  TEST_PARSE( "bar:=foo:=42",
    "seq(data(decldata(bar int) data(decldata(foo int) 42)))", spec);

  spec = ":= and = have same precedence";
  TEST_PARSE( "bar=foo:=42",
    "seq(=(bar data(decldata(foo int) 42)))", spec);
  TEST_PARSE( "bar:=foo=42",
    "seq(data(decldata(bar int) =(foo 42)))", spec);
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    an_ifelse_flow_control_expression,
    parse,
    succeeds_AND_returns_correct_AST) ) {
  // toggling 1) no elif part, one elif part, two elif parts
  // toggling 2) without/with else part
  // toggling 3) syntax ala 'if...;' or call syntax ala 'if(...)'
  // toggling 4) with/without colon after condition
  TEST_PARSE( "if  x: 1                           ;", "seq(if(x seq(1)))", "");
  TEST_PARSE( "if  x  1                           ;", "seq(if(x seq(1)))", "");
  TEST_PARSE( "if( x: 1                           )", "seq(if(x seq(1)))", "");
  TEST_PARSE( "if( x  1                           )", "seq(if(x seq(1)))", "");
  TEST_PARSE( "if  x: 1                     else 2;", "seq(if(x seq(1) seq(2)))", "");
  TEST_PARSE( "if  x  1                     else 2;", "seq(if(x seq(1) seq(2)))", "");
  TEST_PARSE( "if( x: 1                     else 2)", "seq(if(x seq(1) seq(2)))", "");
  TEST_PARSE( "if( x  1                     else 2)", "seq(if(x seq(1) seq(2)))", "");
  TEST_PARSE( "if  x: 1 elif y: 2                 ;", "seq(if(x seq(1) y seq(2)))", "");
  TEST_PARSE( "if  x  1 elif y  2                 ;", "seq(if(x seq(1) y seq(2)))", "");
  TEST_PARSE( "if( x: 1 elif y: 2                 )", "seq(if(x seq(1) y seq(2)))", "");
  TEST_PARSE( "if( x  1 elif y  2                 )", "seq(if(x seq(1) y seq(2)))", "");
  TEST_PARSE( "if  x: 1 elif y: 2           else 3;", "seq(if(x seq(1) y seq(2) seq(3)))", "");
  TEST_PARSE( "if  x  1 elif y  2           else 3;", "seq(if(x seq(1) y seq(2) seq(3)))", "");
  TEST_PARSE( "if( x: 1 elif y: 2           else 3)", "seq(if(x seq(1) y seq(2) seq(3)))", "");
  TEST_PARSE( "if( x  1 elif y  2           else 3)", "seq(if(x seq(1) y seq(2) seq(3)))", "");
  TEST_PARSE( "if  x: 1 elif y: 2 elif z: 3       ;", "seq(if(x seq(1) y seq(2) z seq(3)))", "");
  TEST_PARSE( "if  x  1 elif y  2 elif z  3       ;", "seq(if(x seq(1) y seq(2) z seq(3)))", "");
  TEST_PARSE( "if( x: 1 elif y: 2 elif z: 3       )", "seq(if(x seq(1) y seq(2) z seq(3)))", "");
  TEST_PARSE( "if( x  1 elif y  2 elif z  3       )", "seq(if(x seq(1) y seq(2) z seq(3)))", "");
  TEST_PARSE( "if  x: 1 elif y: 2 elif z: 3 else 4;", "seq(if(x seq(1) y seq(2) z seq(3) seq(4)))", "");
  TEST_PARSE( "if  x  1 elif y  2 elif z  3 else 4;", "seq(if(x seq(1) y seq(2) z seq(3) seq(4)))", "");
  TEST_PARSE( "if( x: 1 elif y: 2 elif z: 3 else 4)", "seq(if(x seq(1) y seq(2) z seq(3) seq(4)))", "");
  TEST_PARSE( "if( x  1 elif y  2 elif z  3 else 4)", "seq(if(x seq(1) y seq(2) z seq(3) seq(4)))", "");
}

