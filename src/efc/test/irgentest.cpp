#include "test.h"
#include "../irgen.h"
#include "../semanticanalizer.h"
#include "../parserext.h"
#include "../ast.h"
#include "../env.h"
#include "../errorhandler.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Module.h"
#include "llvm/ExecutionEngine/JIT.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include <memory>
using namespace testing;
using namespace std;
using namespace llvm;

class TestingIrGen : public IrGen {
public:
  TestingIrGen() :
    IrGen(
      *(m_errorHandler = new ErrorHandler()) ),
    m_semanticAnalizer(m_env, *m_errorHandler) {};
  ~TestingIrGen() { delete m_errorHandler; };
  using IrGen::jitExecFunction;
  using IrGen::jitExecFunction1Arg;
  using IrGen::jitExecFunction2Arg;
  using IrGen::m_module;
  using IrGen::m_executionEngine;
  Env m_env;
  ErrorHandler* m_errorHandler;
  SemanticAnalizer m_semanticAnalizer;
};

void testgenIrInImplicitMain(TestingIrGen& UUT, AstValue* astRoot,
  int expectedResult, const string& spec = "") {

  ENV_ASSERT_TRUE( astRoot!=NULL );
  unique_ptr<AstValue> astRootAp(astRoot);
  bool unexpctedExceptionThrown = false;
  string exceptionDescription;

  try {
    // setup
    SemanticAnalizer semanticAnalizer(UUT.m_env, *UUT.m_errorHandler);
    semanticAnalizer.analyze(*astRoot);

    // execute
    UUT.genIr(*astRoot);
    int result = UUT.jitExecFunction("main");

    // verify
    EXPECT_EQ(expectedResult, result) << amendSpec(spec) << amendAst(astRoot) <<
      amend(UUT.m_module);
  }

  // For better diagnostic messages in case unexpectedly an error
  // occured. Also without those catches, gtest printer does not print the
  // location (file, lineno) of the test that failed.
  catch (exception& e) {
    unexpctedExceptionThrown = true;
    exceptionDescription = e.what();
  }
  catch (...) {
    unexpctedExceptionThrown = true;
  }
  if ( unexpctedExceptionThrown ) {
    FAIL() << "Unexpectedly an exception was thrown. " <<
      "It's description is: '" << exceptionDescription << "'\n" <<
      amendSpec(spec) << amend(*UUT.m_errorHandler) << amendAst(astRoot);
  }
}

#define TEST_GEN_IR_IN_IMPLICIT_MAIN(astRoot, expectedResult, spec)     \
  {                                                                     \
    SCOPED_TRACE("testgenIrInImplicitMain called from here (via TEST_GEN_IR_IN_IMPLICIT_MAIN)"); \
    TestingIrGen UUT;                                                   \
    ParserExt pe(UUT.m_env, *UUT.m_errorHandler);                       \
    testgenIrInImplicitMain(UUT, pe.mkMainFunDef(astRoot), expectedResult, spec); \
  }

