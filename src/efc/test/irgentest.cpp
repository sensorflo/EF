#include "test.h"
#include "../irgen.h"
#include "../parserext.h"
#include "../ast.h"
#include "../env.h"
#include "../errorhandler.h"
#include "llvm/IR/Module.h"
#include <memory>
using namespace testing;
using namespace std;
using namespace llvm;

class TestingIrGen : public IrGen {
public:
  TestingIrGen() : IrGen(
    *(m_env = new Env()),
    *(m_errorHandler = new ErrorHandler()) ) {};
  ~TestingIrGen() { delete m_env; delete m_errorHandler; };
  using IrGen::jitExecFunction;
  using IrGen::jitExecFunction1Arg;
  using IrGen::jitExecFunction2Arg;
  using IrGen::m_module;
  Env* m_env;
  ErrorHandler* m_errorHandler;
};

void testbuilAndRunModule(TestingIrGen& UUT, AstValue* astRoot,
  int expectedResult, const string& spec = "") {

  // setup
  ENV_ASSERT_TRUE( astRoot!=NULL );
  auto_ptr<AstValue> astRootAp(astRoot);

  // execute
  int result = UUT.buildAndRunModule(*astRoot);

  // verify
  EXPECT_EQ(expectedResult, result) << amendSpec(spec) << amendAst(astRoot);
}

#define TEST_BUILD_AND_RUN_MODULE(astRoot, expectedResult, spec)        \
  {                                                                     \
    SCOPED_TRACE("testbuilAndRunModule called from here (via TEST_BUILD_AND_RUN_MODULE)"); \
    TestingIrGen UUT;                                            \
    ParserExt pe(*UUT.m_env, *UUT.m_errorHandler);                      \
    testbuilAndRunModule(UUT, astRoot, expectedResult, spec);           \
  }

void testbuilAndRunModuleThrows(TestingIrGen& UUT, AstValue* astRoot,
  const string& spec = "") {
  // setup
  ENV_ASSERT_TRUE( astRoot!=NULL );
  auto_ptr<AstValue> ast(astRoot);
  bool anyThrow = false;
  string excptionwhat;

  // execute
  try {
    UUT.buildAndRunModule(*ast.get());
  }

  // verify
  catch (BuildError& buildError) {
    ADD_FAILURE() << "Write the test not using the general ...Throws but the "
      "more specific ...ReportsError\n" <<
      amendSpec(spec) << amend(*UUT.m_errorHandler) << amendAst(ast.get());
  }
  catch (exception& e) {
    anyThrow = true;
    excptionwhat = e.what();
  }
  catch (exception* e) {
    anyThrow = true;
    if ( e ) {
      excptionwhat = e->what();
    }
  }
  catch (...) {
    anyThrow = true;
  }
  EXPECT_TRUE(anyThrow) << excptionwhat <<
    amendSpec(spec) << amendAst(ast.get());
}

#define TEST_BUILD_AND_RUN_MODULE_THROWS(astRoot, spec)                 \
  {                                                                     \
    SCOPED_TRACE("testbuilAndRunModuleThrows called from here (via TEST_BUILD_AND_RUN_MODULE_THROWS)"); \
    TestingIrGen UUT;                                            \
    ParserExt pe(*UUT.m_env, *UUT.m_errorHandler);                      \
    testbuilAndRunModuleThrows(UUT, astRoot, spec);                     \
  }

void testbuilModuleReportsError(TestingIrGen& UUT, AstValue* astRoot,
  Error::No expectedErrorNo, bool bNoImplicitMain, const string& spec = "") {

  // setup
  ENV_ASSERT_TRUE( astRoot!=NULL );
  auto_ptr<AstValue> ast(astRoot);
  bool anyThrow = false;
  bool didThrowBuildError = false;
  string excptionwhat;

  // execute
  try {
    if (bNoImplicitMain) {
      UUT.buildModuleNoImplicitMain(*astRoot);
    } else {
      UUT.buildModule(*astRoot);
    }
  }

  // verify
  catch (BuildError& buildError) {
    didThrowBuildError = true;

    const ErrorHandler::Container& errors = UUT.m_errorHandler->errors();
    EXPECT_EQ(1, errors.size()) <<
      "Expecting exactly one error\n" << 
      amendSpec(spec) << amend(*UUT.m_errorHandler) << amendAst(astRoot);
    EXPECT_EQ(expectedErrorNo, errors.front()->no()) <<
      amendSpec(spec) << amend(*UUT.m_errorHandler) << amendAst(astRoot);
  }
  catch (exception& e) {
    anyThrow = true;
    excptionwhat = e.what();
  }
  catch (exception* e) {
    anyThrow = true;
    if ( e ) {
      excptionwhat = e->what();
    }
  }
  catch (...) {
    anyThrow = true;
  }
  EXPECT_TRUE(didThrowBuildError) <<
    amendSpec(spec) << amendAst(astRoot) << amend(*UUT.m_errorHandler) <<
    "\nanyThrow: " << anyThrow <<
    "\nexceptionwhat: " << excptionwhat;
}

