#include "test.h"
#include "driverontmpfile.h"
#include "../ast.h"
#include "../errorhandler.h"
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

void testParse(const string efProgram, const string& expectedAst,
  Error::No expectedErrorNo, const string& spec) {

  // setup
  DriverOnTmpFile driver(efProgram);
  bool foreignThrow = false;
  string excptionwhat;
  std::unique_ptr<AstNode> actualAst{};

  // exercise
  try {
    actualAst = driver.d().scanAndParse();
  }

  catch (BuildError& ) { /* nop */  }
  catch (exception& e) { foreignThrow = true; excptionwhat = e.what(); }
  catch (exception* e) { foreignThrow = true; if ( e ) { excptionwhat = e->what(); } }
  catch (...)          { foreignThrow = true; }

  // verify that ...

  // ... no foreign exception is thrown
  ASSERT_FALSE( foreignThrow ) <<
    amendSpec(spec) << amend(driver.d().errorHandler()) << amendEfProgram(efProgram) <<
    "\nexceptionwhat: " << excptionwhat;

  // ... no error occured in case no error was expected
  const ErrorHandler::Container& errors = driver.d().errorHandler().errors();
  if ( expectedErrorNo == Error::eNone ) {
    EXPECT_TRUE(errors.empty()) <<
      "Expecting no error\n" <<
      amendSpec(spec) << amend(driver.d().errorHandler()) << amendEfProgram(efProgram);

    // ... and ast is as expected
    ASSERT_TRUE( nullptr != actualAst ) <<
      "scanAndParse did return nullptr as actualAst\n" <<
      amendSpec(spec) <<
      amendEfProgram(efProgram);
    EXPECT_EQ( expectedAst, actualAst->toStr() ) <<
      amendSpec(spec) <<
      amendEfProgram(efProgram);
  }

  // ... or if an error occured, it is the one we expected
  else {
    EXPECT_EQ(1U, errors.size()) <<
      "Expecting exactly one error\n" <<
      amendSpec(spec) << amend(driver.d().errorHandler()) << amendEfProgram(efProgram);

    if ( ! errors.empty() ) {
      EXPECT_EQ(expectedErrorNo, errors.front()->no()) <<
        amendSpec(spec) << amend(driver.d().errorHandler()) << amendEfProgram(efProgram);;
    }
  }
}

#define TEST_PARSE_REPORTS_ERROR(efProgram, expectedErrorNo, spec)     \
  {                                                                     \
    SCOPED_TRACE("testParseReportsError called from here (via TEST_PARSE_REPORTS_ERROR)"); \
    static_assert(expectedErrorNo != Error::eNone, "");                 \
    testParse(efProgram, "", expectedErrorNo, spec);                     \
  }