void testgenIrInImplicitMainThrows(TestingIrGen& UUT, AstValue* astRoot,
  const string& spec = "") {
  // setup
  ENV_ASSERT_TRUE( astRoot!=NULL );
  unique_ptr<AstValue> ast(astRoot);
  bool anyThrow = false;
  string excptionwhat;
  SemanticAnalizer semanticAnalizer(UUT.m_env, *UUT.m_errorHandler);
  semanticAnalizer.analyze(*astRoot);
  
  // execute
  try {
    UUT.genIr(*ast.get());
  }

  // verify
  catch (BuildError&) {
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

#define TEST_GEN_IR_IMPLICIT_MAIN_THROWS(astRoot, spec)                 \
  {                                                                     \
    SCOPED_TRACE("testgenIrInImplicitMainThrows called from here (via TEST_GEN_IR_IMPLICIT_MAIN_THROWS)"); \
    TestingIrGen UUT;                                                   \
    ParserExt pe(UUT.m_env, *UUT.m_errorHandler);                       \
    testgenIrInImplicitMainThrows(UUT, pe.mkMainFunDef(astRoot), spec); \
  }

void testgenIrReportsError(TestingIrGen& UUT, AstValue* astRoot,
  Error::No expectedErrorNo, const string& spec = "") {

  // setup
  ENV_ASSERT_TRUE( astRoot!=NULL );
  unique_ptr<AstValue> ast(astRoot);
  bool anyThrow = false;
  bool didThrowBuildError = false;
  string excptionwhat;
  SemanticAnalizer semanticAnalizer(UUT.m_env, *UUT.m_errorHandler);
  semanticAnalizer.analyze(*astRoot);

  // execute
  try {
    UUT.genIr(*astRoot);
  }

  // verify
  catch (BuildError&) {
    didThrowBuildError = true;

    const ErrorHandler::Container& errors = UUT.m_errorHandler->errors();
    EXPECT_EQ(1, errors.size()) <<
      "Expecting exactly one error\n" << 
      amendSpec(spec) << amend(*UUT.m_errorHandler) << amendAst(astRoot);
    if ( !errors.empty() ) {
      EXPECT_EQ(expectedErrorNo, errors.front()->no()) <<
        amendSpec(spec) << amend(*UUT.m_errorHandler) << amendAst(astRoot);
    }
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

#define TEST_GEN_IR_REPORTS_ERROR(astRoot, expectedErrorNo, spec) \
  {                                                                     \
    SCOPED_TRACE("testgenIrReportsError called from here (via TEST_GEN_IR_REPORTS_ERROR)"); \
    TestingIrGen UUT;                                            \
    ParserExt pe(UUT.m_env, *UUT.m_errorHandler);                      \
    testgenIrReportsError(UUT, astRoot, expectedErrorNo, false, spec); \
  }

TEST(IrGenTest, MAKE_TEST_NAME(
    a_single_literal,
    genIrInImplicitMain,
    returns_the_literal_s_value)) {
  TEST_GEN_IR_IN_IMPLICIT_MAIN(new AstNumber(42), 42, "");
}

TEST(IrGenTest, MAKE_TEST_NAME(
    an_operator,
    genIrInImplicitMain,
    returns_the_result_of_that_operator)) {

  // ; aka sequence
  string spec = "The sequence operator evaluates all arguments and returns the last";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstOperator(';', new AstNumber(11), new AstNumber(22)), 22, "");

  // not
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstCast(ObjTypeFunda::eInt,
      new AstOperator(AstOperator::eNot,
        new AstNumber(0, ObjTypeFunda::eBool))),
    !false, "");
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstCast(ObjTypeFunda::eInt,
      new AstOperator(AstOperator::eNot,
        new AstNumber(1, ObjTypeFunda::eBool))),
    !true, "");

  // and
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstCast(ObjTypeFunda::eInt,
      new AstOperator(AstOperator::eAnd,
        new AstNumber(0, ObjTypeFunda::eBool),
        new AstNumber(0, ObjTypeFunda::eBool))),
    false && false, "");
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstCast(ObjTypeFunda::eInt,
      new AstOperator(AstOperator::eAnd,
        new AstNumber(0, ObjTypeFunda::eBool),
        new AstNumber(1, ObjTypeFunda::eBool))),
    false && true, "");
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstCast(ObjTypeFunda::eInt,
      new AstOperator(AstOperator::eAnd,
        new AstNumber(1, ObjTypeFunda::eBool),
        new AstNumber(0, ObjTypeFunda::eBool))),
    true && false, "");
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstCast(ObjTypeFunda::eInt,
      new AstOperator(AstOperator::eAnd,
        new AstNumber(1, ObjTypeFunda::eBool),
        new AstNumber(1, ObjTypeFunda::eBool))),
    true && true, "");

  // or
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstCast(ObjTypeFunda::eInt,
      new AstOperator(AstOperator::eOr,
        new AstNumber(0, ObjTypeFunda::eBool),
        new AstNumber(0, ObjTypeFunda::eBool))),
    false || false, "");
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstCast(ObjTypeFunda::eInt,
      new AstOperator(AstOperator::eOr,
        new AstNumber(0, ObjTypeFunda::eBool),
        new AstNumber(1, ObjTypeFunda::eBool))),
    false || true, "");
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstCast(ObjTypeFunda::eInt,
      new AstOperator(AstOperator::eOr,
        new AstNumber(1, ObjTypeFunda::eBool),
        new AstNumber(0, ObjTypeFunda::eBool))),
    true || false, "");
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstCast(ObjTypeFunda::eInt,
      new AstOperator(AstOperator::eOr,
        new AstNumber(1, ObjTypeFunda::eBool),
        new AstNumber(1, ObjTypeFunda::eBool))),
    true || true, "");

  // ==
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstCast(ObjTypeFunda::eInt,
      new AstOperator(AstOperator::eEqualTo, new AstNumber(2), new AstNumber(2))),
    2==2, "");
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstCast(ObjTypeFunda::eInt,
      new AstOperator(AstOperator::eEqualTo, new AstNumber(1), new AstNumber(2))),
    1==2, "");

  // + - * /
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstOperator('+', new AstNumber(1), new AstNumber(2)),
    1+2, "");
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstOperator('-', new AstNumber(1), new AstNumber(2)),
    1-2, "");
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstOperator('*', new AstNumber(1), new AstNumber(2)),
    1*2, "");
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstOperator('/', new AstNumber(6), new AstNumber(3)),
    6/3, "");
}