#define TEST_BUILD_MODULE_REPORTS_ERROR(astRoot, expectedErrorNo, spec) \
  {                                                                     \
    SCOPED_TRACE("testbuilModuleReportsError called from here (via TEST_BUILD_MODULE_REPORTS_ERROR)"); \
    TestingIrGen UUT;                                            \
    ParserExt pe(*UUT.m_env, *UUT.m_errorHandler);                      \
    testbuilModuleReportsError(UUT, astRoot, expectedErrorNo, false, spec); \
  }

#define TEST_BUILD_MODULE_NO_IMPLICIT_MAIN_REPORTS_ERROR(astRoot, expectedErrorNo, spec) \
  {                                                                     \
    SCOPED_TRACE("testbuilModuleReportsError called from here (via TEST_BUILD_MODULE_NO_IMPLICIT_MAIN_REPORTS_ERROR)"); \
    TestingIrGen UUT;                                            \
    ParserExt pe(*UUT.m_env, *UUT.m_errorHandler);                      \
    testbuilModuleReportsError(UUT, astRoot, expectedErrorNo, true, spec); \
  }

TEST(IrGenTest, MAKE_TEST_NAME(
    a_single_literal,
    buildAndRunModule,
    returns_the_literal_s_value)) {
  TEST_BUILD_AND_RUN_MODULE(new AstNumber(42), 42, "");
}

TEST(IrGenTest, MAKE_TEST_NAME(
    an_operator,
    buildAndRunModule,
    returns_the_result_of_that_operator)) {

  // ; aka sequence
  string spec = "The sequence operator evaluates all arguments and returns the last";
  TEST_BUILD_AND_RUN_MODULE(
    new AstOperator(';', new AstNumber(42)), 42, "");
  TEST_BUILD_AND_RUN_MODULE(
    new AstOperator(';', new AstNumber(11), new AstNumber(22)), 22, "");

  // not
  TEST_BUILD_AND_RUN_MODULE(
    new AstOperator(AstOperator::eNot,
      new AstNumber(0, ObjTypeFunda::eBool)),
    !false, "");
  TEST_BUILD_AND_RUN_MODULE(
    new AstOperator(AstOperator::eNot,
      new AstNumber(1, ObjTypeFunda::eBool)),
    !true, "");

  // and
  TEST_BUILD_AND_RUN_MODULE(
    new AstOperator(AstOperator::eAnd,
      new AstNumber(0, ObjTypeFunda::eBool),
      new AstNumber(0, ObjTypeFunda::eBool)),
    false && false, "");
  TEST_BUILD_AND_RUN_MODULE(
    new AstOperator(AstOperator::eAnd,
      new AstNumber(0, ObjTypeFunda::eBool),
      new AstNumber(1, ObjTypeFunda::eBool)),
    false && true, "");
  TEST_BUILD_AND_RUN_MODULE(
    new AstOperator(AstOperator::eAnd,
      new AstNumber(1, ObjTypeFunda::eBool),
      new AstNumber(0, ObjTypeFunda::eBool)),
    true && false, "");
  TEST_BUILD_AND_RUN_MODULE(
    new AstOperator(AstOperator::eAnd,
      new AstNumber(1, ObjTypeFunda::eBool),
      new AstNumber(1, ObjTypeFunda::eBool)),
    true && true, "");

  // or
  TEST_BUILD_AND_RUN_MODULE(
    new AstOperator(AstOperator::eOr,
      new AstNumber(0, ObjTypeFunda::eBool),
      new AstNumber(0, ObjTypeFunda::eBool)),
    false || false, "");
  TEST_BUILD_AND_RUN_MODULE(
    new AstOperator(AstOperator::eOr,
      new AstNumber(0, ObjTypeFunda::eBool),
      new AstNumber(1, ObjTypeFunda::eBool)),
    false || true, "");
  TEST_BUILD_AND_RUN_MODULE(
    new AstOperator(AstOperator::eOr,
      new AstNumber(1, ObjTypeFunda::eBool),
      new AstNumber(0, ObjTypeFunda::eBool)),
    true || false, "");
  TEST_BUILD_AND_RUN_MODULE(
    new AstOperator(AstOperator::eOr,
      new AstNumber(1, ObjTypeFunda::eBool),
      new AstNumber(1, ObjTypeFunda::eBool)),
    true || true, "");

  // ==
  TEST_BUILD_AND_RUN_MODULE(
    new AstOperator(AstOperator::eEqualTo, new AstNumber(2), new AstNumber(2)),
    2==2, "");
  TEST_BUILD_AND_RUN_MODULE(
    new AstOperator(AstOperator::eEqualTo, new AstNumber(1), new AstNumber(2)),
    1==2, "");

  // + - * /
  TEST_BUILD_AND_RUN_MODULE(
    new AstOperator('+', new AstNumber(1), new AstNumber(2)),
    1+2, "");
  TEST_BUILD_AND_RUN_MODULE(
    new AstOperator('-', new AstNumber(1), new AstNumber(2)),
    1-2, "");
  TEST_BUILD_AND_RUN_MODULE(
    new AstOperator('*', new AstNumber(1), new AstNumber(2)),
    1*2, "");
  TEST_BUILD_AND_RUN_MODULE(
    new AstOperator('/', new AstNumber(6), new AstNumber(3)),
    6/3, "");
}

