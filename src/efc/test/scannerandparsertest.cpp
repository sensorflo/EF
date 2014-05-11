#include "test.h"
#include "driverontmpfile.h"
#include "../ast.h"
#include "../gensrc/parser.hpp"
using namespace testing;
using namespace std;
using namespace yy;

void testParse(const string& efProgram, const string& expectedAst,
  const string& spec = "") {
  // setup
  DriverOnTmpFile driver(efProgram);

  // exercise
  AstNode* astRoot = NULL;
  int res = driver.d().parse(astRoot);

  // verify
  EXPECT_FALSE( driver.d().gotError() ) << "Scanner or parser found error.\n" <<
    "ef program = \"" << efProgram << "\"\n";
  EXPECT_EQ( 0, res) <<
    (res==1 ? "parser failed due to invalid input, i.e. input contains a syntax error\n" :
      (res==2 ? "parser failed due to memory exhaustion\n" :
        "parser failed for unknown reason\n")) <<
    "ef program = \"" << efProgram << "\"\n";
  ASSERT_TRUE( NULL != astRoot );
  EXPECT_EQ( expectedAst, astRoot->toStr() ) <<
    "ef program = \"" << efProgram << "\"\n";

  // tear down
  if (astRoot) { delete astRoot; }
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    literal_number_sequence,
    parse,
    succeeds_AND_returns_an_AST_form) ) {
  testParse( "42", "seq(42)", "trivial example with only one element" );
  testParse( "42 64 77", "seq(42 64 77)", "trivial example" );
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    math_expression,
    parse,
    succeeds_AND_returns_AST_form_of_math_expression) ) {
  testParse( "42 + 77", "seq(+(42 77))", "trivial example");
  testParse( "1+2+3-4-5", "seq(-(-(+(+(1 2) 3) 4) 5))",
    "+ and - have same precedence and are both left associative");
  testParse( "1*2*3/4/5", "seq(/(/(*(*(1 2) 3) 4) 5))",
    "* and / have same precedence and are both left associative");
  testParse( "1+2*3-4/5", "seq(-(+(1 *(2 3)) /(4 5)))",
    "* and / have higher precedence than + and -");
  testParse( "{1+2}*{3-4}/5", "seq(/(*(+(1 2) -(3 4)) 5))",
    "{} groups expressions and thus overwrites precedence");
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    assignement_expression,
    parse,
    succeeds_AND_returns_AST_form_of_assignement_expression) ) {
  testParse( "foo = 42", "seq(=(foo 42))", "trivial example");
  testParse( "foo = 1+2*3", "seq(=(foo +(1 *(2 3))))", "simple example");
  testParse( "foo = bar = 42", "seq(=(foo =(bar 42)))", "= is right associative");
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    function_declaration,
    parse,
    succeeds_AND_returns_AST_form_of_function_declaration) ) {
  string spec = "example with zero arguments";
  testParse( "decl fun foo: ();", "seq(declfun(foo ()))", spec);
  testParse( "decl fun foo:;", "seq(declfun(foo ()))", spec);

  spec = "example with one argument";
  testParse( "decl fun foo: arg1;", "seq(declfun(foo (arg1)))", spec);
  testParse( "decl fun foo: (arg1);", "seq(declfun(foo (arg1)))", spec);

  spec = "example with two arguments";
  testParse( "decl fun foo: arg1, arg2;", "seq(declfun(foo (arg1 arg2)))", spec);
  testParse( "decl fun foo: (arg1, arg2);", "seq(declfun(foo (arg1 arg2)))", spec);

  spec = "should allow trailing comma in argument list";
  testParse( "decl fun foo: arg1,;", "seq(declfun(foo (arg1)))", spec);
  testParse( "decl fun foo: (arg1,);", "seq(declfun(foo (arg1)))", spec);
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    reference_to_symbol_in_a_function_definition,
    parse,
    succeeds_AND_returns_AST_form_of_function_definition) ) {

  string spec = "example with one argument and a body just returning the arg";
  testParse( "fun foo: (x) = x;", "seq(fun(declfun(foo (x)) seq(x)))");
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    function_definition,
    parse,
    succeeds_AND_returns_AST_form_of_function_definition) ) {

  string spec = "example with zero arguments and trivial body";
  testParse( "fun foo: = 42;", "seq(fun(declfun(foo ()) seq(42)))");
  testParse( "fun foo: () = 42;", "seq(fun(declfun(foo ()) seq(42)))");

  spec = "example with zero arguments and simple but not trivial body";
  testParse( "fun foo: = 42 1+2;", "seq(fun(declfun(foo ()) seq(42 +(1 2))))");
  testParse( "fun foo: () = 42 1+2;", "seq(fun(declfun(foo ()) seq(42 +(1 2))))");

  spec = "example with one argument and trivial body";
  testParse( "fun foo: arg1 = 42;", "seq(fun(declfun(foo (arg1)) seq(42)))");
  testParse( "fun foo: (arg1) = 42;", "seq(fun(declfun(foo (arg1)) seq(42)))");

  spec = "should allow trailing comma in argument list";
  testParse( "fun foo: arg1, = 42;", "seq(fun(declfun(foo (arg1)) seq(42)))");
  testParse( "fun foo: (arg1,) = 42;", "seq(fun(declfun(foo (arg1)) seq(42)))");

  spec = "example with two arguments and trivial body";
  testParse( "fun foo: arg1, arg2 = 42;", "seq(fun(declfun(foo (arg1 arg2)) seq(42)))");
  testParse( "fun foo: (arg1, arg2) = 42;", "seq(fun(declfun(foo (arg1 arg2)) seq(42)))");
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    function_call,
    parse,
    succeeds_AND_returns_AST_form_of_function_call) ) {
  testParse( "foo()", "seq(foo())");
  testParse( "foo(42)", "seq(foo(seq(42)))");
  testParse( "foo(42,77)", "seq(foo(seq(42) seq(77)))");
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    data_declaration,
    parse,
    succeeds_AND_returns_AST_form_of_data_declaration) ) {
  string spec = "trivial example";
  testParse( "decl val foo:;", "seq(decldata(foo))", spec);
  testParse( "decl var foo:;", "seq(decldata(foo mut))", spec);
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    data_definition,
    parse,
    succeeds_AND_returns_AST_form_of_data_definition) ) {

  string spec = "trivial example";
  testParse( "val foo:= 42;", "seq(data(decldata(foo) seq(42)))", spec);
  testParse( "var foo:= 42;", "seq(data(decldata(foo mut) seq(42)))", spec);

  spec = "implicit init value";
  testParse( "val foo:;", "seq(data(decldata(foo)))", spec);
  testParse( "var foo:;", "seq(data(decldata(foo mut)))", spec);

  spec = "short version with implicit type";
  testParse( "foo:=42", "seq(data(decldata(foo) seq(42)))", spec);

  spec = ":= has lower precedence than * +";
  testParse( "foo:=1+2*3",
    "seq(data(decldata(foo) seq(+(1 *(2 3)))))", spec);

  spec = ":= is right associative";
  testParse( "bar:=foo:=42",
    "seq(data(decldata(bar) seq(data(decldata(foo) seq(42)))))", spec);

  spec = ":= and = have same precedence";
  testParse( "bar=foo:=42",
    "seq(=(bar data(decldata(foo) seq(42))))", spec);
  testParse( "bar:=foo=42",
    "seq(data(decldata(bar) seq(=(foo 42))))", spec);
}
