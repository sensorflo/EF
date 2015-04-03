#include "test.h"
#include "../irgen.h"
#include "../executionengineadapter.h"
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
  Env m_env;
  ErrorHandler* m_errorHandler;
  SemanticAnalizer m_semanticAnalizer;
};

/** Tests the method genIr by first executing it on the given AST and then
JIT executing the given function, passing the given args, and verifying that
it returns the expected result. */
template<typename TRet, typename...TArgs>
void testgenIr(TestingIrGen& UUT, AstValue* astRoot,
  const string& spec, TRet expectedResult, const string functionName,
  TArgs...args) {

  ENV_ASSERT_TRUE( astRoot!=NULL );
  unique_ptr<AstValue> astRootAp(astRoot);
  bool unexpctedExceptionThrown = false;
  string exceptionDescription;

  try {
    // setup
    SemanticAnalizer semanticAnalizer(UUT.m_env, *UUT.m_errorHandler);
    semanticAnalizer.analyze(*astRoot);

    // execute
    auto module = UUT.genIr(*astRoot);

    // verify
    ExecutionEngineApater ee(move(module));
    TRet result = ee.jitExecFunction<TRet,TArgs...>(functionName, args...);
    EXPECT_EQ(expectedResult, result) << amendSpec(spec) << amendAst(astRoot) <<
      amend(&ee.module());
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

#define TEST_GEN_IR_IN_IMPLICIT_MAIN(astRoot, expectedResult, spec) \
  TEST_GEN_IR_0ARG(pe.mkMainFunDef(astRoot), spec, int, "main", expectedResult)

#define TEST_GEN_IR_0ARG(astRoot, spec, rettype, functionName,          \
  expectedResult)                                                       \
  {                                                                     \
    SCOPED_TRACE("testgenIr called from here (via TEST_GEN_IR)");       \
    TestingIrGen UUT;                                                   \
    ParserExt pe(UUT.m_env, *UUT.m_errorHandler);                       \
    testgenIr<rettype>(UUT, astRoot, spec, expectedResult,              \
      functionName);                                                    \
  }

#define TEST_GEN_IR_1ARG(astRoot, spec, rettype, functionName, arg1type,\
  expectedResult, arg1)                                                 \
  {                                                                     \
    SCOPED_TRACE("testgenIr called from here (via TEST_GEN_IR)");       \
    TestingIrGen UUT;                                                   \
    ParserExt pe(UUT.m_env, *UUT.m_errorHandler);                       \
    testgenIr<rettype, arg1type>(UUT, astRoot, spec, expectedResult,    \
      functionName, arg1);                                              \
  }

#define TEST_GEN_IR_2ARG(astRoot, spec, rettype, functionName, arg1type,\
  arg2type, expectedResult, arg1, arg2)                                 \
  {                                                                     \
    SCOPED_TRACE("testgenIr called from here (via TEST_GEN_IR)");       \
    TestingIrGen UUT;                                                   \
    ParserExt pe(UUT.m_env, *UUT.m_errorHandler);                       \
    testgenIr<rettype, arg1type, arg2type>(UUT, astRoot, spec,          \
      expectedResult, functionName, arg1, arg2);                        \
  }

TEST(IrGenTest, MAKE_TEST_NAME(
    a_single_literal,
    genIrInImplicitMain,
    returns_the_literal_s_value)) {
  TEST_GEN_IR_IN_IMPLICIT_MAIN(new AstNumber(42), 42, "");
}