TEST(IrGenTest, MAKE_TEST_NAME(
    an_AstOperator_like_it_results_from_op_call_syntax_constructs,
    buildAndRunModule,
    returns_the_correct_result)) {

  // such operators are
  // - n-ary

  //null-ary not "!()" is invalid 
  string spec = "null-ary or: or() = false";
  TEST_BUILD_AND_RUN_MODULE(
    new AstOperator(AstOperator::eOr),
    false, spec);
  spec = "null-ary and: and() = true";
  TEST_BUILD_AND_RUN_MODULE(
    new AstOperator(AstOperator::eAnd),
    true, spec);
  spec = "null-ary minus: -() = 0";
  TEST_BUILD_AND_RUN_MODULE(
    new AstOperator('-'),
    0, spec);
  spec = "null-ary minus: +() = 0";
  TEST_BUILD_AND_RUN_MODULE(
    new AstOperator('+'),
    0, spec);
  spec = "null-ary mult: *() = 1";
  TEST_BUILD_AND_RUN_MODULE(
    new AstOperator('*'),
    1, spec);
  //null-ary div "/()" is invalid


  // unary not "!(x)" is the normal case and has been tested above
  spec = "unary seq: ;(x) = x";
  TEST_BUILD_AND_RUN_MODULE(
    new AstOperator(';', new AstNumber(2)),
    2, spec);
  spec = "unary or: or(x) = bool(x)";
  TEST_BUILD_AND_RUN_MODULE(
    new AstOperator(AstOperator::eOr, new AstNumber(1, ObjTypeFunda::eBool)),
    true, spec);
  spec = "unary and: and(x) = bool(x)";
  TEST_BUILD_AND_RUN_MODULE(
    new AstOperator(AstOperator::eAnd, new AstNumber(1, ObjTypeFunda::eBool)),
    true, spec);
  spec = "unary minus: -(x)";
  TEST_BUILD_AND_RUN_MODULE(
    new AstOperator('-', new AstNumber(1)),
    -1, spec);
  spec = "unary plus: +(x) = x";
  TEST_BUILD_AND_RUN_MODULE(
    new AstOperator('+', new AstNumber(1)),
    +1, spec);
  spec = "spec = unary mult: *(x) = x";
  TEST_BUILD_AND_RUN_MODULE(
    new AstOperator('*', new AstNumber(1)),
    1, spec);
  // unary div "/(x)" is invalid

  spec = "binary equal-to: ==(1,1)";
  TEST_BUILD_AND_RUN_MODULE(
    new AstOperator(AstOperator::eEqualTo,
      new AstNumber(1),
      new AstNumber(1)),
    1==1, spec);
  spec = "binary equal-to: ==(1,2)";
  TEST_BUILD_AND_RUN_MODULE(
    new AstOperator(AstOperator::eEqualTo,
      new AstNumber(1),
      new AstNumber(2)),
    1==2, spec);

  // n-ary not "!(x y z)" is invalid
  spec = "n-ary seq: ;(1,2,3)";
  TEST_BUILD_AND_RUN_MODULE(
    new AstOperator(';',
      new AstNumber(1),
      new AstNumber(2),
      new AstNumber(3)),
    3, spec);
  spec = "n-ary plus: +(1,2,3)";
  TEST_BUILD_AND_RUN_MODULE(
    new AstOperator('+',
      new AstNumber(1),
      new AstNumber(2),
      new AstNumber(3)),
    1+2+3, spec);
  spec = "n-ary minus: -(1,2,3)";
  TEST_BUILD_AND_RUN_MODULE(
    new AstOperator('-',
      new AstNumber(1),
      new AstNumber(2),
      new AstNumber(3)),
    1-2-3, spec);
  spec = "n-ary mult: *(1,2,3)";
  TEST_BUILD_AND_RUN_MODULE(
    new AstOperator('*',
      new AstNumber(1),
      new AstNumber(2),
      new AstNumber(3)),
    1*2*3, spec);
  spec = "n-ary div: /(24,4,3)";
  TEST_BUILD_AND_RUN_MODULE(
    new AstOperator('/',
      new AstNumber(24),
      new AstNumber(4),
      new AstNumber(3)),
    24/4/3, spec);
}