TEST(IrGenTest, MAKE_TEST_NAME(
    a_cast,
    genIrInImplicitMain,
    succeeds)) {

  string spec = "bool -> int: false is 0";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstCast(
      new ObjTypeFunda(ObjTypeFunda::eInt),
      new AstNumber(0, ObjTypeFunda::eBool)),
    0, spec);

  spec = "bool -> int: true is 1";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstCast(
      new ObjTypeFunda(ObjTypeFunda::eInt),
      new AstNumber(1, ObjTypeFunda::eBool)),
    1, spec);

  spec = "int -> bool: 0 is false";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstIf(
      new AstCast(
        new ObjTypeFunda(ObjTypeFunda::eBool),
        new AstNumber(0, ObjTypeFunda::eInt)),
      new AstNumber(1),
      new AstNumber(2)),
    2, spec);

  spec = "int -> bool: not 0 is true";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstIf(
      new AstCast(
        new ObjTypeFunda(ObjTypeFunda::eBool),
        new AstNumber(42, ObjTypeFunda::eInt)),
      new AstNumber(1),
      new AstNumber(2)),
    1, spec);
}

TEST(IrGenTest, MAKE_TEST_NAME(
    a_seq_with_some_expressions_not_having_a_value_but_the_last_having_a_value,
    genIrInImplicitMain,
    returns_the_result_of_the_last_expression)) {
  // the normal use case of the sequence operator is tested in a separte test
  // together with all other operators

  string spec = "Sequence containing a function declaration";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstOperator(';',
      pe.mkFunDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt)),
      new AstNumber(42)),
    42, spec);
}

