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
    SCOPED_TRACE(""); \
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
  TEST_PARSE( "1+2+3-4-5", "seq(-(-(+(+(1 2) 3) 4) 5))",
    "+ and - have same precedence and are both left associative");
  TEST_PARSE( "1*2*3/4/5", "seq(/(/(*(*(1 2) 3) 4) 5))",
    "* and / have same precedence and are both left associative");
  TEST_PARSE( "1+2*3-4/5", "seq(-(+(1 *(2 3)) /(4 5)))",
    "* and / have higher precedence than + and -");
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
  TEST_PARSE( "+()", "seq(+())", "");
  TEST_PARSE( "-(1)", "seq(-(seq(1)))", "");
  TEST_PARSE( "*(1,2)", "seq(*(seq(1) seq(2)))", "");
  TEST_PARSE( "/(1,2,3)", "seq(/(seq(1) seq(2) seq(3)))", "");
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
  //Toggling 1) optional parentheses around parameters
  //Toggling 2) 'keyword...;' vs 'keyword(...)' syntax
  TEST_PARSE( "decl fun foo: () -> int;", "seq(declfun(foo ()))", spec);
  TEST_PARSE( "decl fun foo:    -> int;", "seq(declfun(foo ()))", spec);
  TEST_PARSE( "decl(fun foo: () -> int)", "seq(declfun(foo ()))", spec);
  TEST_PARSE( "decl(fun foo:    -> int)", "seq(declfun(foo ()))", spec);

  spec = "example with one argument and explicity return type";
  //Toggling 1) optional parentheses around parameters
  //Toggling 2) 'keyword...;' vs 'keyword(...)' syntax
  TEST_PARSE( "decl fun foo: arg1:int   -> int;", "seq(declfun(foo (arg1)))", spec);
  TEST_PARSE( "decl fun foo: (arg1:int) -> int;", "seq(declfun(foo (arg1)))", spec);
  TEST_PARSE( "decl(fun foo: arg1:int   -> int)", "seq(declfun(foo (arg1)))", spec);
  TEST_PARSE( "decl(fun foo: (arg1:int) -> int)", "seq(declfun(foo (arg1)))", spec);

  spec = "example with two arguments and explicity return type\n";
  //Toggling 1) optional parentheses around parameters
  //Toggling 2) 'keyword...;' vs 'keyword(...)' syntax
  TEST_PARSE( "decl fun foo: arg1:int, arg2:int   -> int;", "seq(declfun(foo (arg1 arg2)))", spec);
  TEST_PARSE( "decl fun foo: (arg1:int, arg2:int) -> int;", "seq(declfun(foo (arg1 arg2)))", spec);
  TEST_PARSE( "decl(fun foo: arg1:int, arg2:int   -> int)", "seq(declfun(foo (arg1 arg2)))", spec);
  TEST_PARSE( "decl(fun foo: (arg1:int, arg2:int) -> int)", "seq(declfun(foo (arg1 arg2)))", spec);

  spec = "example with implicit return type";
  //Toggling 1) optional parentheses around parameters
  //Toggling 2) zero vs one parameter
  //Toggling 3) 'keyword...;' vs 'keyword(...)' syntax
  TEST_PARSE( "decl fun foo:           ;", "seq(declfun(foo ()))", spec);
  TEST_PARSE( "decl fun foo: ()        ;", "seq(declfun(foo ()))", spec);
  TEST_PARSE( "decl fun foo: arg1:int  ;", "seq(declfun(foo (arg1)))", spec);
  TEST_PARSE( "decl fun foo: (arg1:int);", "seq(declfun(foo (arg1)))", spec);
  TEST_PARSE( "decl(fun foo:           )", "seq(declfun(foo ()))", spec);
  TEST_PARSE( "decl(fun foo: ()        )", "seq(declfun(foo ()))", spec);
  TEST_PARSE( "decl(fun foo: arg1:int  )", "seq(declfun(foo (arg1)))", spec);
  TEST_PARSE( "decl(fun foo: (arg1:int))", "seq(declfun(foo (arg1)))", spec);

  spec = "should allow trailing comma in argument list";
  //Toggling 1) optional parentheses around parameters
  //Toggling 2) optional return type
  TEST_PARSE( "decl fun foo: arg1:int,   -> int;", "seq(declfun(foo (arg1)))", spec);
  TEST_PARSE( "decl fun foo: (arg1:int,) -> int;", "seq(declfun(foo (arg1)))", spec);
  TEST_PARSE( "decl fun foo: arg1:int,         ;", "seq(declfun(foo (arg1)))", spec);
  TEST_PARSE( "decl fun foo: (arg1:int,)       ;", "seq(declfun(foo (arg1)))", spec);
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    a_reference_to_a_symbol_within_a_function_definition,
    parse,
    succeeds_AND_returns_correct_AST) ) {
  string spec = "example with one argument and a body just returning the arg";
  TEST_PARSE( "fun foo: (x:int) -> int = x;", "seq(fun(declfun(foo (x)) seq(x)))", spec);
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    a_function_definition,
    parse,
    succeeds_AND_returns_correct_AST) ) {

  string spec = "example with zero arguments and trivial body";
  TEST_PARSE( "fun foo:           = 42;", "seq(fun(declfun(foo ()) seq(42)))", spec);
  TEST_PARSE( "fun foo: ()        = 42;", "seq(fun(declfun(foo ()) seq(42)))", spec);
  TEST_PARSE( "fun foo: () -> int = 42;", "seq(fun(declfun(foo ()) seq(42)))", spec);
  TEST_PARSE( "fun(foo: () -> int = 42)", "seq(fun(declfun(foo ()) seq(42)))", spec);

  spec = "example with zero arguments and simple but not trivial body";
  TEST_PARSE( "fun foo:           = 42 1+2;", "seq(fun(declfun(foo ()) seq(42 +(1 2))))", spec);
  TEST_PARSE( "fun foo: ()        = 42 1+2;", "seq(fun(declfun(foo ()) seq(42 +(1 2))))", spec);
  TEST_PARSE( "fun foo: () -> int = 42 1+2;", "seq(fun(declfun(foo ()) seq(42 +(1 2))))", spec);
  TEST_PARSE( "fun(foo: () -> int = 42 1+2)", "seq(fun(declfun(foo ()) seq(42 +(1 2))))", spec);

  spec = "example with one argument and trivial body";
  TEST_PARSE( "fun foo: arg1:int          = 42;", "seq(fun(declfun(foo (arg1)) seq(42)))", spec);
  TEST_PARSE( "fun foo: (arg1:int)        = 42;", "seq(fun(declfun(foo (arg1)) seq(42)))", spec);
  TEST_PARSE( "fun foo: (arg1:int) -> int = 42;", "seq(fun(declfun(foo (arg1)) seq(42)))", spec);
  TEST_PARSE( "fun(foo: (arg1:int) -> int = 42)", "seq(fun(declfun(foo (arg1)) seq(42)))", spec);

  spec = "should allow trailing comma in argument list";
  TEST_PARSE( "fun foo: arg1:int,          = 42;", "seq(fun(declfun(foo (arg1)) seq(42)))", spec);
  TEST_PARSE( "fun foo: (arg1:int,)        = 42;", "seq(fun(declfun(foo (arg1)) seq(42)))", spec);

  spec = "example with two arguments and trivial body";
  TEST_PARSE( "fun foo: arg1:int, arg2:int          = 42;", "seq(fun(declfun(foo (arg1 arg2)) seq(42)))", spec);
  TEST_PARSE( "fun foo: (arg1:int, arg2:int)        = 42;", "seq(fun(declfun(foo (arg1 arg2)) seq(42)))", spec);
  TEST_PARSE( "fun foo: (arg1:int, arg2:int) -> int = 42;", "seq(fun(declfun(foo (arg1 arg2)) seq(42)))", spec);
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    a_function_call,
    parse,
    succeeds_AND_returns_correct_AST) ) {
  TEST_PARSE( "foo()", "seq(foo())", "");
  TEST_PARSE( "foo(42)", "seq(foo(seq(42)))", "");
  TEST_PARSE( "foo(42,77)", "seq(foo(seq(42) seq(77)))", "");
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    a_data_definition,
    parse,
    succeeds_AND_returns_correct_AST) ) {

  string spec = "trivial example with explicit init value";
  //Toggling 1) type defined in 3 variants: a) explictit with ':int'
  //            b) implicit with ':' or ''  
  //Toggling 2) 'keyword...;' vs 'keyword(...)' syntax
  //Toggling 3) val vs var
  TEST_PARSE( "val foo: int = 42;", "seq(data(decldata(foo) seq(42)))", spec);
  TEST_PARSE( "val foo:     = 42;", "seq(data(decldata(foo) seq(42)))", spec);
  TEST_PARSE( "val foo      = 42;", "seq(data(decldata(foo) seq(42)))", spec);
  TEST_PARSE( "val(foo: int = 42)", "seq(data(decldata(foo) seq(42)))", spec);
  TEST_PARSE( "val(foo:     = 42)", "seq(data(decldata(foo) seq(42)))", spec);
  TEST_PARSE( "val(foo      = 42)", "seq(data(decldata(foo) seq(42)))", spec);
  TEST_PARSE( "var foo: int = 42;", "seq(data(decldata(foo mut) seq(42)))", spec);
  TEST_PARSE( "var foo:     = 42;", "seq(data(decldata(foo mut) seq(42)))", spec);
  TEST_PARSE( "var foo      = 42;", "seq(data(decldata(foo mut) seq(42)))", spec);
  TEST_PARSE( "var(foo: int = 42)", "seq(data(decldata(foo mut) seq(42)))", spec);
  TEST_PARSE( "var(foo:     = 42)", "seq(data(decldata(foo mut) seq(42)))", spec);
  TEST_PARSE( "var(foo      = 42)", "seq(data(decldata(foo mut) seq(42)))", spec);

  spec = "trivial example with implicit init value";
  //Toggling 1) 'keyword...;' vs 'keyword(...)' syntax
  //Toggling 2) val vs var
  TEST_PARSE( "val foo:int;", "seq(data(decldata(foo)))", spec);
  TEST_PARSE( "val(foo:int)", "seq(data(decldata(foo)))", spec);
  TEST_PARSE( "var foo:int;", "seq(data(decldata(foo mut)))", spec);
  TEST_PARSE( "var(foo:int)", "seq(data(decldata(foo mut)))", spec);

  spec = "short version with implicit type";
  TEST_PARSE( "foo:=42", "seq(data(decldata(foo) seq(42)))", spec);

  spec = ":= has lower precedence than * +";
  TEST_PARSE( "foo:=1+2*3",
    "seq(data(decldata(foo) seq(+(1 *(2 3)))))", spec);

  spec = ":= is right associative";
  TEST_PARSE( "bar:=foo:=42",
    "seq(data(decldata(bar) seq(data(decldata(foo) seq(42)))))", spec);

  spec = ":= and = have same precedence";
  TEST_PARSE( "bar=foo:=42",
    "seq(=(bar data(decldata(foo) seq(42))))", spec);
  TEST_PARSE( "bar:=foo=42",
    "seq(data(decldata(bar) seq(=(foo 42))))", spec);
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    an_ifelse_flow_control_expression,
    parse,
    succeeds_AND_returns_correct_AST) ) {
  TEST_PARSE( "if  x: 1                           ;", "seq(if(seq(x) seq(1)))", "");
  TEST_PARSE( "if( x: 1                           )", "seq(if(seq(x) seq(1)))", "");
  TEST_PARSE( "if  x: 1                     else 2;", "seq(if(seq(x) seq(1) seq(2)))", "");
  TEST_PARSE( "if( x: 1                     else 2)", "seq(if(seq(x) seq(1) seq(2)))", "");
  TEST_PARSE( "if  x: 1 elif y: 2                 ;", "seq(if(seq(x) seq(1) seq(y) seq(2)))", "");
  TEST_PARSE( "if( x: 1 elif y: 2                 )", "seq(if(seq(x) seq(1) seq(y) seq(2)))", "");
  TEST_PARSE( "if  x: 1 elif y: 2           else 3;", "seq(if(seq(x) seq(1) seq(y) seq(2) seq(3)))", "");
  TEST_PARSE( "if( x: 1 elif y: 2           else 3)", "seq(if(seq(x) seq(1) seq(y) seq(2) seq(3)))", "");
  TEST_PARSE( "if  x: 1 elif y: 2 elif z: 3       ;", "seq(if(seq(x) seq(1) seq(y) seq(2) seq(z) seq(3)))", "");
  TEST_PARSE( "if( x: 1 elif y: 2 elif z: 3       )", "seq(if(seq(x) seq(1) seq(y) seq(2) seq(z) seq(3)))", "");
  TEST_PARSE( "if  x: 1 elif y: 2 elif z: 3 else 4;", "seq(if(seq(x) seq(1) seq(y) seq(2) seq(z) seq(3) seq(4)))", "");
  TEST_PARSE( "if( x: 1 elif y: 2 elif z: 3 else 4)", "seq(if(seq(x) seq(1) seq(y) seq(2) seq(z) seq(3) seq(4)))", "");
}