TEST(IrGenTest, MAKE_TEST_NAME(
    a_seq_with_some_expressions_not_having_a_value_but_the_last_having_a_value,
    buildAndRunModule,
    returns_the_result_of_the_last_expression)) {
  string spec = "Sequence containing a function declaration";
  TEST_BUILD_AND_RUN_MODULE(
    new AstOperator(';',
      pe.mkFunDecl("foo"),
      new AstNumber(42)),
    42, spec);
}

TEST(IrGenTest, MAKE_TEST_NAME(
    a_function_declaration,
    buildModuleNoImplicitMain,
    adds_the_function_declaration_to_the_module_with_the_correct_signature)) {

  string spec = "Example: zero arguments";
  {
    // setup
    // IrBuilder is currently dumb and expects an expression having a value at
    // the end of a seq, thus provide one altought not needed for this test
    TestingIrGen UUT;
    ParserExt pe(*UUT.m_env, *UUT.m_errorHandler);
    auto_ptr<AstValue> ast(
      pe.mkFunDecl("foo"));

    // execute
    UUT.buildModuleNoImplicitMain(*ast);

    // verify
    Function* functionIr = UUT.m_module->getFunction("foo");
    EXPECT_TRUE(functionIr!=NULL)
      << amendAst(ast) << amendSpec(spec);
    EXPECT_EQ(Type::getInt32Ty(getGlobalContext()), functionIr->getReturnType())
      << amendAst(ast) << amendSpec(spec);
    EXPECT_EQ(functionIr->arg_size(), 0)
      << amendAst(ast) << amendSpec(spec);
  }

  spec = "Example: two arguments";
  {
    // setup
    // IrBuilder is currently dumb and expects an expression having a value at
    // the end of a seq, thus provide one altought not needed for this test
    TestingIrGen UUT;
    ParserExt pe(*UUT.m_env, *UUT.m_errorHandler);
    auto_ptr<AstValue> ast(
      pe.mkFunDecl(
        "foo",
        new AstArgDecl("arg1", new ObjTypeFunda(ObjTypeFunda::eInt)),
        new AstArgDecl("arg2", new ObjTypeFunda(ObjTypeFunda::eInt))));

    // execute
    UUT.buildModuleNoImplicitMain(*ast);

    // verify
    Function* functionIr = UUT.m_module->getFunction("foo");
    EXPECT_TRUE(functionIr!=NULL)
      << amendAst(ast) << amendSpec(spec);
    EXPECT_EQ(Type::getInt32Ty(getGlobalContext()), functionIr->getReturnType())
      << amendAst(ast) << amendSpec(spec);
    EXPECT_EQ(functionIr->arg_size(), 2)
      << amendAst(ast) << amendSpec(spec);
  }
}

TEST(IrGenTest, MAKE_TEST_NAME(
    a_function_definition,
    buildModuleNoImplicitMain,
    adds_the_definition_to_the_module_with_the_correct_signature)) {

  string spec = "Example: zero arguments";
  {
    // setup
    // IrBuilder is currently dumb and expects an expression having a value at
    // the end of a seq, thus provide one altought not needed for this test
    TestingIrGen UUT;
    ParserExt pe(*UUT.m_env, *UUT.m_errorHandler);
    auto_ptr<AstValue> ast(
      pe.mkFunDef(pe.mkFunDecl("foo"), new AstNumber(77)));

    // execute
    UUT.buildModuleNoImplicitMain(*ast);

    // verify
    Function* functionIr = UUT.m_module->getFunction("foo");
    EXPECT_TRUE(functionIr!=NULL)
      << amendAst(ast) << amendSpec(spec);
    EXPECT_EQ(Type::getInt32Ty(getGlobalContext()), functionIr->getReturnType())
      << amendAst(ast) << amendSpec(spec);
    EXPECT_EQ(functionIr->arg_size(), 0)
      << amendAst(ast) << amendSpec(spec);
  }

  spec = "Example: two arguments";
  {
    // setup
    // IrBuilder is currently dumb and expects an expression having a value at
    // the end of a seq, thus provide one altought not needed for this test
    TestingIrGen UUT;
    ParserExt pe(*UUT.m_env, *UUT.m_errorHandler);
    list<string>* args = new list<string>();
    args->push_back("arg1");
    args->push_back("arg2");
    auto_ptr<AstValue> ast(
      pe.mkFunDef(
        pe.mkFunDecl(
          "foo",
          new AstArgDecl("arg1", new ObjTypeFunda(ObjTypeFunda::eInt)),
          new AstArgDecl("arg2", new ObjTypeFunda(ObjTypeFunda::eInt))),
        new AstNumber(77)));

    // execute
    UUT.buildModuleNoImplicitMain(*ast);

    // verify
    Function* functionIr = UUT.m_module->getFunction("foo");
    EXPECT_TRUE(functionIr!=NULL)
      << amendAst(ast) << amendSpec(spec);
    EXPECT_EQ(Type::getInt32Ty(getGlobalContext()), functionIr->getReturnType())
      << amendAst(ast) << amendSpec(spec);
    EXPECT_EQ(functionIr->arg_size(), args->size())
      << amendAst(ast) << amendSpec(spec);
  }
}