#define TEST_PARSE( efProgram, expectedAst, spec ) \
  { \
    SCOPED_TRACE("testParse called from here (via TEST_PARSE)"); \
    testParse(efProgram, expectedAst, Error::eNone, spec);        \
  }

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    a_nop,
    scanAndParse,
    succeeds_AND_returns_correct_AST) ) {
  TEST_PARSE( "nop", ":;nop", "trivial example");
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    a_return_expression,
    scanAndParse,
    succeeds_AND_returns_correct_AST) ) {
  TEST_PARSE( "return  42$", ":;return(42)", "trivial example");
  TEST_PARSE( "return(42)" , ":;return(42)", "trivial example");
  TEST_PARSE( "return$"    , ":;return(nop)", "");
  TEST_PARSE( "return()"   , ":;return(nop)", "");
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    a_literal,
    scanAndParse,
    succeeds_AND_returns_correct_AST) ) {
  TEST_PARSE( "42", ":;42", "trivial example");
  TEST_PARSE( "false", ":;0bool", "trivial example");
  TEST_PARSE( "true", ":;1bool", "trivial example");
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    a_cast_expression,
    scanAndParse,
    succeeds_AND_returns_correct_AST) ) {
  TEST_PARSE( "int(false)", ":;int(0bool)", "trivial example");
  TEST_PARSE( "bool(0)", ":;bool(0)", "trivial example");
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    a_math_expression,
    scanAndParse,
    succeeds_AND_returns_correct_AST) ) {
  TEST_PARSE( "42 + 77", ":;+(42 77)", "trivial example");
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    a_math_expression,
    scanAndParse,
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


  // precedence level group: function call
  string spec = "function call () is right associative";
  TEST_PARSE( "foo(a)(b)", ":;call(call(foo a) b)", spec);

  // precedence level group: unary prefix not & * - +
  spec = "! aka 'not' is right associative. ! and 'not' are synonyms";
  TEST_PARSE( "not not a", ":;!(!(a))", spec);
  TEST_PARSE( "not !   a", ":;!(!(a))", spec);
  TEST_PARSE( "!   not a", ":;!(!(a))", spec);
  TEST_PARSE( "!   !   a", ":;!(!(a))", spec);

  spec = "! has lower precedence than function call ()";
  TEST_PARSE( "!foo()", ":;!(call(foo))", spec);

  spec = "& is right associative.";
  TEST_PARSE( "& & a", ":;&(&(a))", spec);

  spec = "& and ! have equal precedence";
  TEST_PARSE( "& ! a", ":;&(!(a))", spec);
  TEST_PARSE( "! & a", ":;!(&(a))", spec);

  spec = "& has lower precedence than function call ()";
  TEST_PARSE( "&foo()", ":;&(call(foo))", spec);

  spec = "* is right associative.";
  TEST_PARSE( "* * a", ":;*(*(a))", spec);

  spec = "* and ! have equal precedence";
  TEST_PARSE( "* ! a", ":;*(!(a))", spec);
  TEST_PARSE( "! * a", ":;!(*(a))", spec);

  spec = "* has lower precedence than function call ()";
  TEST_PARSE( "*foo()", ":;*(call(foo))", spec);

  spec = "unary - is right associative.";
  TEST_PARSE( "- - a", ":;-(-(a))", spec);

  spec = "unary - and ! have equal precedence";
  TEST_PARSE( "- ! a", ":;-(!(a))", spec);
  TEST_PARSE( "! - a", ":;!(-(a))", spec);

  spec = "- has lower precedence than function call ()";
  TEST_PARSE( "-foo()", ":;-(call(foo))", spec);

  spec = "unary + is right associative.";
  TEST_PARSE( "+ + a", ":;+(+(a))", spec);

  spec = "unary + and ! have equal precedence";
  TEST_PARSE( "+ ! a", ":;+(!(a))", spec);
  TEST_PARSE( "! + a", ":;!(+(a))", spec);

  spec = "unary + has lower precedence than function call ()";
  TEST_PARSE( "+foo()", ":;+(call(foo))", spec);

  // precedence level group: binary * /
  spec = "* is left associative";
  TEST_PARSE( "a*b*c", ":;*(*(a b) c)", spec);

  spec = "* has lower precedence than !";
  TEST_PARSE( "!a *  b", ":;*(!(a) b)", spec);
  TEST_PARSE( " a * !b", ":;*(a !(b))", spec);

  spec = "/ is left associative";
  TEST_PARSE( "a/b/c", ":;/(/(a b) c)", spec);

  spec = "/ has same precedence as *";
  TEST_PARSE( "a/b*c", ":;*(/(a b) c)", spec);
  TEST_PARSE( "a*b/c", ":;/(*(a b) c)", spec);

  // precedence level group: binary + -
  spec = "+ is left associative";
  TEST_PARSE( "a+b+c", ":;+(+(a b) c)", spec);

  spec = "+ has lower precedence than *";
  TEST_PARSE( "a+b*c", ":;+(a *(b c))", spec);
  TEST_PARSE( "a*b+c", ":;+(*(a b) c)", spec);

  spec = "- is left associative";
  TEST_PARSE( "a-b-c", ":;-(-(a b) c)", spec);

  spec = "- has same precedence as +";
  TEST_PARSE( "a-b+c", ":;+(-(a b) c)", spec);
  TEST_PARSE( "a+b-c", ":;-(+(a b) c)", spec);

  // precedence level group: ==
  spec = "== is left associative";
  TEST_PARSE( "a==b==c", ":;==(==(a b) c)", spec);
  
  spec = "== has lower precedence than +";
  TEST_PARSE( "a==b+c", ":;==(a +(b c))", spec);
  TEST_PARSE( "a+b==c", ":;==(+(a b) c)", spec);

  // precedence level group: binary and &&
  spec = "&& aka 'and' is left associative. && and 'and' are synonyms.";
  TEST_PARSE( "a &&  b &&  c", ":;and(and(a b) c)", spec);
  TEST_PARSE( "a &&  b and c", ":;and(and(a b) c)", spec);
  TEST_PARSE( "a and b &&  c", ":;and(and(a b) c)", spec);
  TEST_PARSE( "a and b and c", ":;and(and(a b) c)", spec);

  spec = "&& aka 'and' has lower precedence than +";
  TEST_PARSE( "a &&  b +   c", ":;and(a +(b c))", spec);
  TEST_PARSE( "a and b +   c", ":;and(a +(b c))", spec);
  TEST_PARSE( "a +   b &&  c", ":;and(+(a b) c)", spec);
  TEST_PARSE( "a +   b and c", ":;and(+(a b) c)", spec);

  // precedence level group: binary or ||
  spec = "|| aka 'or' is left associative. || and 'or' are synonyms.";
  TEST_PARSE( "a || b || c", ":;or(or(a b) c)", spec);
  TEST_PARSE( "a || b or c", ":;or(or(a b) c)", spec);
  TEST_PARSE( "a or b || c", ":;or(or(a b) c)", spec);
  TEST_PARSE( "a or b or c", ":;or(or(a b) c)", spec);

  spec = "|| aka 'or' has lower precedence than && aka 'and'";
  TEST_PARSE( "a ||  b &&  c", ":;or(a and(b c))", spec);
  TEST_PARSE( "a ||  b and c", ":;or(a and(b c))", spec);
  TEST_PARSE( "a or  b &&  c", ":;or(a and(b c))", spec);
  TEST_PARSE( "a or  b and c", ":;or(a and(b c))", spec);
  TEST_PARSE( "a &&  b ||  c", ":;or(and(a b) c)", spec);
  TEST_PARSE( "a &&  b or  c", ":;or(and(a b) c)", spec);
  TEST_PARSE( "a and b ||  c", ":;or(and(a b) c)", spec);
  TEST_PARSE( "a and b or  c", ":;or(and(a b) c)", spec);

  // precedence level group: binrary = :=
  spec = "= is right associative";
  TEST_PARSE( "a = b = c", ":;=(a =(b c))", spec);

  spec = ":= is right associative";
  TEST_PARSE( "a := b := c", ":;data(a int (data(b int (c))))", spec);

  spec = "= has lower precedence than || aka 'or'";
  TEST_PARSE( "a =  b or c", ":;=(a or(b c))", spec);
  TEST_PARSE( "a or b =  c", ":;=(or(a b) c)", spec);

  spec = ":= has same precedence as =";
  TEST_PARSE( "a := b =  c", ":;data(a int (=(b c)))", spec);
  TEST_PARSE( "a =  b := c", ":;=(a data(b int (c)))", spec);
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    a_sequence,
    scanAndParse,
    succeeds_AND_returns_correct_AST) ) {

  string spec = "Sequence are created not by ';'/newlines, but by the context "
    "which demands a sequence. ";
  string example = spec + "E.g. a block imposese sequence context";
  TEST_PARSE( "a",         ":;a", example);

  example = spec + "E.g. grouping parentheses impose sequence context";
  TEST_PARSE( "a = (b = c)", ":;=(a ;=(b c))", example);

  spec = "sequence elements are seprated by ';' or by newlines";
  TEST_PARSE( "a ;  b",      ":;(a b)", spec);
  TEST_PARSE( "a \n b",      ":;(a b)", spec);
  TEST_PARSE( "a ;  b ;  c", ":;(a b c)", spec);
  TEST_PARSE( "a \n b \n c", ":;(a b c)", spec);

  spec = "trailing ';' or newlines are allowed and are ignored";
  TEST_PARSE( "a ; b ;",   ":;(a b)", spec);
  TEST_PARSE( "a \n b \n", ":;(a b)", spec);
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
  a_newline_sequence_operator_in_a_non_trivial_place,
  scanAndParse,
  succeeds_AND_returns_correct_AST) ) {

  string spec = "before list start";
  TEST_PARSE( "a \n val foo :int$",   ":;(a data(foo int ()))", spec);

  spec = "after list end";
  TEST_PARSE( "val foo :int$ \n a",   ":;(data(foo int ()) a)", spec);

  spec = "before and after = as list argument separator";
  TEST_PARSE( "val foo \n = \n a :int$", ":;data(foo int (a))", spec);

  spec = "before and after = as list argument separator";
  TEST_PARSE( "val foo \n = \n a :int$", ":;data(foo int (a))", spec);

  spec = "before and after ( as separator (e.g. when it's an initializer)";
  TEST_PARSE( "val foo \n ( \n a ) :int$", ":;data(foo int (a))", spec);
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME4(
  a_newline_sequence_operator_before_a_binary_operator_or_an_unary_postfix_operator,
  scanAndParse,
  reports_eScanOrParseFailed,
  BECAUSE_no_binary_operator_can_appear_before_another_binary_operator_or_unary_postfix_operator) ) {

  string spec = "Example: binary operetor which is not also an unary prefix operator";
  TEST_PARSE_REPORTS_ERROR(
    "a \n / b", Error::eScanOrParseFailed, spec);

  // no unary postfix operators yet
}