TEST(IrGenTest, MAKE_TEST_NAME(
    a_function_declaration,
    genIr,
    adds_the_function_declaration_to_the_module_with_the_correct_signature)) {

  string spec = "Example: zero arguments";
  {
    // setup
    // IrGen is currently dumb and expects an expression having a value at
    // the end of a seq, thus provide one altought not needed for this test
    TestingIrGen UUT;
    ParserExt pe(UUT.m_env, *UUT.m_errorHandler);
    unique_ptr<AstValue> ast(
      pe.mkFunDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt)));
    UUT.m_semanticAnalizer.analyze(*ast.get());

    // execute
    UUT.genIr(*ast);

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
    // IrGen is currently dumb and expects an expression having a value at
    // the end of a seq, thus provide one altought not needed for this test
    TestingIrGen UUT;
    ParserExt pe(UUT.m_env, *UUT.m_errorHandler);
    unique_ptr<AstValue> ast(
      pe.mkFunDecl(
        "foo",
        new ObjTypeFunda(ObjTypeFunda::eInt),
        new AstArgDecl("arg1", new ObjTypeFunda(ObjTypeFunda::eInt)),
        new AstArgDecl("arg2", new ObjTypeFunda(ObjTypeFunda::eInt))));
    UUT.m_semanticAnalizer.analyze(*ast.get());

    // execute
    UUT.genIr(*ast);

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
    genIr,
    adds_the_definition_to_the_module_with_the_correct_signature)) {

  string spec = "Example: zero args, ret type int";
  {
    // setup
    // IrGen is currently dumb and expects an expression having a value at
    // the end of a seq, thus provide one altought not needed for this test
    TestingIrGen UUT;
    ParserExt pe(UUT.m_env, *UUT.m_errorHandler);
    unique_ptr<AstValue> ast(
      pe.mkFunDef(
        pe.mkFunDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt)),
        new AstNumber(77)));
    UUT.m_semanticAnalizer.analyze(*ast.get());

    // execute
    UUT.genIr(*ast);

    // verify
    Function* functionIr = UUT.m_module->getFunction("foo");
    EXPECT_TRUE(functionIr!=NULL)
      << amendAst(ast) << amendSpec(spec);
    EXPECT_EQ(Type::getInt32Ty(getGlobalContext()), functionIr->getReturnType())
      << amendAst(ast) << amendSpec(spec);
    EXPECT_EQ(functionIr->arg_size(), 0)
      << amendAst(ast) << amendSpec(spec);
  }

  spec = "Example: 1 int arg, 1 bool arg, ret type void";
  {
    // setup
    // IrGen is currently dumb and expects an expression having a value at
    // the end of a seq, thus provide one altought not needed for this test
    TestingIrGen UUT;
    ParserExt pe(UUT.m_env, *UUT.m_errorHandler);
    unique_ptr<AstValue> ast(
      pe.mkFunDef(
        pe.mkFunDecl(
          "foo",
          new ObjTypeFunda(ObjTypeFunda::eVoid),
          new AstArgDecl("arg1", new ObjTypeFunda(ObjTypeFunda::eInt)),
          new AstArgDecl("arg2", new ObjTypeFunda(ObjTypeFunda::eBool))),
        new AstNop()));
    UUT.m_semanticAnalizer.analyze(*ast.get());

    // execute
    UUT.genIr(*ast);

    // verify
    Function* functionIr = UUT.m_module->getFunction("foo");
    EXPECT_TRUE(functionIr!=NULL)
      << amendAst(ast) << amendSpec(spec);
    EXPECT_EQ(Type::getVoidTy(getGlobalContext()), functionIr->getReturnType())
      << amendAst(ast) << amendSpec(spec);
    EXPECT_EQ(2, functionIr->arg_size())
      << amendAst(ast) << amendSpec(spec);
  }
}

TEST(IrGenTest, MAKE_TEST_NAME(
    a_function_definition_foo_with_return_type_void,
    genIr,
    JIT_executing_foo_succeeds))   {
  // setup
  TestingIrGen UUT;
  ParserExt pe(UUT.m_env, *UUT.m_errorHandler);
  unique_ptr<AstValue> ast(
    pe.mkFunDef(
      pe.mkFunDecl("foo", new ObjTypeFunda(ObjTypeFunda::eVoid)),
      new AstNop()));
  UUT.m_semanticAnalizer.analyze(*ast.get());

  // execute
  UUT.genIr(*ast);

  // verify
  // todo: via e.g. global variable verify that function really was called
  EXPECT_NO_THROW( UUT.jitExecFunction("foo"))
    << amendAst(ast);
}

TEST(IrGenTest, MAKE_TEST_NAME(
    a_function_definition_foo_with_body_returning_a_value_x,
    genIr,
    JIT_executing_foo_returns_x)) {

  string spec = "Example: zero arguments";
  {
    // setup
    // IrGen is currently dumb and expects an expression having a value at
    // the end of a seq, thus provide one altought not needed for this test
    TestingIrGen UUT;
    ParserExt pe(UUT.m_env, *UUT.m_errorHandler);
    unique_ptr<AstValue> ast(
      pe.mkFunDef(
        pe.mkFunDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt)),
        new AstNumber(77)));
    UUT.m_semanticAnalizer.analyze(*ast.get());

    // execute
    UUT.genIr(*ast);

    // verify
    EXPECT_EQ( 77, UUT.jitExecFunction("foo") )
      << amendAst(ast) << amendSpec(spec);
  }

  spec = "Example: one argument, which however is ignored";
  {
    // setup
    // IrGen is currently dumb and expects an expression having a value at
    // the end of a seq, thus provide one altought not needed for this test
    TestingIrGen UUT;
    ParserExt pe(UUT.m_env, *UUT.m_errorHandler);
    unique_ptr<AstValue> ast(
      pe.mkFunDef(
        pe.mkFunDecl(
          "foo",
          new ObjTypeFunda(ObjTypeFunda::eInt),
          new AstArgDecl("arg1", new ObjTypeFunda(ObjTypeFunda::eInt))),
        new AstNumber(42)));
    UUT.m_semanticAnalizer.analyze(*ast.get());

    // execute
    UUT.genIr(*ast);

    // verify
    EXPECT_EQ( 42, UUT.jitExecFunction1Arg("foo", 256) )
      << amendAst(ast) << amendSpec(spec);
  }
}