TEST(IrGenTest, MAKE_TEST_NAME(
    a_function_definition_foo_with_body_returning_a_value_x,
    buildModuleNoImplicitMain,
    JIT_executing_foo_returns_x)) {

  string spec = "Example: zero arguments";
  {
    // setup
    // IrBuilder is currently dumb and expects an expression having a value at
    // the end of a seq, thus provide one altought not needed for this test
    TestingIrGen UUT;
    ParserExt pe(*UUT.m_env, *UUT.m_errorHandler);
    auto_ptr<AstValue> ast(
      pe.mkFunDef(pe.mkFunDecl("foo"), new AstNumber(77)));

    // execute
    UUT.buildModuleNoImplicitMain(*ast);

    // verify
    EXPECT_EQ( 77, UUT.jitExecFunction("foo") )
      << amendAst(ast) << amendSpec(spec);
  }

  spec = "Example: one argument, which however is ignored";
  {
    // setup
    // IrBuilder is currently dumb and expects an expression having a value at
    // the end of a seq, thus provide one altought not needed for this test
    TestingIrGen UUT;
    ParserExt pe(*UUT.m_env, *UUT.m_errorHandler);
    auto_ptr<AstValue> ast(
      pe.mkFunDef(
        pe.mkFunDecl(
          "foo",
          new AstArgDecl("arg1", new ObjTypeFunda(ObjTypeFunda::eInt))),
        new AstNumber(42)));

    // execute
    UUT.buildModuleNoImplicitMain(*ast);

    // verify
    EXPECT_EQ( 42, UUT.jitExecFunction1Arg("foo", 256) )
      << amendAst(ast) << amendSpec(spec);
  }
}

TEST(IrGenTest, MAKE_TEST_NAME(
    a_function_definition_foo_with_argument_x_with_body_returning_x,
    buildModuleNoImplicitMain,
    JIT_executing_foo_returns_x)) {
  // setup
  // IrBuilder is currently dumb and expects an expression having a value at
  // the end of a seq, thus provide one altought not needed for this test
  TestingIrGen UUT;
  ParserExt pe(*UUT.m_env, *UUT.m_errorHandler);
  auto_ptr<AstValue> ast(
    pe.mkFunDef(
      pe.mkFunDecl(
        "foo",
        new AstArgDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt))),
      new AstSymbol("x")));

  // execute
  UUT.buildModuleNoImplicitMain(*ast);

  // verify
  int x = 256;
  EXPECT_EQ( x, UUT.jitExecFunction1Arg("foo", x) )
    << amendAst(ast);
}

TEST(IrGenTest, MAKE_TEST_NAME(
    a_function_definition_foo_with_body_returning_a_simple_calculation_with_its_arguments,
    buildModuleNoImplicitMain,
    JIT_executing_foo_returns_result_of_that_calculation)) {

  // Culcultation: x*y

  // setup
  // IrBuilder is currently dumb and expects an expression having a value at
  // the end of a seq, thus provide one altought not needed for this test
  TestingIrGen UUT;
  ParserExt pe(*UUT.m_env, *UUT.m_errorHandler);
  auto_ptr<AstValue> ast(
    pe.mkFunDef(
      pe.mkFunDecl(
        "foo",
        new AstArgDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt)),
        new AstArgDecl("y", new ObjTypeFunda(ObjTypeFunda::eInt))),
      new AstOperator('*',
        new AstSymbol("x"),
        new AstSymbol("y"))));

  // execute
  UUT.buildModuleNoImplicitMain(*ast);

  // verify
  int x = 2;
  int y = 3;
  EXPECT_EQ( x*y, UUT.jitExecFunction2Arg("foo", x, y) ) << amendAst(ast);
}

TEST(IrGenTest, MAKE_TEST_NAME(
    a_function_definition_foo_with_argument_x_and_with_body_containing_a_simple_assignement_to_x_and_as_last_expr_in_bodies_seq_x,
    buildModuleNoImplicitMain,
    JIT_executing_foo_returns_result_of_that_calculation)) {

  // Culcultation: x = x+1, return x

  // setup
  // IrBuilder is currently dumb and expects an expression having a value at
  // the end of a seq, thus provide one altought not needed for this test
  TestingIrGen UUT;
  ParserExt pe(*UUT.m_env, *UUT.m_errorHandler);
  auto_ptr<AstValue> ast(
    pe.mkFunDef(
      pe.mkFunDecl(
        "foo",
        new AstArgDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt))),
      new AstOperator(';',
        new AstOperator('=',
          new AstSymbol("x"),
          new AstOperator('+',
            new AstSymbol("x"),
            new AstNumber(1))),
        new AstSymbol("x"))));

  // execute
  UUT.buildModuleNoImplicitMain(*ast);

  // verify
  int x = 2;
  EXPECT_EQ( x+1, UUT.jitExecFunction1Arg("foo", x)) << amendAst(ast);
}

