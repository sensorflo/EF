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
  AstValue* actualAst = NULL;
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
    SCOPED_TRACE("testParse called from here (via TEST_PARSE)"); \
    testParse(efProgram, expectedAst, spec);    \
  }

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    a_literal,
    parse,
    succeeds_AND_returns_correct_AST) ) {
  TEST_PARSE( "42", "42", "trivial example");
  TEST_PARSE( "false", "0bool", "trivial example");
  TEST_PARSE( "true", "1bool", "trivial example");
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    a_cast_expression,
    parse,
    succeeds_AND_returns_correct_AST) ) {
  TEST_PARSE( "int(false)", "cast(int 0bool)", "trivial example");
  TEST_PARSE( "bool(0)", "cast(bool 0)", "trivial example");
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    a_math_expression,
    parse,
    succeeds_AND_returns_correct_AST) ) {
  TEST_PARSE( "42 + 77", "+(42 77)", "trivial example");
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


  // precedence level group: function call
  string spec = "function call () is right associative";
  TEST_PARSE( "foo(a)(b)", "foo(a)(b)", spec);

  // precedence level group: unary prefix not
  spec = "! aka 'not' is right associative. ! and 'not' are synonyms";
  TEST_PARSE( "not not a", "not(not(a))", spec);
  TEST_PARSE( "not !   a", "not(not(a))", spec);
  TEST_PARSE( "!   not a", "not(not(a))", spec);
  TEST_PARSE( "!   !   a", "not(not(a))", spec);

  spec = "! has lower precedence than function call ()";
  TEST_PARSE( "!foo()", "not(foo())", spec);

  // precedence level group: binary * /
  spec = "* is left associative";
  TEST_PARSE( "a*b*c", "*(*(a b) c)", spec);

  spec = "* has lower precedence than !";
  TEST_PARSE( "!a *  b", "*(not(a) b)", spec);
  TEST_PARSE( " a * !b", "*(a not(b))", spec);

  spec = "/ is left associative";
  TEST_PARSE( "a/b/c", "/(/(a b) c)", spec);

  spec = "/ has same precedence as *";
  TEST_PARSE( "a/b*c", "*(/(a b) c)", spec);
  TEST_PARSE( "a*b/c", "/(*(a b) c)", spec);

  // precedence level group: binary + -
  spec = "+ is left associative";
  TEST_PARSE( "a+b+c", "+(+(a b) c)", spec);

  spec = "+ has lower precedence than *";
  TEST_PARSE( "a+b*c", "+(a *(b c))", spec);
  TEST_PARSE( "a*b+c", "+(*(a b) c)", spec);

  spec = "- is left associative";
  TEST_PARSE( "a-b-c", "-(-(a b) c)", spec);

  spec = "- has same precedence as +";
  TEST_PARSE( "a-b+c", "+(-(a b) c)", spec);
  TEST_PARSE( "a+b-c", "-(+(a b) c)", spec);

  // precedence level group: ==
  spec = "== is left associative";
  TEST_PARSE( "a==b==c", "==(==(a b) c)", spec);
  
  spec = "== has lower precedence than +";
  TEST_PARSE( "a==b+c", "==(a +(b c))", spec);
  TEST_PARSE( "a+b==c", "==(+(a b) c)", spec);

  // precedence level group: binary and &&
  spec = "&& aka 'and' is left associative. && and 'and' are synonyms.";
  TEST_PARSE( "a &&  b &&  c", "and(and(a b) c)", spec);
  TEST_PARSE( "a &&  b and c", "and(and(a b) c)", spec);
  TEST_PARSE( "a and b &&  c", "and(and(a b) c)", spec);
  TEST_PARSE( "a and b and c", "and(and(a b) c)", spec);

  spec = "&& aka 'and' has lower precedence than +";
  TEST_PARSE( "a &&  b +   c", "and(a +(b c))", spec);
  TEST_PARSE( "a and b +   c", "and(a +(b c))", spec);
  TEST_PARSE( "a +   b &&  c", "and(+(a b) c)", spec);
  TEST_PARSE( "a +   b and c", "and(+(a b) c)", spec);

  // precedence level group: binary or ||
  spec = "|| aka 'or' is left associative. || and 'or' are synonyms.";
  TEST_PARSE( "a || b || c", "or(or(a b) c)", spec);
  TEST_PARSE( "a || b or c", "or(or(a b) c)", spec);
  TEST_PARSE( "a or b || c", "or(or(a b) c)", spec);
  TEST_PARSE( "a or b or c", "or(or(a b) c)", spec);

  spec = "|| aka 'or' has lower precedence than && aka 'and'";
  TEST_PARSE( "a ||  b &&  c", "or(a and(b c))", spec);
  TEST_PARSE( "a ||  b and c", "or(a and(b c))", spec);
  TEST_PARSE( "a or  b &&  c", "or(a and(b c))", spec);
  TEST_PARSE( "a or  b and c", "or(a and(b c))", spec);
  TEST_PARSE( "a &&  b ||  c", "or(and(a b) c)", spec);
  TEST_PARSE( "a &&  b or  c", "or(and(a b) c)", spec);
  TEST_PARSE( "a and b ||  c", "or(and(a b) c)", spec);
  TEST_PARSE( "a and b or  c", "or(and(a b) c)", spec);

  // precedence level group: binrary = :=
  spec = "= is right associative";
  TEST_PARSE( "a = b = c", "=(a =(b c))", spec);

  spec = ":= is right associative";
  TEST_PARSE( "a := b := c", "data(decldata(a int) (data(decldata(b int) (c))))", spec);

  spec = "= has lower precedence than || aka 'or'";
  TEST_PARSE( "a =  b or c", "=(a or(b c))", spec);
  TEST_PARSE( "a or b =  c", "=(or(a b) c)", spec);

  spec = ":= has same precedence as =";
  TEST_PARSE( "a := b =  c", "data(decldata(a int) (=(b c)))", spec);
  TEST_PARSE( "a =  b := c", "=(a data(decldata(b int) (c)))", spec);

  // precedence level group: binary sequence operator (; or adjancted standalone expression)
  spec = "sequence operator (; or adjancted standalone expression) is left associative";
  TEST_PARSE( "a;b;c", ";(a b c)", spec);
  TEST_PARSE( "a;b c", ";(a b c)", spec);
  TEST_PARSE( "a b;c", ";(a b c)", spec);
  TEST_PARSE( "a b c", ";(a b c)", spec);

  spec = "sequence operator (; or adjancted standalone expression) has lower precedence than =";
  TEST_PARSE( "a ; b = c", ";(a =(b c))", spec);
  TEST_PARSE( "a   b = c", ";(a =(b c))", spec);
  TEST_PARSE( "a = b ; c", ";(=(a b) c)", spec);
  TEST_PARSE( "a = b   c", ";(=(a b) c)", spec);
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    a_math_expression_containing_brace_grouping,
    parse,
    succeeds_AND_returns_correct_AST) ) {
  TEST_PARSE( "g(1+2)*g(3-4)/5", "/(*(+(1 2) -(3 4)) 5)",
    "g(...) group: 1) overwrites precedence and 2) turns sub_expr into exp");
  TEST_PARSE( "g(1 2)", ";(1 2)",
    "g(...) group can contain not only a sub_expr but also an expr");
  TEST_PARSE( "g(1 2)*3", "*(;(1 2) 3)",
    "g(...) group can contain not only a sub_expr but also an expr");
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    an_operator_in_call_syntax,
    parse,
    succeeds_AND_returns_correct_AST) ) {
  TEST_PARSE( "!(x)", "not(x)", "");
  TEST_PARSE( "not(x)", "not(x)", "");
  TEST_PARSE( "and(x,y,z)", "and(x y z)", "");
  TEST_PARSE( "&&(x,y,z)", "and(x y z)", "");
  TEST_PARSE( "or(x,y,z)", "or(x y z)", "");
  TEST_PARSE( "||(x,y,z)", "or(x y z)", "");
  TEST_PARSE( "==(1,1)", "==(1 1)", "");
  TEST_PARSE( "+()", "+()", "");
  TEST_PARSE( "-(1)", "-(1)", "");
  TEST_PARSE( "*(1,2)", "*(1 2)", "");
  TEST_PARSE( "/(1,2,3)", "/(1 2 3)", "");
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    an_assignement_expression,
    parse,
    succeeds_AND_returns_correct_AST) ) {
  TEST_PARSE( "foo = 42", "=(foo 42)", "trivial example");
  TEST_PARSE( "foo = 1+2*3", "=(foo +(1 *(2 3)))", "simple example");
  TEST_PARSE( "foo = bar = 42", "=(foo =(bar 42))", "= is right associative");
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    a_function_declaration,
    parse,
    succeeds_AND_returns_correct_AST) ) {
  string spec = "example with zero arguments and explicit return type";
  //Toggling 1) 'keyword...;' vs 'keyword(...)' vs 'keyword...end...;' syntax
  TEST_PARSE( "decl fun foo:() int$"         , "declfun(foo () int)", spec);
  TEST_PARSE( "decl(fun foo:() int)"         , "declfun(foo () int)", spec);
  TEST_PARSE( "decl fun foo:() int end foo$" , "declfun(foo () int)", spec);

  spec = "example with one argument and explicity return type";
  //Toggling 1) 'keyword...;' vs 'keyword(...)' syntax
  TEST_PARSE( "decl fun foo:(arg1:int) int$", "declfun(foo ((arg1 int-mut)) int)", spec);
  TEST_PARSE( "decl(fun foo:(arg1:int) int)", "declfun(foo ((arg1 int-mut)) int)", spec);

  spec = "example with two arguments and explicity return type\n";
  //Toggling 1) 'keyword...;' vs 'keyword(...)' syntax
  TEST_PARSE( "decl fun foo:(arg1:int, arg2:int) int$", "declfun(foo ((arg1 int-mut) (arg2 int-mut)) int)", spec);
  TEST_PARSE( "decl(fun foo:(arg1:int, arg2:int) int)", "declfun(foo ((arg1 int-mut) (arg2 int-mut)) int)", spec);

  spec = "example with implicit return type";
  //Toggling 1) zero vs one parameter
  //Toggling 2) 'keyword...;' vs 'keyword(...)' syntax
  TEST_PARSE( "decl fun foo:          $", "declfun(foo () int)", spec);
  TEST_PARSE( "decl fun foo:(arg1:int)$", "declfun(foo ((arg1 int-mut)) int)", spec);
  TEST_PARSE( "decl(fun foo:          )", "declfun(foo () int)", spec);
  TEST_PARSE( "decl(fun foo:(arg1:int))", "declfun(foo ((arg1 int-mut)) int)", spec);

  spec = "example with zero arguments, no parantheses around (empty) arg list and implicit return type";
  TEST_PARSE( "decl fun foo$", "declfun(foo () int)", spec);

  spec = "should allow trailing comma in argument list";
  //Toggling 1) optional return type
  TEST_PARSE( "decl fun foo:(arg1:int,) int$", "declfun(foo ((arg1 int-mut)) int)", spec);
  TEST_PARSE( "decl fun foo:(arg1:int,)    $", "declfun(foo ((arg1 int-mut)) int)", spec);
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    a_reference_to_a_symbol_within_a_function_definition,
    parse,
    succeeds_AND_returns_correct_AST) ) {
  string spec = "example with one argument and a body just returning the arg";
  TEST_PARSE( "fun foo: (x:int) int = x$", "fun(declfun(foo ((x int-mut)) int) x)", spec);
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    a_function_definition,
    parse,
    succeeds_AND_returns_correct_AST) ) {

  string spec = "example with zero arguments and trivial body";
  TEST_PARSE( "fun foo:        = 42$"        , "fun(declfun(foo () int) 42)", spec);
  TEST_PARSE( "fun foo: ()     = 42$"        , "fun(declfun(foo () int) 42)", spec);
  TEST_PARSE( "fun foo: () int = 42$"        , "fun(declfun(foo () int) 42)", spec);
  TEST_PARSE( "fun(foo: () int = 42)"        , "fun(declfun(foo () int) 42)", spec);
  TEST_PARSE( "fun foo: () int = 42 end foo$", "fun(declfun(foo () int) 42)", spec);

  spec = "example with zero arguments and simple but not trivial body";
  TEST_PARSE( "fun foo:        = 42 1+2$", "fun(declfun(foo () int) ;(42 +(1 2)))", spec);
  TEST_PARSE( "fun foo: ()     = 42 1+2$", "fun(declfun(foo () int) ;(42 +(1 2)))", spec);
  TEST_PARSE( "fun foo: () int = 42 1+2$", "fun(declfun(foo () int) ;(42 +(1 2)))", spec);
  TEST_PARSE( "fun(foo: () int = 42 1+2)", "fun(declfun(foo () int) ;(42 +(1 2)))", spec);

  spec = "example with one argument and trivial body";
  TEST_PARSE( "fun foo: (arg1:int)     = 42$", "fun(declfun(foo ((arg1 int-mut)) int) 42)", spec);
  TEST_PARSE( "fun foo: (arg1:int)     = 42$", "fun(declfun(foo ((arg1 int-mut)) int) 42)", spec);
  TEST_PARSE( "fun foo: (arg1:int) int = 42$", "fun(declfun(foo ((arg1 int-mut)) int) 42)", spec);
  TEST_PARSE( "fun(foo: (arg1:int) int = 42)", "fun(declfun(foo ((arg1 int-mut)) int) 42)", spec);

  spec = "should allow trailing comma in argument list";
  TEST_PARSE( "fun foo: (arg1:int,)        = 42$", "fun(declfun(foo ((arg1 int-mut)) int) 42)", spec);
  TEST_PARSE( "fun foo: (arg1:int,)        = 42$", "fun(declfun(foo ((arg1 int-mut)) int) 42)", spec);

  spec = "example with two arguments and trivial body";
  TEST_PARSE( "fun foo: (arg1:int, arg2:int)     = 42$", "fun(declfun(foo ((arg1 int-mut) (arg2 int-mut)) int) 42)", spec);
  TEST_PARSE( "fun foo: (arg1:int, arg2:int)     = 42$", "fun(declfun(foo ((arg1 int-mut) (arg2 int-mut)) int) 42)", spec);
  TEST_PARSE( "fun foo: (arg1:int, arg2:int) int = 42$", "fun(declfun(foo ((arg1 int-mut) (arg2 int-mut)) int) 42)", spec);
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    a_function_call,
    parse,
    succeeds_AND_returns_correct_AST) ) {

  string spec = "trivial example";
  TEST_PARSE( "foo()", "foo()", spec);
  TEST_PARSE( "foo(42)", "foo(42)", spec);
  TEST_PARSE( "foo(42,77)", "foo(42 77)", spec);

  spec = "Function address can be given by an expression";
  TEST_PARSE( "g(foo+bar)(42,77)", "+(foo bar)(42 77)", spec);
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    a_data_definition,
    parse,
    succeeds_AND_returns_correct_AST) ) {

  string spec = "trivial example with explicit init value";
  //Toggling 1) type defined in 3 variants: a) explictit with ':int'
  //            b) explictit with ':bool' c) implicit with ':'
  //            d) implicit with ''  
  //Toggling 2) 'keyword...;' vs 'keyword(...)' vs 'keyword...end...;' syntax
  //Toggling 3) initializer behind id vs initializer behind type
  //Toggling 4) val vs var
  TEST_PARSE( "val foo: int  = 42$"          , "data(decldata(foo int) (42))", spec);
  TEST_PARSE( "val foo: bool = true$"        , "data(decldata(foo bool) (1bool))", spec);
  TEST_PARSE( "val foo:      = 42$"          , "data(decldata(foo int) (42))", spec);
  TEST_PARSE( "val foo       = 42$"          , "data(decldata(foo int) (42))", spec);
  TEST_PARSE( "val(foo: int  = 42)"          , "data(decldata(foo int) (42))", spec);
  TEST_PARSE( "val(foo: bool = true)"        , "data(decldata(foo bool) (1bool))", spec);
  TEST_PARSE( "val(foo:      = 42)"          , "data(decldata(foo int) (42))", spec);
  TEST_PARSE( "val(foo       = 42)"          , "data(decldata(foo int) (42))", spec);
  TEST_PARSE( "val foo: int  = 42 end foo$"  , "data(decldata(foo int) (42))", spec);
  TEST_PARSE( "val foo: bool = true end foo$", "data(decldata(foo bool) (1bool))", spec);
  TEST_PARSE( "val foo:      = 42 end foo$"  , "data(decldata(foo int) (42))", spec);
  TEST_PARSE( "val foo       = 42 end foo$"  , "data(decldata(foo int) (42))", spec);

  TEST_PARSE( "val foo = 42: int $"          , "data(decldata(foo int) (42))", spec);
  TEST_PARSE( "val foo = true: bool$"        , "data(decldata(foo bool) (1bool))", spec);
  TEST_PARSE( "val foo = 42:     $"          , "data(decldata(foo int) (42))", spec);
  TEST_PARSE( "val foo = 42      $"          , "data(decldata(foo int) (42))", spec);
  TEST_PARSE( "val(foo = 42: int )"          , "data(decldata(foo int) (42))", spec);
  TEST_PARSE( "val(foo = true: bool)"        , "data(decldata(foo bool) (1bool))", spec);
  TEST_PARSE( "val(foo = 42:     )"          , "data(decldata(foo int) (42))", spec);
  TEST_PARSE( "val(foo = 42      )"          , "data(decldata(foo int) (42))", spec);
  TEST_PARSE( "val foo = 42: int  end foo$"  , "data(decldata(foo int) (42))", spec);
  TEST_PARSE( "val foo = true: bool end foo$", "data(decldata(foo bool) (1bool))", spec);
  TEST_PARSE( "val foo = 42:      end foo$"  , "data(decldata(foo int) (42))", spec);
  TEST_PARSE( "val foo = 42       end foo$"  , "data(decldata(foo int) (42))", spec);

  TEST_PARSE( "var foo: int  = 42$"          , "data(decldata(foo int-mut) (42))", spec);
  TEST_PARSE( "var foo: bool = true$"        , "data(decldata(foo bool-mut) (1bool))", spec);
  TEST_PARSE( "var foo:      = 42$"          , "data(decldata(foo int-mut) (42))", spec);
  TEST_PARSE( "var foo       = 42$"          , "data(decldata(foo int-mut) (42))", spec);
  TEST_PARSE( "var(foo: int  = 42)"          , "data(decldata(foo int-mut) (42))", spec);
  TEST_PARSE( "var(foo: bool = true)"        , "data(decldata(foo bool-mut) (1bool))", spec);
  TEST_PARSE( "var(foo:      = 42)"          , "data(decldata(foo int-mut) (42))", spec);
  TEST_PARSE( "var(foo       = 42)"          , "data(decldata(foo int-mut) (42))", spec);
  TEST_PARSE( "var foo: int  = 42 end foo$"  , "data(decldata(foo int-mut) (42))", spec);
  TEST_PARSE( "var foo: bool = true end foo$", "data(decldata(foo bool-mut) (1bool))", spec);
  TEST_PARSE( "var foo:      = 42 end foo$"  , "data(decldata(foo int-mut) (42))", spec);
  TEST_PARSE( "var foo       = 42 end foo$"  , "data(decldata(foo int-mut) (42))", spec);

  TEST_PARSE( "var foo = 42: int $"          , "data(decldata(foo int-mut) (42))", spec);
  TEST_PARSE( "var foo = true: bool$"        , "data(decldata(foo bool-mut) (1bool))", spec);
  TEST_PARSE( "var foo = 42:     $"          , "data(decldata(foo int-mut) (42))", spec);
  TEST_PARSE( "var foo = 42      $"          , "data(decldata(foo int-mut) (42))", spec);
  TEST_PARSE( "var(foo = 42: int )"          , "data(decldata(foo int-mut) (42))", spec);
  TEST_PARSE( "var(foo = true: bool)"        , "data(decldata(foo bool-mut) (1bool))", spec);
  TEST_PARSE( "var(foo = 42:     )"          , "data(decldata(foo int-mut) (42))", spec);
  TEST_PARSE( "var(foo = 42      )"          , "data(decldata(foo int-mut) (42))", spec);
  TEST_PARSE( "var foo = 42: int  end foo$"  , "data(decldata(foo int-mut) (42))", spec);
  TEST_PARSE( "var foo = true: bool end foo$", "data(decldata(foo bool-mut) (1bool))", spec);
  TEST_PARSE( "var foo = 42:      end foo$"  , "data(decldata(foo int-mut) (42))", spec);
  TEST_PARSE( "var foo = 42       end foo$"  , "data(decldata(foo int-mut) (42))", spec);

  spec = "trivial example with implicit init value";
  //Toggling 1) 'keyword...;' vs 'keyword(...)' vs 'keyword...end...;' syntax
  //Toggling 2) val vs var
  TEST_PARSE( "val foo:int$"        , "data(decldata(foo int) ())", spec);
  TEST_PARSE( "val(foo:int)"        , "data(decldata(foo int) ())", spec);
  TEST_PARSE( "val foo:int end foo$", "data(decldata(foo int) ())", spec);
  TEST_PARSE( "var foo:int$"        , "data(decldata(foo int-mut) ())", spec);
  TEST_PARSE( "var(foo:int)"        , "data(decldata(foo int-mut) ())", spec);
  TEST_PARSE( "var foo:int end foo$", "data(decldata(foo int-mut) ())", spec);

  spec = "trivial example with constructor call style initializer";
  TEST_PARSE( "val foo=(42) : int      $", "data(decldata(foo int) (42))", spec);
  TEST_PARSE( "val foo=(42) :          $", "data(decldata(foo int) (42))", spec);
  TEST_PARSE( "val foo=(42)            $", "data(decldata(foo int) (42))", spec);
  TEST_PARSE( "val foo      : int =(42)$", "data(decldata(foo int) (42))", spec);
  TEST_PARSE( "val foo      :     =(42)$", "data(decldata(foo int) (42))", spec);
  TEST_PARSE( "val foo            =(42)$", "data(decldata(foo int) (42))", spec);

  spec = "initializer is the empty 'ctor call' argument list";
  TEST_PARSE( "val foo=() : int    $", "data(decldata(foo int) ())", spec);
  TEST_PARSE( "val foo    : int =()$", "data(decldata(foo int) ())", spec);

  spec = "initializer is in 'ctor call' style with 2+ arguments. "
    "Here it's about parsing, the fact that the type 'int' does not expect "
    "two arguments is irrelevant here.";
  TEST_PARSE( "val foo =(42,77) :int         $", "data(decldata(foo int) (42 77))", spec);
  TEST_PARSE( "val foo          :int =(42,77)$", "data(decldata(foo int) (42 77))", spec);

  spec = "short version with implicit type";
  TEST_PARSE( "foo:=42", "data(decldata(foo int) (42))", spec);
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    an_ifelse_flow_control_expression,
    parse,
    succeeds_AND_returns_correct_AST) ) {
  // toggling 1) no elif part, one elif part, two elif parts
  // toggling 2) without/with else part
  // toggling 3) syntax 'if...;' vs 'if(...)' vs 'if...end if;' 
  // toggling 4) with/without colon after condition
  TEST_PARSE( "if  x: 1                                  $", "if(x 1)", "");
  TEST_PARSE( "if  x  1                                  $", "if(x 1)", "");
  TEST_PARSE( "if( x: 1                                  )", "if(x 1)", "");
  TEST_PARSE( "if( x  1                                  )", "if(x 1)", "");
  TEST_PARSE( "if  x: 1                            end if$", "if(x 1)", "");
  TEST_PARSE( "if  x  1                            end if$", "if(x 1)", "");
  TEST_PARSE( "if  x: 1                     else 2       $", "if(x 1 2)", "");
  TEST_PARSE( "if  x  1                     else 2       $", "if(x 1 2)", "");
  TEST_PARSE( "if( x: 1                     else 2       )", "if(x 1 2)", "");
  TEST_PARSE( "if( x  1                     else 2       )", "if(x 1 2)", "");
  TEST_PARSE( "if  x: 1                     else 2 end if$", "if(x 1 2)", "");
  TEST_PARSE( "if  x  1                     else 2 end if$", "if(x 1 2)", "");
  TEST_PARSE( "if  x: 1 elif y: 2                        $", "if(x 1 y 2)", "");
  TEST_PARSE( "if  x  1 elif y  2                        $", "if(x 1 y 2)", "");
  TEST_PARSE( "if( x: 1 elif y: 2                        )", "if(x 1 y 2)", "");
  TEST_PARSE( "if( x  1 elif y  2                        )", "if(x 1 y 2)", "");
  TEST_PARSE( "if  x: 1 elif y: 2                  end if$", "if(x 1 y 2)", "");
  TEST_PARSE( "if  x  1 elif y  2                  end if$", "if(x 1 y 2)", "");
  TEST_PARSE( "if  x: 1 elif y: 2           else 3       $", "if(x 1 y 2 3)", "");
  TEST_PARSE( "if  x  1 elif y  2           else 3       $", "if(x 1 y 2 3)", "");
  TEST_PARSE( "if( x: 1 elif y: 2           else 3       )", "if(x 1 y 2 3)", "");
  TEST_PARSE( "if( x  1 elif y  2           else 3       )", "if(x 1 y 2 3)", "");
  TEST_PARSE( "if  x: 1 elif y: 2           else 3 end if$", "if(x 1 y 2 3)", "");
  TEST_PARSE( "if  x  1 elif y  2           else 3 end if$", "if(x 1 y 2 3)", "");
  TEST_PARSE( "if  x: 1 elif y: 2 elif z: 3              $", "if(x 1 y 2 z 3)", "");
  TEST_PARSE( "if  x  1 elif y  2 elif z  3              $", "if(x 1 y 2 z 3)", "");
  TEST_PARSE( "if( x: 1 elif y: 2 elif z: 3              )", "if(x 1 y 2 z 3)", "");
  TEST_PARSE( "if( x  1 elif y  2 elif z  3              )", "if(x 1 y 2 z 3)", "");
  TEST_PARSE( "if  x: 1 elif y: 2 elif z: 3        end if$", "if(x 1 y 2 z 3)", "");
  TEST_PARSE( "if  x  1 elif y  2 elif z  3        end if$", "if(x 1 y 2 z 3)", "");
  TEST_PARSE( "if  x: 1 elif y: 2 elif z: 3 else 4       $", "if(x 1 y 2 z 3 4)", "");
  TEST_PARSE( "if  x  1 elif y  2 elif z  3 else 4       $", "if(x 1 y 2 z 3 4)", "");
  TEST_PARSE( "if( x: 1 elif y: 2 elif z: 3 else 4       )", "if(x 1 y 2 z 3 4)", "");
  TEST_PARSE( "if( x  1 elif y  2 elif z  3 else 4       )", "if(x 1 y 2 z 3 4)", "");
  TEST_PARSE( "if  x: 1 elif y: 2 elif z: 3 else 4 end if$", "if(x 1 y 2 z 3 4)", "");
  TEST_PARSE( "if  x  1 elif y  2 elif z  3 else 4 end if$", "if(x 1 y 2 z 3 4)", "");
}