TEST(IrGenTest, MAKE_TEST_NAME(
    a_function_definition_foo_with_argument_x_with_body_returning_x,
    genIr,
    JIT_executing_foo_returns_x)) {
  // setup
  // IrGen is currently dumb and expects an expression having a value at
  // the end of a seq, thus provide one altought not needed for this test
  TestingIrGen UUT;
  ParserExt pe(UUT.m_env, *UUT.m_errorHandler);
  unique_ptr<AstValue> ast(
    pe.mkFunDef(
      pe.mkFunDecl(
        "foo",
        new ObjTypeFunda(ObjTypeFunda::eInt),
        new AstArgDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt))),
      new AstSymbol("x")));
  UUT.m_semanticAnalizer.analyze(*ast);

  // execute
  UUT.genIr(*ast);

  // verify
  int x = 256;
  EXPECT_EQ( x, UUT.jitExecFunction1Arg("foo", x) )
    << amendAst(ast);
}

TEST(IrGenTest, MAKE_TEST_NAME(
    a_function_definition_foo_with_body_returning_a_simple_calculation_with_its_arguments,
    genIr,
    JIT_executing_foo_returns_result_of_that_calculation)) {

  // Culcultation: x*y

  // setup
  // IrGen is currently dumb and expects an expression having a value at
  // the end of a seq, thus provide one altought not needed for this test
  TestingIrGen UUT;
  ParserExt pe(UUT.m_env, *UUT.m_errorHandler);
  unique_ptr<AstValue> ast(
    pe.mkFunDef(
      pe.mkFunDecl(
        "foo",
        new ObjTypeFunda(ObjTypeFunda::eInt),
        new AstArgDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt)),
        new AstArgDecl("y", new ObjTypeFunda(ObjTypeFunda::eInt))),
      new AstOperator('*',
        new AstSymbol("x"),
        new AstSymbol("y"))));
  UUT.m_semanticAnalizer.analyze(*ast);

  // execute
  UUT.genIr(*ast);

  // verify
  int x = 2;
  int y = 3;
  EXPECT_EQ( x*y, UUT.jitExecFunction2Arg("foo", x, y) ) << amendAst(ast);
}

TEST(IrGenTest, MAKE_TEST_NAME(
    a_function_definition_foo_with_argument_x_and_with_body_containing_a_simple_assignement_to_x_and_as_last_expr_in_bodies_seq_x,
    genIr,
    JIT_executing_foo_returns_result_of_that_calculation)) {

  // Culcultation: x = x+1, return x

  // setup
  // IrGen is currently dumb and expects an expression having a value at
  // the end of a seq, thus provide one altought not needed for this test
  TestingIrGen UUT;
  ParserExt pe(UUT.m_env, *UUT.m_errorHandler);
  unique_ptr<AstValue> ast(
    pe.mkFunDef(
      pe.mkFunDecl(
        "foo",
        new ObjTypeFunda(ObjTypeFunda::eInt),
        new AstArgDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt))),
      new AstOperator(';',
        new AstOperator('=',
          new AstSymbol("x"),
          new AstOperator('+',
            new AstSymbol("x"),
            new AstNumber(1))),
        new AstSymbol("x"))));
  UUT.m_semanticAnalizer.analyze(*ast);

  // execute
  UUT.genIr(*ast);

  // verify
  int x = 2;
  EXPECT_EQ( x+1, UUT.jitExecFunction1Arg("foo", x)) << amendAst(ast);
}