TEST(IrGenTest, MAKE_TEST_NAME(
    multiple_identical_function_declarations,
    buildModuleNoImplicitMain,
    succeeds)) {
  TestingIrGen UUT;
  ParserExt pe(*UUT.m_env, *UUT.m_errorHandler);
  EXPECT_NO_THROW(
    UUT.buildModuleNoImplicitMain(
      *new AstOperator(';',
        pe.mkFunDecl("foo"),
        pe.mkFunDecl("foo"))));
  EXPECT_TRUE(UUT.m_errorHandler->errors().empty());
}

TEST(IrGenTest, MAKE_TEST_NAME(
    a_function_declaration_and_a_matching_function_definition,
    buildModuleNoImplicitMain,
    succeeds)) {
  TestingIrGen UUT;
  ParserExt pe(*UUT.m_env, *UUT.m_errorHandler);
  EXPECT_NO_THROW(
    UUT.buildModuleNoImplicitMain(
      *new AstOperator(';',
        pe.mkFunDecl("foo"),
        pe.mkFunDef(pe.mkFunDecl("foo"), new AstNumber(42)))));
  EXPECT_TRUE(UUT.m_errorHandler->errors().empty());
}

TEST(IrGenTest, MAKE_TEST_NAME(
    a_function_definition_in_a_function_body,
    buildAndRunModule,
    succeeds)) {
  
  TEST_BUILD_AND_RUN_MODULE(
    new AstOperator(';',
      pe.mkFunDef(pe.mkFunDecl("foo"),
        new AstOperator(';',
          pe.mkFunDef(pe.mkFunDecl("bar"), new AstNumber(42)),
          new AstFunCall(new AstSymbol("bar")))),
      new AstFunCall(new AstSymbol("foo"))),
    42, "");
}

TEST(IrGenTest, MAKE_TEST_NAME(
    re_declaration_of_an_identifier_with_different_type,
    buildAndRunModule,
    reports_an_eIncompatibleRedaclaration)) {
  string spec = "First function type, then fundamental type";
  TEST_BUILD_MODULE_REPORTS_ERROR(
    new AstOperator(';',
      pe.mkFunDecl("foo"),
      new AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt))),
    Error::eIncompatibleRedaclaration, spec);

  spec = "First fundamental type, then function type";
  TEST_BUILD_MODULE_REPORTS_ERROR(
    new AstOperator(';',
      new AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt)),
      pe.mkFunDecl("foo")),
    Error::eIncompatibleRedaclaration, spec);

  spec = "One mutable, the other inmutable";
  TEST_BUILD_MODULE_REPORTS_ERROR(
    new AstOperator(';',
      new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eNoQualifier)),
      new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable))),
    Error::eIncompatibleRedaclaration, spec);
}

TEST(IrGenTest, MAKE_TEST_NAME(
    a_function_call_to_an_defined_function,
    buildAndRunModule,
    returns_result_of_that_function_call)) {

  string spec = "Trivial function with zero arguments returning a constant";
  TEST_BUILD_AND_RUN_MODULE(
    new AstOperator(';',
      pe.mkFunDef(
        pe.mkFunDecl("foo"),
        new AstNumber(42)),
      new AstFunCall(new AstSymbol("foo"))),
    42, spec);

  spec = "Simple function with one argument which is ignored and a constant is returned";
  TEST_BUILD_AND_RUN_MODULE(
    new AstOperator(';',
      pe.mkFunDef(
        pe.mkFunDecl("foo", new AstArgDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt))),
        new AstNumber(42)),
      new AstFunCall(new AstSymbol("foo"), new AstCtList(new AstNumber(0)))),
    42, spec);

  spec = "Simple function with two arguments whichs sum is returned";
  TEST_BUILD_AND_RUN_MODULE(
    new AstOperator(';',
      pe.mkFunDef(
        pe.mkFunDecl(
          "add",
          new AstArgDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt)),
          new AstArgDecl("y", new ObjTypeFunda(ObjTypeFunda::eInt))),
        new AstOperator('+',
          new AstSymbol("x"),
          new AstSymbol("y"))),
      new AstFunCall(new AstSymbol("add"),
        new AstCtList(new AstNumber(1), new AstNumber(2)))),
    1+2, spec);
}

