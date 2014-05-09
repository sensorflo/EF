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
  testParse( "42 64 77", "seq(42 64 77)", "trivial example, blanks as separator" );
  testParse( "42; 64; 77", "seq(42 64 77)", "trivial example, semicolons as separator" );
  testParse( "42; 64 77", "seq(42 64 77)",
    "trivial example, blanks and semicolons mixed as seperator" );
  testParse( "42;", "seq(42)", "trailing semicolon is allowed" );
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
    function_declaration,
    parse,
    succeeds_AND_returns_AST_form_of_function_declaration) ) {
  testParse( "decl fun foo end", "seq(declfun(foo))");
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    function_definition,
    parse,
    succeeds_AND_returns_AST_form_of_function_definition) ) {
  testParse( "fun foo = 42 end", "seq(fun(foo seq(42)))");
  testParse( "fun foo = 42; 1+2 end", "seq(fun(foo seq(42 +(1 2))))");
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    function_call,
    parse,
    succeeds_AND_returns_AST_form_of_function_call) ) {
  testParse( "foo()", "seq(foo())");
  testParse( "foo(42)", "seq(foo(seq(42)))");
  testParse( "foo(42,77)", "seq(foo(seq(42) seq(77)))");
}