TEST(IrGenTest, MAKE_TEST_NAME2(
    a_function_definition_foo_with_mixed_argument_types,
    genIr)) {

  // setup
  TestingIrGen UUT;
  ParserExt pe(UUT.m_env, *UUT.m_errorHandler);
  unique_ptr<AstValue> ast(
    pe.mkFunDef(
      pe.mkFunDecl(
        "foo",
        new ObjTypeFunda(ObjTypeFunda::eInt),
        new AstArgDecl("condition", new ObjTypeFunda(ObjTypeFunda::eBool)),
        new AstArgDecl("thenValue", new ObjTypeFunda(ObjTypeFunda::eInt)),
        new AstArgDecl("elseValue", new ObjTypeFunda(ObjTypeFunda::eInt))),
      new AstIf(
        new AstSymbol("condition"),
        new AstSymbol("thenValue"),
        new AstSymbol("elseValue"))));
  UUT.m_semanticAnalizer.analyze(*ast);

  // execute
  UUT.genIr(*ast);

  // verify
  auto jitExecFoo = [&UUT](bool condition, int thenValue, int elseValue)->int {
    Function* functionIr = UUT.m_module->getFunction("foo");
    void* functionVoidPtr = UUT.m_executionEngine->getPointerToFunction(functionIr);
    assert(functionVoidPtr);
    int (*functionPtr)(bool,int,int) = (int (*)(bool,int,int))(intptr_t)functionVoidPtr;
    assert(functionPtr);
    return functionPtr(condition, thenValue, elseValue);
  };
  EXPECT_EQ( 42, jitExecFoo(true, 42, 77) ) << amendAst(ast);
  EXPECT_EQ( 77, jitExecFoo(false, 42, 77) ) << amendAst(ast);
}

TEST(IrGenTest, MAKE_TEST_NAME(
    a_function_declaration_and_a_matching_function_definition,
    genIr,
    succeeds)) {
  TestingIrGen UUT;
  ParserExt pe(UUT.m_env, *UUT.m_errorHandler);
  EXPECT_NO_THROW(
    UUT.genIr(
      *new AstOperator(';',
        pe.mkFunDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt)),
        pe.mkFunDef(
          pe.mkFunDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt)),
          new AstNumber(42)))));
  EXPECT_TRUE(UUT.m_errorHandler->errors().empty());
}

TEST(IrGenTest, MAKE_TEST_NAME(
    a_function_definition_in_a_function_body,
    genIrInImplicitMain,
    succeeds)) {
  
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstOperator(';',
      pe.mkFunDef(pe.mkFunDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt)),
        new AstOperator(';',
          pe.mkFunDef(
            pe.mkFunDecl("bar", new ObjTypeFunda(ObjTypeFunda::eInt)),
            new AstNumber(42)),
          new AstFunCall(new AstSymbol("bar")))),
      new AstFunCall(new AstSymbol("foo"))),
    42, "");
}

TEST(IrGenTest, MAKE_TEST_NAME(
    a_function_call_to_an_defined_function,
    genIrInImplicitMain,
    returns_result_of_that_function_call)) {

  string spec = "Trivial function with zero arguments returning a constant";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstOperator(';',
      pe.mkFunDef(
        pe.mkFunDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt)),
        new AstNumber(42)),
      new AstFunCall(new AstSymbol("foo"))),
    42, spec);

  spec = "Simple function with one argument which is ignored and a constant is returned";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstOperator(';',
      pe.mkFunDef(
        pe.mkFunDecl(
          "foo",
          new ObjTypeFunda(ObjTypeFunda::eInt),
          new AstArgDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt))),
        new AstNumber(42)),
      new AstFunCall(new AstSymbol("foo"), new AstCtList(new AstNumber(0)))),
    42, spec);

  spec = "Simple function with two arguments whichs sum is returned";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstOperator(';',
      pe.mkFunDef(
        pe.mkFunDecl(
          "add",
          new ObjTypeFunda(ObjTypeFunda::eInt),
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
    a_inmutable_data_object_definition_of_foo_being_initialized_with_x,
    genIrInImplicitMain,
    returns_x_rvalue)) {
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstDataDef(
      new AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt)),
      new AstNumber(42)),
    42, "");
}