TEST(IrGenTest, MAKE_TEST_NAME(
    a_function_call_to_an_defined_function_WITH_incorrect_number_of_arguments,
    buildAndRunModule,
    throws)) {

  string spec = "Function foo expects one arg, but zero where passed on call";
  TEST_BUILD_AND_RUN_MODULE_THROWS(
    new AstOperator(';',
      pe.mkFunDef(
        pe.mkFunDecl("foo", new AstArgDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt))),
        new AstNumber(42)),
      new AstFunCall(new AstSymbol("foo"))),
    spec);

  spec = "Function foo expects no args, but one arg was passed on call";
  TEST_BUILD_AND_RUN_MODULE_THROWS(
    new AstOperator(';',
        pe.mkFunDef(
          pe.mkFunDecl("foo"),
          new AstNumber(42)),
      new AstFunCall(new AstSymbol("foo"), new AstCtList(new AstNumber(0)))),
    spec);
}

TEST(IrGenTest, MAKE_TEST_NAME(
    a_inmutable_data_object_definition_of_foo_being_initialized_with_x,
    buildAndRunModule,
    returns_x_rvalue)) {

  string spec = "Value of definition expression should equal initializer's value";
  TEST_BUILD_AND_RUN_MODULE(
    new AstDataDef(
      new AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt)),
      new AstNumber(42)),
    42, spec);

  spec = "Value of definition expression is an rvalue and thus _not_ "
    "assignable to";
  TEST_BUILD_AND_RUN_MODULE_THROWS(
    new AstOperator(';',
      new AstOperator('=',
        new AstDataDef(
          new AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt)),
          new AstNumber(42)),
        new AstNumber(77)),
      new AstSymbol("foo")),
    spec);
}

TEST(IrGenTest, MAKE_TEST_NAME(
    a_mutable_data_object_definition_of_foo_being_initialized_with_x,
    buildAndRunModule,
    returns_x_lvalue)) {

  string spec = "Value of definition expression should equal initializer's value";
  TEST_BUILD_AND_RUN_MODULE(
    new AstDataDef(
      new AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable)),
      new AstNumber(42)),
    42, spec);

  spec = "Definition expresssion is an lvalue which concequently is assignable to";
  TEST_BUILD_AND_RUN_MODULE(
    new AstOperator('=',
      new AstDataDef(
        new AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable)),
        new AstNumber(42)),
      new AstNumber(77)),
    77, spec);
}

TEST(IrGenTest, MAKE_TEST_NAME(
    foo_defined_as_data_object_followed_by_a_simple_expression_referencing_foo,
    buildAndRunModule,
    returns_result_of_that_expression)) {

  string spec = "value (aka immutable (aka const) data obj)";
  TEST_BUILD_AND_RUN_MODULE(
    new AstOperator(';',
      new AstDataDef(
        new AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt)),
        new AstNumber(42)),
      new AstOperator('+',
        new AstSymbol("foo"),
        new AstNumber(77))),
    42+77, spec);

  spec = "variable (aka mutable data obj)";
  TEST_BUILD_AND_RUN_MODULE(
    new AstOperator(';',
      new AstDataDef(
        new AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable)),
        new AstNumber(42)),
      new AstOperator('+',
        new AstSymbol("foo"),
        new AstNumber(77))),
    42+77, spec);
}

TEST(IrGenTest, MAKE_TEST_NAME(
    foo_defined_as_variable_initialized_with_x_followed_by_assigning_y_to_foo,
    buildAndRunModule,
    returns_y)) {

  string spec = "Example: Assignement 'foo=77' being last expression in sequence"; 
  TEST_BUILD_AND_RUN_MODULE(
    new AstOperator(';',
      new AstDataDef(
        new AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable)),
        new AstNumber(42)),
      new AstOperator('=',
        new AstSymbol("foo"),
        new AstNumber(77))),
    77, spec);

  spec = "Example: After assignment 'foo=77', there is a further 'foo' expression"; 
  TEST_BUILD_AND_RUN_MODULE(
    new AstOperator(';',
      new AstDataDef(
        new AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable)),
        new AstNumber(42)),
      new AstOperator('=',
        new AstSymbol("foo"),
        new AstNumber(77)),
      new AstSymbol("foo")),
    77, spec);
}

TEST(IrGenTest, MAKE_TEST_NAME4(
    foo_defined_as_value_followed_by_assigning_to_foo,
    buildAndRunModule,
    throws,
    BECAUSE_values_are_immutable_data_objects)) {
  TEST_BUILD_AND_RUN_MODULE_THROWS(
    new AstOperator(';',
      new AstDataDef(
        new AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt)),
        new AstNumber(42)),
      new AstOperator('=',
        new AstSymbol("foo"),
        new AstNumber(77))),
    "");
}

TEST(IrGenTest, MAKE_TEST_NAME(
    a_value_definition_of_foo_with_no_explicit_initializer,
    buildAndRunModule,
    returns_the_default_which_is_zero)) {
  TEST_BUILD_AND_RUN_MODULE(
    new AstDataDef(
      new AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable))),
    0, "");
}

TEST(IrGenTest, MAKE_TEST_NAME(
    multiple_identical_data_object_declaration_of_x,
    buildAndRunModule,
    succeeds)) {
  TestingIrGen UUT;
  ParserExt pe(*UUT.m_env, *UUT.m_errorHandler);
  EXPECT_NO_THROW(
    new AstOperator(';',
      new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt)),
      new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt))));
  EXPECT_TRUE(UUT.m_errorHandler->errors().empty());
}