// That newlines after list start, around list separator, and before list end
// are dropped is tested by tokenfiltertest.
TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    a_binary_or_unary_prefix_operator_followed_by_newline,
    scanAndParse,
    ignores_newline_AND_succeeds_AND_returns_correct_AST) ) {

  string spec = "Example: binary operand";
  TEST_PARSE("a=\nb", ":;=(a b)", spec);
  TEST_PARSE("a or\nb", ":;or(a b)", spec);
  TEST_PARSE("a||\nb", ":;or(a b)", spec);
  TEST_PARSE("a and\nb", ":;and(a b)", spec);
  TEST_PARSE("a&&\nb", ":;and(a b)", spec);
  TEST_PARSE("a==\nb", ":;==(a b)", spec);
  TEST_PARSE("a+\nb", ":;+(a b)", spec);
  TEST_PARSE("a-\nb", ":;-(a b)", spec);
  TEST_PARSE("a*\nb", ":;*(a b)", spec);
  TEST_PARSE("a/\nb", ":;/(a b)", spec);

  spec = "Example: unary prefix operand";
  TEST_PARSE("not \na", ":;!(a)", spec);
  TEST_PARSE("!\na", ":;!(a)", spec);
  TEST_PARSE("*\na", ":;*(a)", spec);
  TEST_PARSE("&\na", ":;&(a)", spec);
  TEST_PARSE("-\na", ":;-(a)", spec);
  TEST_PARSE("+\na", ":;+(a)", spec);

  spec = "Example: unary prefix operand of type expression";
  TEST_PARSE("val foo :*\nint$", ":;data(foo raw*int ())", spec);
  TEST_PARSE("val foo :mut\nint$", ":;data(foo mut-int ())", spec);
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    a_math_expression_containin_brace_grouping,
    scanAndParse,
    succeeds_AND_returns_correct_AST) ) {
  TEST_PARSE( "(1+2)*(3-4)/5", ":;/(*(;+(1 2) ;-(3 4)) 5)",
    "(...) group: 1) overwrites precedence and 2) turns sub_expr into exp");
  TEST_PARSE( "(1 ; 2)", ":;;(1 2)",
    "(...) group can contain not only a sub_expr but also an sa expr seq");
  TEST_PARSE( "(1 ; 2)*3", ":;*(;(1 2) 3)",
    "(...) group can contain not only a sub_expr but also an sa expr seq");
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    an_operator_in_call_syntax,
    scanAndParse,
    succeeds_AND_returns_correct_AST) ) {
  // unary
  TEST_PARSE( "op!(x)", ":;!(x)", "");
  TEST_PARSE( "op_not(x)", ":;!(x)", "");

  // binary
  TEST_PARSE( "op_and(x,y)", ":;and(x y)", "");
  TEST_PARSE( "op&&(x,y)", ":;and(x y)", "");
  TEST_PARSE( "op_or(x,y)", ":;or(x y)", "");
  TEST_PARSE( "op||(x,y)", ":;or(x y)", "");
  TEST_PARSE( "op==(1,1)", ":;==(1 1)", "");
  TEST_PARSE( "op*(1,2)", ":;*(1 2)", "");
  TEST_PARSE( "op/(1,2)", ":;/(1 2)", "");
  TEST_PARSE( "op+(1,2)", ":;+(1 2)", "");
  TEST_PARSE( "op-(1,2)", ":;-(1 2)", "");

  // binary with more than two args result in a tree
  TEST_PARSE( "op+(1,2,3)", ":;+(+(1 2) 3)", ""); // left associative
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    an_assignement_expression,
    scanAndParse,
    succeeds_AND_returns_correct_AST) ) {
  TEST_PARSE( "foo = 42", ":;=(foo 42)", "trivial example");
  TEST_PARSE( "foo = 1+2*3", ":;=(foo +(1 *(2 3)))", "simple example");
  TEST_PARSE( "foo = bar = 42", ":;=(foo =(bar 42))", "= is right associative");
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    a_reference_to_a_symbol_within_a_function_definition,
    scanAndParse,
    succeeds_AND_returns_correct_AST) ) {
  string spec = "example with one argument and a body just returning the arg";
  TEST_PARSE( "fun foo: (x:int) int ,= x$", ":;fun(foo ((x int)) int :;x)", spec);
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    a_function_definition,
    scanAndParse,
    succeeds_AND_returns_correct_AST) ) {

  string spec = "example with zero arguments and trivial body";
  TEST_PARSE( "fun  foo: () int ,= 42$"          , ":;fun(foo () int :;42)", spec);
  TEST_PARSE( "fun  foo: () bool,= true$"        , ":;fun(foo () bool :;1bool)", spec);
  TEST_PARSE( "fun( foo: () int ,= 42)"          , ":;fun(foo () int :;42)", spec);
  TEST_PARSE( "fun  foo: () int ,= 42 end"       , ":;fun(foo () int :;42)", spec);
  TEST_PARSE( "fun  foo: () int ,= 42 endof foo$", ":;fun(foo () int :;42)", spec);

  spec = "example with zero arguments and simple but not trivial body";
  TEST_PARSE( "fun  foo: () int ,= 42; 1+2$", ":;fun(foo () int :;(42 +(1 2)))", spec);
  TEST_PARSE( "fun( foo: () int ,= 42; 1+2)", ":;fun(foo () int :;(42 +(1 2)))", spec);

  spec = "example with one argument and trivial body";
  TEST_PARSE( "fun  foo: (arg1:int) int ,= 42$", ":;fun(foo ((arg1 int)) int :;42)", spec);
  TEST_PARSE( "fun( foo: (arg1:int) int ,= 42)", ":;fun(foo ((arg1 int)) int :;42)", spec);

  spec = "should allow trailing comma in argument list";
  TEST_PARSE( "fun foo: (arg1:int,) int ,= 42$", ":;fun(foo ((arg1 int)) int :;42)", spec);

  spec = "example with two arguments and trivial body";
  TEST_PARSE( "fun foo: (arg1:int, arg2:int) int ,= 42$", ":;fun(foo ((arg1 int) (arg2 int)) int :;42)", spec);
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    a_function_call,
    scanAndParse,
    succeeds_AND_returns_correct_AST) ) {

  string spec = "trivial example";
  TEST_PARSE( "foo()", ":;call(foo)", spec);
  TEST_PARSE( "foo(42)", ":;call(foo 42)", spec);
  TEST_PARSE( "foo(42,77)", ":;call(foo 42 77)", spec);

  spec = "Function address can be given by an expression";
  TEST_PARSE( "(foo+bar)(42,77)", ":;call(;+(foo bar) 42 77)", spec);
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    a_data_definition,
    scanAndParse,
    succeeds_AND_returns_correct_AST) ) {

  string spec = "trivial example with explicit init value";
  //Toggling 1) type defined in 3 variants: a) explictit with ':int'
  //            b) explictit with ':bool' c) implicit with ':'
  //            d) implicit with ''  
  //Toggling 2) 'keyword...;' vs 'keyword(...)' vs 'keyword...end...;' syntax
  //Toggling 3) initializer behind id vs initializer behind type
  //Toggling 4) val vs var
  TEST_PARSE( "val  foo : int  ,= 42   $"         , ":;data(foo int (42))", spec);
  TEST_PARSE( "val  foo : bool ,= true $"         , ":;data(foo bool (1bool))", spec);
  TEST_PARSE( "val  foo :      ,= 42   $"         , ":;data(foo int (42))", spec);
  TEST_PARSE( "val  foo        ,= 42   $"         , ":;data(foo int (42))", spec);
  TEST_PARSE( "val( foo : int  ,= 42   )"         , ":;data(foo int (42))", spec);
  TEST_PARSE( "val( foo : bool ,= true )"         , ":;data(foo bool (1bool))", spec);
  TEST_PARSE( "val( foo :      ,= 42   )"         , ":;data(foo int (42))", spec);
  TEST_PARSE( "val( foo        ,= 42   )"         , ":;data(foo int (42))", spec);
  TEST_PARSE( "val  foo : int  ,= 42   end"       , ":;data(foo int (42))", spec);
  TEST_PARSE( "val  foo : bool ,= true end"       , ":;data(foo bool (1bool))", spec);
  TEST_PARSE( "val  foo :      ,= 42   end"       , ":;data(foo int (42))", spec);
  TEST_PARSE( "val  foo        ,= 42   end"       , ":;data(foo int (42))", spec);
  TEST_PARSE( "val  foo : int  ,= 42   endof foo$", ":;data(foo int (42))", spec);
  TEST_PARSE( "val  foo : bool ,= true endof foo$", ":;data(foo bool (1bool))", spec);
  TEST_PARSE( "val  foo :      ,= 42   endof foo$", ":;data(foo int (42))", spec);
  TEST_PARSE( "val  foo        ,= 42   endof foo$", ":;data(foo int (42))", spec);

  TEST_PARSE( "val  foo ,= 42   : int  $"         , ":;data(foo int (42))", spec);
  TEST_PARSE( "val  foo ,= true : bool $"         , ":;data(foo bool (1bool))", spec);
  TEST_PARSE( "val  foo ,= 42   :      $"         , ":;data(foo int (42))", spec);
  TEST_PARSE( "val  foo ,= 42          $"         , ":;data(foo int (42))", spec);
  TEST_PARSE( "val( foo ,= 42   : int  )"         , ":;data(foo int (42))", spec);
  TEST_PARSE( "val( foo ,= true : bool )"         , ":;data(foo bool (1bool))", spec);
  TEST_PARSE( "val( foo ,= 42   :      )"         , ":;data(foo int (42))", spec);
  TEST_PARSE( "val( foo ,= 42          )"         , ":;data(foo int (42))", spec);
  TEST_PARSE( "val  foo ,= 42   : int  end"       , ":;data(foo int (42))", spec);
  TEST_PARSE( "val  foo ,= true : bool end"       , ":;data(foo bool (1bool))", spec);
  TEST_PARSE( "val  foo ,= 42   :      end"       , ":;data(foo int (42))", spec);
  TEST_PARSE( "val  foo ,= 42          end"       , ":;data(foo int (42))", spec);
  TEST_PARSE( "val  foo ,= 42   : int  endof foo$", ":;data(foo int (42))", spec);
  TEST_PARSE( "val  foo ,= true : bool endof foo$", ":;data(foo bool (1bool))", spec);
  TEST_PARSE( "val  foo ,= 42   :      endof foo$", ":;data(foo int (42))", spec);
  TEST_PARSE( "val  foo ,= 42          endof foo$", ":;data(foo int (42))", spec);

  TEST_PARSE( "var  foo : int  ,= 42   $"         , ":;data(foo mut-int (42))", spec);
  TEST_PARSE( "var  foo : bool ,= true $"         , ":;data(foo mut-bool (1bool))", spec);
  TEST_PARSE( "var  foo :      ,= 42   $"         , ":;data(foo mut-int (42))", spec);
  TEST_PARSE( "var  foo        ,= 42   $"         , ":;data(foo mut-int (42))", spec);
  TEST_PARSE( "var( foo : int  ,= 42   )"         , ":;data(foo mut-int (42))", spec);
  TEST_PARSE( "var( foo : bool ,= true )"         , ":;data(foo mut-bool (1bool))", spec);
  TEST_PARSE( "var( foo :      ,= 42   )"         , ":;data(foo mut-int (42))", spec);
  TEST_PARSE( "var( foo        ,= 42   )"         , ":;data(foo mut-int (42))", spec);
  TEST_PARSE( "var  foo : int  ,= 42   end"       , ":;data(foo mut-int (42))", spec);
  TEST_PARSE( "var  foo : bool ,= true end"       , ":;data(foo mut-bool (1bool))", spec);
  TEST_PARSE( "var  foo :      ,= 42   end"       , ":;data(foo mut-int (42))", spec);
  TEST_PARSE( "var  foo        ,= 42   end"       , ":;data(foo mut-int (42))", spec);
  TEST_PARSE( "var  foo : int  ,= 42   endof foo$", ":;data(foo mut-int (42))", spec);
  TEST_PARSE( "var  foo : bool ,= true endof foo$", ":;data(foo mut-bool (1bool))", spec);
  TEST_PARSE( "var  foo :      ,= 42   endof foo$", ":;data(foo mut-int (42))", spec);
  TEST_PARSE( "var  foo        ,= 42   endof foo$", ":;data(foo mut-int (42))", spec);

  spec = "trivial example with implicit init value";
  //Toggling 1) 'keyword...;' vs 'keyword(...)' vs 'keyword...end' vs
  //'keyword...endof...$' syntax
  //Toggling 2) val vs var
  TEST_PARSE( "val  foo :int $"         , ":;data(foo int ())", spec);
  TEST_PARSE( "val( foo :int )"         , ":;data(foo int ())", spec);
  TEST_PARSE( "val  foo :int end"       , ":;data(foo int ())", spec);
  TEST_PARSE( "val  foo :int endof foo$", ":;data(foo int ())", spec);
  TEST_PARSE( "var  foo :int $"         , ":;data(foo mut-int ())", spec);
  TEST_PARSE( "var( foo :int )"         , ":;data(foo mut-int ())", spec);
  TEST_PARSE( "var  foo :int endof foo$", ":;data(foo mut-int ())", spec);

  spec = "trivial example with constructor call style initializer";
  TEST_PARSE( "val foo(=42) : int      $", ":;data(foo int (42))", spec);
  TEST_PARSE( "val foo(=42) :          $", ":;data(foo int (42))", spec);
  TEST_PARSE( "val foo(=42)            $", ":;data(foo int (42))", spec);
  TEST_PARSE( "val foo      : int (=42)$", ":;data(foo int (42))", spec);
  TEST_PARSE( "val foo      :     (=42)$", ":;data(foo int (42))", spec);
  TEST_PARSE( "val foo            (=42)$", ":;data(foo int (42))", spec);

  spec = "initializer is the empty 'ctor call' argument list";
  TEST_PARSE( "val foo(=) : int    $", ":;data(foo int ())", spec);
  TEST_PARSE( "val foo    : int (=)$", ":;data(foo int ())", spec);

  spec = "initializer is in 'ctor call' style with 2+ arguments. "
    "Here it's about parsing, the fact that the type 'int' does not expect "
    "two arguments is irrelevant here.";
  TEST_PARSE( "val foo (=42,77) :int         $", ":;data(foo int (42 77))", spec);
  TEST_PARSE( "val foo          :int (=42,77)$", ":;data(foo int (42 77))", spec);

  spec = "muttable qualifier";
  TEST_PARSE( "val foo :mut int$", ":;data(foo mut-int ())", spec);

  spec = "(process) static storage duration";
  TEST_PARSE( "val foo :int is static$", ":;data(foo static/int ())", spec);

  spec = "short version with implicit type";
  TEST_PARSE( "foo:=42", ":;data(foo int (42))", spec);

  spec = "noinit initializer";
  TEST_PARSE( "val foo :int ,= noinit$", ":;data(foo int noinit)", spec);
  TEST_PARSE( "val foo = noinit :int$", ":;data(foo int noinit)", spec);
}