TEST(IrGenTest, MAKE_TEST_NAME2(
    GIVEN_a_mutable_data_object_definition_expression,
    THEN_its_value_is_the_defined_data_object)) {

  string spec = "Reading from the data definition expression gives the value "
    "of the data object after initialization, which in turn equals value of the initializer.";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstDataDef(
      new AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable)),
      new AstNumber(42)),
    42, spec);

  spec = "Writing to the data definition expression modifies the defined data object.";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstOperator(';',
      new AstOperator('=',
        new AstDataDef(
          new AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable)),
          new AstNumber(42)),
        new AstNumber(77)),
      new AstSymbol("foo")),
    77, spec);
}

TEST(IrGenTest, MAKE_TEST_NAME2(
    GIVEN_a_dot_assignment_expression,
    THEN_its_value_is_the_lhs_data_object)) {

  string spec = "Reading from the dot assignment expression gives the value "
    "of the lhs after the assignment, which in turn equals value of the rhs.";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    pe.mkOperatorTree(";",
      new AstDataDef(
        new AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable))),
      new AstOperator(".=",
        new AstSymbol("foo"),
        new AstNumber(42))),
    42, spec);

  spec = "Writing to the dot assignment expression modifies the lhs data object.";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    pe.mkOperatorTree(";",
      new AstDataDef(
        new AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable))),
      new AstOperator('=',
        new AstOperator(".=",
          new AstSymbol("foo"),
          new AstNumber(42)),
        new AstNumber(77)),
      new AstSymbol("foo")),
    77, spec);
}

TEST(IrGenTest, MAKE_TEST_NAME2(
    GIVEN_a_seq_expression,
    THEN_its_value_is_the_rhs_data_object)) {

  string spec = "Reading from the seq expression gives the value of the rhs";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    pe.mkOperatorTree(";",
      new AstNumber(42),
      new AstNumber(77)),
    77, spec);

  spec = "Writing to the seq assignment expression modifies the rhs data object.";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    pe.mkOperatorTree(";",
      new AstDataDef(
        new AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable))),
      new AstOperator('=',
        new AstOperator(';',
          new AstNumber(42),
          new AstSymbol("foo")),
        new AstNumber(77)),
      new AstSymbol("foo")),
    77, spec);
}

TEST(IrGenTest, MAKE_TEST_NAME(
    foo_defined_as_data_object_followed_by_a_simple_expression_referencing_foo,
    genIrInImplicitMain,
    returns_result_of_that_expression)) {

  string spec = "value (aka immutable (aka const) data obj)";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstOperator(';',
      new AstDataDef(
        new AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt)),
        new AstNumber(42)),
      new AstOperator('+',
        new AstSymbol("foo"),
        new AstNumber(77))),
    42+77, spec);

  spec = "variable (aka mutable data obj)";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
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
    assigning_to_a_variable_and_later_referencing_the_variable,
    genIrInImplicitMain,
    returns_the_new_value_of_the_variable)) {
  string spec = "Example: using operator '='";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    pe.mkOperatorTree(";",
      new AstDataDef(
        new AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable)),
        new AstNumber(42)),
      new AstOperator('=',
        new AstSymbol("foo"),
        new AstNumber(77)),
      new AstSymbol("foo")),
    77, spec);

  spec = "Example: using operator '.='";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    pe.mkOperatorTree(";",
      new AstDataDef(
        new AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable)),
        new AstNumber(42)),
      new AstOperator(".=",
        new AstSymbol("foo"),
        new AstNumber(77)),
      new AstSymbol("foo")),
    77, spec);
}

TEST(IrGenTest, MAKE_TEST_NAME(
    a_value_definition_of_foo_with_no_explicit_initializer,
    genIrInImplicitMain,
    returns_the_default_which_is_zero)) {
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstDataDef(
      new AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable))),
    0, "");
}