TEST(IrGenTest, MAKE_TEST_NAME(
    a_block,
    genIrInImplicitMain,
    returns_the_blocks_bodies_value)) {

  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstBlock(new AstNumber(42)), 42, "");

  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstBlock(
      new AstDataDef(
        new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt)),
        new AstNumber(42))), 42, "");
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
    an_short_circuit_perator,
    genIrInImplicitMain,
    does_not_evaluate_rhs_operand_if_after_evaluating_lhs_operand_the_result_of_the_operator_is_already_known)) {

  string spec = "Example: lhs of 'and' operator is false -> rhs is not evaluated";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    pe.mkOperatorTree(";",
      new AstDataDef(
        new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eBool, ObjType::eMutable)),
        new AstNumber(1, ObjTypeFunda::eBool)),
      new AstOperator(AstOperator::eAnd,
        new AstNumber(0, ObjTypeFunda::eBool),
        new AstOperator(".=", new AstSymbol("x"), new AstNumber(0, ObjTypeFunda::eBool))),
      new AstCast(ObjTypeFunda::eInt, new AstSymbol("x"))),
    1, "");

  spec = "Example: lhs of 'and' operator is true -> rhs is evaluated";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    pe.mkOperatorTree(";",
      new AstDataDef(
        new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eBool, ObjType::eMutable)),
        new AstNumber(1, ObjTypeFunda::eBool)),
      new AstOperator(AstOperator::eAnd,
        new AstNumber(1, ObjTypeFunda::eBool),
        new AstOperator(".=", new AstSymbol("x"), new AstNumber(0, ObjTypeFunda::eBool))),
      new AstCast(ObjTypeFunda::eInt, new AstSymbol("x"))),
    0, "");

  spec = "Example: lhs of 'or' operator is true -> rhs is not evaluated";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    pe.mkOperatorTree(";",
      new AstDataDef(
        new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eBool, ObjType::eMutable)),
        new AstNumber(1, ObjTypeFunda::eBool)),
      new AstOperator(AstOperator::eOr,
        new AstNumber(1, ObjTypeFunda::eBool),
        new AstOperator(".=", new AstSymbol("x"), new AstNumber(0, ObjTypeFunda::eBool))),
      new AstCast(ObjTypeFunda::eInt, new AstSymbol("x"))),
    1, "");

  spec = "Example: lhs of 'or' operator is false -> rhs is evaluated";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    pe.mkOperatorTree(";",
      new AstDataDef(
        new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eBool, ObjType::eMutable)),
        new AstNumber(1, ObjTypeFunda::eBool)),
      new AstOperator(AstOperator::eOr,
        new AstNumber(0, ObjTypeFunda::eBool),
        new AstOperator(".=", new AstSymbol("x"), new AstNumber(0, ObjTypeFunda::eBool))),
      new AstCast(ObjTypeFunda::eInt, new AstSymbol("x"))),
    0, "");
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
    auto module = UUT.genIr(*ast);

    // verify
    Function* functionIr = module->getFunction("foo");
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
    auto module = UUT.genIr(*ast);

    // verify
    Function* functionIr = module->getFunction("foo");
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
    auto module = UUT.genIr(*ast);

    // verify
    Function* functionIr = module->getFunction("foo");
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
    auto module = UUT.genIr(*ast);

    // verify
    Function* functionIr = module->getFunction("foo");
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
  auto module = UUT.genIr(*ast);

  // verify
  // todo: via e.g. global variable verify that function really was called
  ExecutionEngineApater ee(move(module));
  EXPECT_NO_THROW( ee.jitExecFunction("foo"))
    << amendAst(ast) << amend(&ee.module());
}

TEST(IrGenTest, MAKE_TEST_NAME2(
    GIVEN_a_function_defintion_returning_a_value,
    THEN_JIT_executing_it_returns_that_value)) {

  string spec = "Example: zero arguments, returning a literal";
  TEST_GEN_IR_0ARG(
    pe.mkFunDef(
      pe.mkFunDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt)),
      new AstNumber(42)),
    spec, int, "foo", 42)

  spec = "Example: one argument, which however is ignored, returning "
    "a literal";
  TEST_GEN_IR_1ARG(
    pe.mkFunDef(
      pe.mkFunDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt)),
      new AstNumber(42)),
    spec, int, "foo", int, 42, 0);

  spec = "Example: one argument which is returned";
  TEST_GEN_IR_1ARG(
    pe.mkFunDef(
      pe.mkFunDecl(
        "foo",
        new ObjTypeFunda(ObjTypeFunda::eInt),
        new AstArgDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt))),
      new AstSymbol("x")),
    spec, int, "foo", int, 42, 42);

  spec = "Example: two arguments, returns the product";
  TEST_GEN_IR_2ARG(
    pe.mkFunDef(
      pe.mkFunDecl(
        "foo",
        new ObjTypeFunda(ObjTypeFunda::eInt),
        new AstArgDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt)),
        new AstArgDecl("y", new ObjTypeFunda(ObjTypeFunda::eInt))),
      new AstOperator('*',
        new AstSymbol("x"),
        new AstSymbol("y"))),
    spec, int, "foo", int, int, 3*4, 3, 4);
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
  auto module = UUT.genIr(*ast);

  // verify
  int x = 2;
  ExecutionEngineApater ee(move(module));
  EXPECT_EQ( x+1, (ee.jitExecFunction<int,int>("foo", x))) << amendAst(ast)
    << amend(&ee.module());
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
  auto module = UUT.genIr(*ast);

  // verify
  Function* functionIr = module->getFunction("foo");
  unique_ptr<ExecutionEngine> ee(EngineBuilder(module.release()).create());
  void* functionVoidPtr = ee->getPointerToFunction(functionIr);
  assert(functionVoidPtr);
  int (*functionPtr)(bool,int,int) = (int (*)(bool,int,int))(intptr_t)functionVoidPtr;
  assert(functionPtr);
  EXPECT_EQ( 42, functionPtr(true, 42, 77) ) << amendAst(ast);
  EXPECT_EQ( 77, functionPtr(false, 42, 77) ) << amendAst(ast);
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

TEST(IrGenTest, MAKE_TEST_NAME2(
    GIVEN_an_loop_expression,
    THEN_it_loops_as_long_as_condition_evaluates_to_true)) {

  string spec = "Example: Count x down from 1 to 0";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    pe.mkOperatorTree(";",
      new AstDataDef(
        new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable)),
        new AstNumber(1)),
      new AstLoop(
        new AstOperator('!', new AstOperator("==", new AstSymbol("x"), new AstNumber(0))),
        new AstOperator('=',
          new AstSymbol("x"),
          new AstOperator('-', new AstSymbol("x"), new AstNumber(1)))),
      new AstSymbol("x")),
    0, spec);
}