// note that it is a semantic error, not a grammar error
TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    a_data_definition_WITH_multiple_same_arg,
    scanAndParse,
    reports_eSameArgWasDefinedMultipleTimes) ) {
  TEST_PARSE_REPORTS_ERROR(
    "val foo ,=42 :int ,=42$", Error::eSameArgWasDefinedMultipleTimes, "");
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    an_ifelse_flow_control_expression,
    scanAndParse,
    succeeds_AND_returns_correct_AST) ) {

  // common used variations
  TEST_PARSE( "if  x \n   1        $"  , ":;:if(x :;1)", "");
  TEST_PARSE( "if  x :    1        $"  , ":;:if(x :;1)", "");
  TEST_PARSE( "if  x then 1        end", ":;:if(x :;1)", "");
  TEST_PARSE( "if( x :    1        )"  , ":;:if(x :;1)", "");
  TEST_PARSE( "if  x \n   1 else 2 $"  , ":;:if(x :;1 :;2)", "");
  TEST_PARSE( "if  x :    1 else 2 $"  , ":;:if(x :;1 :;2)", "");
  TEST_PARSE( "if  x then 1 else 2 end", ":;:if(x :;1 :;2)", "");
  TEST_PARSE( "if( x :    1 :    2 )"  , ":;:if(x :;1 :;2)", "");

  // toggling 1) no elif part, one elif part, two elif parts
  // toggling 2) without/with else part
  // toggling 3) syntax 'if...;' vs 'if(...)' vs 'if...end if;' 
  // toggling 4) with/without colon after condition
  TEST_PARSE( "if  x: 1                                    $", ":;:if(x :;1)", "");
  TEST_PARSE( "if  x\n1                                    $", ":;:if(x :;1)", "");
  TEST_PARSE( "if( x: 1                                    )", ":;:if(x :;1)", "");
  TEST_PARSE( "if( x\n1                                    )", ":;:if(x :;1)", "");
  TEST_PARSE( "if  x: 1                            endof if$", ":;:if(x :;1)", "");
  TEST_PARSE( "if  x\n1                            endof if$", ":;:if(x :;1)", "");
  TEST_PARSE( "if  x: 1                     else 2         $", ":;:if(x :;1 :;2)", "");
  TEST_PARSE( "if  x\n1                     else 2         $", ":;:if(x :;1 :;2)", "");
  TEST_PARSE( "if( x: 1                     :    2         )", ":;:if(x :;1 :;2)", "");
  TEST_PARSE( "if( x\n1                     :    2         )", ":;:if(x :;1 :;2)", "");
  TEST_PARSE( "if  x: 1                     else 2 endof if$", ":;:if(x :;1 :;2)", "");
  TEST_PARSE( "if  x\n1                     else 2 endof if$", ":;:if(x :;1 :;2)", "");
  TEST_PARSE( "if  x: 1 elif y: 2                          $", ":;:if(x :;1 :if(y :;2))", "");
  TEST_PARSE( "if  x\n1 elif y\n2                          $", ":;:if(x :;1 :if(y :;2))", "");
  TEST_PARSE( "if( x: 1 elif y: 2                          )", ":;:if(x :;1 :if(y :;2))", "");
  TEST_PARSE( "if( x\n1 elif y\n2                          )", ":;:if(x :;1 :if(y :;2))", "");
  TEST_PARSE( "if  x: 1 elif y: 2                  endof if$", ":;:if(x :;1 :if(y :;2))", "");
  TEST_PARSE( "if  x\n1 elif y\n2                  endof if$", ":;:if(x :;1 :if(y :;2))", "");
  TEST_PARSE( "if  x: 1 elif y: 2           else 3         $", ":;:if(x :;1 :if(y :;2 :;3))", "");
  TEST_PARSE( "if  x\n1 elif y\n2           else 3         $", ":;:if(x :;1 :if(y :;2 :;3))", "");
  TEST_PARSE( "if( x: 1 elif y: 2           :    3         )", ":;:if(x :;1 :if(y :;2 :;3))", "");
  TEST_PARSE( "if( x\n1 elif y\n2           :    3         )", ":;:if(x :;1 :if(y :;2 :;3))", "");
  TEST_PARSE( "if  x: 1 elif y: 2           else 3 endof if$", ":;:if(x :;1 :if(y :;2 :;3))", "");
  TEST_PARSE( "if  x\n1 elif y\n2           else 3 endof if$", ":;:if(x :;1 :if(y :;2 :;3))", "");
  TEST_PARSE( "if  x: 1 elif y: 2 elif z: 3                $", ":;:if(x :;1 :if(y :;2 :if(z :;3)))", "");
  TEST_PARSE( "if  x\n1 elif y\n2 elif z\n3                $", ":;:if(x :;1 :if(y :;2 :if(z :;3)))", "");
  TEST_PARSE( "if( x: 1 elif y: 2 elif z: 3                )", ":;:if(x :;1 :if(y :;2 :if(z :;3)))", "");
  TEST_PARSE( "if( x\n1 elif y\n2 elif z\n3                )", ":;:if(x :;1 :if(y :;2 :if(z :;3)))", "");
  TEST_PARSE( "if  x: 1 elif y: 2 elif z: 3        endof if$", ":;:if(x :;1 :if(y :;2 :if(z :;3)))", "");
  TEST_PARSE( "if  x\n1 elif y\n2 elif z\n3        endof if$", ":;:if(x :;1 :if(y :;2 :if(z :;3)))", "");
  TEST_PARSE( "if  x: 1 elif y: 2 elif z: 3 else 4         $", ":;:if(x :;1 :if(y :;2 :if(z :;3 :;4)))", "");
  TEST_PARSE( "if  x\n1 elif y\n2 elif z\n3 else 4         $", ":;:if(x :;1 :if(y :;2 :if(z :;3 :;4)))", "");
  TEST_PARSE( "if( x: 1 elif y: 2 elif z: 3 :    4         )", ":;:if(x :;1 :if(y :;2 :if(z :;3 :;4)))", "");
  TEST_PARSE( "if( x\n1 elif y\n2 elif z\n3 :    4         )", ":;:if(x :;1 :if(y :;2 :if(z :;3 :;4)))", "");
  TEST_PARSE( "if  x: 1 elif y: 2 elif z: 3 else 4 endof if$", ":;:if(x :;1 :if(y :;2 :if(z :;3 :;4)))", "");
  TEST_PARSE( "if  x\n1 elif y\n2 elif z\n3 else 4 endof if$", ":;:if(x :;1 :if(y :;2 :if(z :;3 :;4)))", "");
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    an_loop_control_expression,
    scanAndParse,
    succeeds_AND_returns_correct_AST) ) {
  TEST_PARSE( "while  x :  y $"  , ":;:while(x :;y)", "");
  TEST_PARSE( "while  x do y end", ":;:while(x :;y)", "");
  TEST_PARSE( "while( x :  y )"  , ":;:while(x :;y)", "");
}