TEST(IrGenTest, MAKE_TEST_NAME2(
    GIVEN_a_pseudo_global_variable_named_x_AND_a_function_definition_also_defining_a_variable_x,
    THEN_the_inner_defintion_of_variable_x_shadows_the_outer_variable_x)) {

  // 'global' in quotes because there not really global variables
  // yet. Variables defined in 'global' scope really are local variables in
  // the implicit main method. 

  string spec = "function argument named 'x' shadows 'global' variable also named 'x'";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    pe.mkOperatorTree(";",
      new AstDataDef(new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt)), new AstNumber(42)),
      pe.mkFunDef(
        pe.mkFunDecl(
          "foo",
          new ObjTypeFunda(ObjTypeFunda::eVoid),
          new AstArgDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt))),
        new AstOperator('=',
          new AstSymbol("x"),
          new AstNumber(77))),
      new AstSymbol("x")),
    42, spec);

  spec = "variable 'x' local to a function shadows 'global' variable also named 'x'";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    pe.mkOperatorTree(";",
      new AstDataDef(new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt)), new AstNumber(42)),
      pe.mkFunDef(
        pe.mkFunDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt)),
        new AstDataDef(
          new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt)),
          new AstNumber(77))),
      new AstSymbol("x")),
    42, spec);
}

TEST(IrGenTest, MAKE_TEST_NAME2(
    GIVEN_an_if_expression,
    THEN_its_value_is_the_resulting_obj_of_the_evaluated_clase)) {

  string specBase = "Reading from the if expression gives the value"
    "of the clause which is evaluated.";

  string spec = specBase + "Example: Condition evaluates to true.";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstIf(
      new AstNumber(1, ObjTypeFunda::eBool),
      new AstNumber(42),
      new AstNumber(77)),
    42, spec);

  spec = specBase + "Example: condition evaluates to false.";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstIf(
      new AstNumber(0, ObjTypeFunda::eBool),
      new AstNumber(42),
      new AstNumber(77)),
    77, spec);

  specBase = "Writing to the if expression modifies the object "
    "resulting from the evaluated clause.";
  spec = specBase + "Example: condition evaluates to true.";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    pe.mkOperatorTree(";",
      new AstDataDef(new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable))),
      new AstDataDef(new AstDataDecl("y", new ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable))),
      new AstOperator('=',
        new AstIf(
          new AstNumber(1, ObjTypeFunda::eBool),
          new AstSymbol("x"),
          new AstSymbol("y")),
        new AstNumber(42)),
      new AstSymbol("x")),
    42, spec);


  spec = specBase + "Example: condition evaluates to false.";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    pe.mkOperatorTree(";",
      new AstDataDef(new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable))),
      new AstDataDef(new AstDataDecl("y", new ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable))),
      new AstOperator('=',
        new AstIf(
          new AstNumber(0, ObjTypeFunda::eBool),
          new AstSymbol("x"),
          new AstSymbol("y")),
        new AstNumber(42)),
      new AstSymbol("y")),
    42, spec);
}

TEST(IrGenTest, MAKE_TEST_NAME2(
    GIVEN_an_return_expression,
    THEN_the_current_function_is_terminated_and_the_operands_value_is_returned_to_the_caller)) {

  string spec = "Trivial example: return expression, returning an int, at end of function body";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstReturn( new AstNumber(42)),
    42, spec);

  spec = "Example: return expression, returning an void, at end of function body";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    pe.mkOperatorTree(";",
      pe.mkFunDef(
        pe.mkFunDecl("foo", new ObjTypeFunda(ObjTypeFunda::eVoid)),
        new AstReturn(new AstNop)),
      new AstFunCall(new AstSymbol("foo")),
      new AstNumber(42)),
    42, spec);

  spec = "Example: Early return in the then clause of an if expression";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstOperator(";",
      new AstIf(
        new AstNumber(1, ObjTypeFunda::eBool),
        new AstReturn(new AstNumber(42))),
      new AstNumber(0)),
    42, spec);

  spec = "Example: Early return in the else clause of an if expression";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstOperator(";",
      new AstIf(
        new AstNumber(0, ObjTypeFunda::eBool),
        new AstNumber(0),
        new AstReturn(new AstNumber(42))),
      new AstNumber(0)),
    42, spec);
}