TEST(IrGenTest, MAKE_TEST_NAME2(
    GIVEN_a_pseudo_global_variable_named_x_AND_a_function_definition_also_defining_a_variable_x,
    THEN_the_inner_defintion_of_variable_x_shadows_the_outer_variable_x)) {

  // 'global' in quotes because there not really global variables
  // yet. Variables defined in 'global' scope really are local variables in
  // the implicit main method. 

  string spec = "function argument named 'x' shadows 'global' variable also named 'x'";
  TEST_BUILD_AND_RUN_MODULE(
    new AstOperator(';',
      new AstDataDef(new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt)), new AstNumber(42)),
      pe.mkFunDef(
        pe.mkFunDecl("foo", new AstArgDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt))),
        new AstOperator('=',
          new AstSymbol("x"),
          new AstNumber(77))),
      new AstSymbol("x")),
    42, spec);

  spec = "variable 'x' local to a function shadows 'global' variable also named 'x'";
  TEST_BUILD_AND_RUN_MODULE(
    new AstOperator(';',
      new AstDataDef(new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt)), new AstNumber(42)),
      pe.mkFunDef(
        pe.mkFunDecl("foo"),
        new AstDataDef(
          new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt)),
          new AstNumber(77))),
      new AstSymbol("x")),
    42, spec);
}

TEST(IrGenTest, MAKE_TEST_NAME(
    redefinition_of_an_object_which_means_same_name_and_same_type,
    buildModuleNoImplicitMain,
    reports_an_eRedefinition)) {

  string spec = "Example: two local variables in implicit main method";
  TEST_BUILD_MODULE_NO_IMPLICIT_MAIN_REPORTS_ERROR(
    new AstOperator(';',
      new AstDataDef(new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt))),
      new AstDataDef(new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt)))),
    Error::eRedefinition, spec);

  spec = "Example: two parameters";
  TEST_BUILD_MODULE_NO_IMPLICIT_MAIN_REPORTS_ERROR(
    pe.mkFunDef(
      pe.mkFunDecl(
        "foo",
        new AstArgDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt)),
        new AstArgDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt))),
      new AstNumber(42)),
    Error::eRedefinition, spec);

  spec = "Example: a parameter and a local variable in the function's body";
  TEST_BUILD_MODULE_NO_IMPLICIT_MAIN_REPORTS_ERROR(
    pe.mkFunDef(
      pe.mkFunDecl("foo", new AstArgDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt))),
      new AstDataDef(new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable)))),
    Error::eRedefinition, spec);

  spec = "Example: two functions";
  TEST_BUILD_MODULE_NO_IMPLICIT_MAIN_REPORTS_ERROR(
    new AstOperator(';',
      pe.mkFunDef(pe.mkFunDecl("foo"), new AstNumber(42)),
      pe.mkFunDef(pe.mkFunDecl("foo"), new AstNumber(42))),
    Error::eRedefinition, spec);
}

// Temporary test while introducing types
TEST(IrGenTest, MAKE_TEST_NAME(
    a_seq_containing_a_bool_literal,
    buildAndRunModule,
    succeeds)) {
  TEST_BUILD_AND_RUN_MODULE(
    new AstOperator(';',
      new AstNumber(0, ObjTypeFunda::eBool),
      new AstNumber(1, ObjTypeFunda::eBool)),
    1, "");
}

TEST(IrGenTest, MAKE_TEST_NAME(
    an_if_else_expression_WITH_a_condition_evaluating_to_true,
    buildAndRunModule,
    returns_the_value_of_the_then_clause)) {
  TEST_BUILD_AND_RUN_MODULE(
    new AstIf(
      new AstNumber(1, ObjTypeFunda::eBool), // condition
      new AstNumber(2), // then clause
      new AstNumber(3)), // else clause
    2, "");
}

TEST(IrGenTest, MAKE_TEST_NAME(
    an_if_else_expression_WITH_a_condition_evaluating_to_false,
    buildAndRunModule,
    returns_the_value_of_the_else_clause)) {
  TEST_BUILD_AND_RUN_MODULE(
    new AstIf(
      new AstNumber(0, ObjTypeFunda::eBool), // condition
      new AstNumber(2), // then clause
      new AstNumber(3)), // else clause
    3, "");
}

TEST(IrGenTest, MAKE_TEST_NAME(
    an_if_expression_without_else_WITH_a_condition_evaluating_to_false,
    buildAndRunModule,
    returns_default_value_of_expressions_type)) {
  TEST_BUILD_AND_RUN_MODULE(
    new AstIf(
      new AstNumber(0, ObjTypeFunda::eBool), // condition
      new AstNumber(2)), // then clause
    0, ""); // default for int
}
