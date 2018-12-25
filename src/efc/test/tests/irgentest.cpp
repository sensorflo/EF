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
void testgenIr(TestingIrGen& UUT, AstObject* astRoot,
  const string& spec, TRet expectedResult, const string fqFunctionName,
  TArgs...args) {

  ENV_ASSERT_TRUE( astRoot!=NULL );
  unique_ptr<AstObject> astRootAp(astRoot);
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
    TRet result = ee.jitExecFunction<TRet,TArgs...>(fqFunctionName, args...);
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
  TEST_GEN_IR_0ARG(pe.mkMainFunDef(astRoot), spec, int, ".main", expectedResult)

#define TEST_GEN_IR_IN_IMPLICIT_FOO_RET_BOOL(astRoot, expectedResult, spec) \
  TEST_GEN_IR_0ARG(                                                     \
    pe.mkFunDef("foo", ObjTypeFunda::eBool,                             \
      astRoot),                                                         \
    spec, bool, ".foo", expectedResult)

#define TEST_GEN_IR_IN_IMPLICIT_FOO_RET_CHAR(astRoot, expectedResult, spec) \
  TEST_GEN_IR_0ARG(                                                     \
    pe.mkFunDef("foo", ObjTypeFunda::eChar,                             \
      astRoot),                                                         \
    spec, unsigned char, ".foo", expectedResult)

#define TEST_GEN_IR_IN_IMPLICIT_FOO_RET_DOUBLE(astRoot, expectedResult, spec) \
  TEST_GEN_IR_0ARG(                                                     \
    pe.mkFunDef("foo", ObjTypeFunda::eDouble,                           \
      astRoot),                                                         \
    spec, double, ".foo", expectedResult)

#define TEST_GEN_IR_0ARG(astRoot, spec, rettype, fqFunctionName,        \
  expectedResult)                                                       \
  {                                                                     \
    SCOPED_TRACE("testgenIr called from here (via TEST_GEN_IR_0ARG)");  \
    TestingIrGen UUT;                                                   \
    ParserExt pe(UUT.m_env, *UUT.m_errorHandler);                       \
    testgenIr<rettype>(UUT, astRoot, spec, expectedResult,              \
      fqFunctionName);                                                  \
  }

#define TEST_GEN_IR_1ARG(astRoot, spec, rettype, fqFunctionName, arg1type, \
  expectedResult, arg1)                                                 \
  {                                                                     \
    SCOPED_TRACE("testgenIr called from here (via TEST_GEN_IR_1ARG)");  \
    TestingIrGen UUT;                                                   \
    ParserExt pe(UUT.m_env, *UUT.m_errorHandler);                       \
    testgenIr<rettype, arg1type>(UUT, astRoot, spec, expectedResult,    \
      fqFunctionName, arg1);                                            \
  }

#define TEST_GEN_IR_2ARG(astRoot, spec, rettype, fqFunctionName, arg1type,\
  arg2type, expectedResult, arg1, arg2)                                 \
  {                                                                     \
    SCOPED_TRACE("testgenIr called from here (via TEST_GEN_IR_2ARG)");  \
    TestingIrGen UUT;                                                   \
    ParserExt pe(UUT.m_env, *UUT.m_errorHandler);                       \
    testgenIr<rettype, arg1type, arg2type>(UUT, astRoot, spec,          \
      expectedResult, fqFunctionName, arg1, arg2);                      \
  }

TEST(IrGenTest, MAKE_TEST_NAME(
    a_single_literal,
    genIrInImplicitMain,
    returns_the_literal_s_value)) {

  string spec = "Example: char literal";
  TEST_GEN_IR_IN_IMPLICIT_FOO_RET_CHAR(
    new AstNumber('x', ObjTypeFunda::eChar),
    'x', spec);

  spec = "Example: int literal";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstNumber(42),
    42, spec);

  spec = "Example: double literal";
  TEST_GEN_IR_IN_IMPLICIT_FOO_RET_DOUBLE(
    new AstNumber(42.77, ObjTypeFunda::eDouble),
    42.77, spec);

  spec = "Example: bool literal";
  TEST_GEN_IR_IN_IMPLICIT_FOO_RET_BOOL(
    new AstNumber(1, ObjTypeFunda::eBool),
    true, spec);
}

TEST(IrGenTest, MAKE_TEST_NAME(
    a_block,
    genIrInImplicitMain,
    returns_the_blocks_bodies_value)) {

  string spec = "Example: body contains a literal";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstBlock(new AstNumber(42)), 42, spec);
}

TEST(IrGenTest, MAKE_TEST_NAME(
    an_operator,
    genIrInImplicitMain,
    returns_the_result_of_that_operator)) {

  // ; aka sequence
  string spec = "The sequence operator evaluates all arguments and returns the last";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstSeq( new AstNumber(11), new AstNumber(22)), 22, "");

  // not
  TEST_GEN_IR_IN_IMPLICIT_FOO_RET_BOOL(
    new AstOperator(AstOperator::eNot,
      new AstNumber(0, ObjTypeFunda::eBool)),
    !false, "");
  TEST_GEN_IR_IN_IMPLICIT_FOO_RET_BOOL(
    new AstOperator(AstOperator::eNot,
      new AstNumber(1, ObjTypeFunda::eBool)),
    !true, "");

  // and
  TEST_GEN_IR_IN_IMPLICIT_FOO_RET_BOOL(
    new AstOperator(AstOperator::eAnd,
      new AstNumber(0, ObjTypeFunda::eBool),
      new AstNumber(0, ObjTypeFunda::eBool)),
    false && false, "");
  TEST_GEN_IR_IN_IMPLICIT_FOO_RET_BOOL(
    new AstOperator(AstOperator::eAnd,
      new AstNumber(0, ObjTypeFunda::eBool),
      new AstNumber(1, ObjTypeFunda::eBool)),
    false && true, "");
  TEST_GEN_IR_IN_IMPLICIT_FOO_RET_BOOL(
    new AstOperator(AstOperator::eAnd,
      new AstNumber(1, ObjTypeFunda::eBool),
      new AstNumber(0, ObjTypeFunda::eBool)),
    true && false, "");
  TEST_GEN_IR_IN_IMPLICIT_FOO_RET_BOOL(
    new AstOperator(AstOperator::eAnd,
      new AstNumber(1, ObjTypeFunda::eBool),
      new AstNumber(1, ObjTypeFunda::eBool)),
    true && true, "");

  // or
  TEST_GEN_IR_IN_IMPLICIT_FOO_RET_BOOL(
    new AstOperator(AstOperator::eOr,
      new AstNumber(0, ObjTypeFunda::eBool),
      new AstNumber(0, ObjTypeFunda::eBool)),
    false || false, "");
  TEST_GEN_IR_IN_IMPLICIT_FOO_RET_BOOL(
    new AstOperator(AstOperator::eOr,
      new AstNumber(0, ObjTypeFunda::eBool),
      new AstNumber(1, ObjTypeFunda::eBool)),
    false || true, "");
  TEST_GEN_IR_IN_IMPLICIT_FOO_RET_BOOL(
    new AstOperator(AstOperator::eOr,
      new AstNumber(1, ObjTypeFunda::eBool),
      new AstNumber(0, ObjTypeFunda::eBool)),
    true || false, "");
  TEST_GEN_IR_IN_IMPLICIT_FOO_RET_BOOL(
    new AstOperator(AstOperator::eOr,
      new AstNumber(1, ObjTypeFunda::eBool),
      new AstNumber(1, ObjTypeFunda::eBool)),
    true || true, "");

  // == with int operands
  TEST_GEN_IR_IN_IMPLICIT_FOO_RET_BOOL(
    new AstOperator(AstOperator::eEqualTo, new AstNumber(2), new AstNumber(2)),
    2==2, "");
  TEST_GEN_IR_IN_IMPLICIT_FOO_RET_BOOL(
    new AstOperator(AstOperator::eEqualTo, new AstNumber(1), new AstNumber(2)),
    1==2, "");

  // == with double operands
  TEST_GEN_IR_IN_IMPLICIT_FOO_RET_BOOL(
    new AstOperator(AstOperator::eEqualTo,
      new AstNumber(42.5, ObjTypeFunda::eDouble),
      new AstNumber(42.5, ObjTypeFunda::eDouble)),
    42.5==42.5, "");
  TEST_GEN_IR_IN_IMPLICIT_FOO_RET_BOOL(
    new AstOperator(AstOperator::eEqualTo,
      new AstNumber(42.5, ObjTypeFunda::eDouble),
      new AstNumber(77.0, ObjTypeFunda::eDouble)),
    42.5==77.0, "");

  // unary - with int operands
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstOperator('-', new AstNumber(42)),
    -42, "");
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstOperator('+', new AstNumber(42)),
    +42, "");

  // unary - with double operands
  TEST_GEN_IR_IN_IMPLICIT_FOO_RET_DOUBLE(
    new AstOperator('-', new AstNumber(42.5, ObjTypeFunda::eDouble)),
    -42.5, "");
  TEST_GEN_IR_IN_IMPLICIT_FOO_RET_DOUBLE(
    new AstOperator('+', new AstNumber(42.5, ObjTypeFunda::eDouble)),
    +42.5, "");

  // binary + - * / with int operands
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

  // binary + - * / with double operands
  TEST_GEN_IR_IN_IMPLICIT_FOO_RET_DOUBLE(
    new AstOperator('+',
      new AstNumber(1.1, ObjTypeFunda::eDouble),
      new AstNumber(2.2, ObjTypeFunda::eDouble)),
    1.1+2.2, "");
  TEST_GEN_IR_IN_IMPLICIT_FOO_RET_DOUBLE(
    new AstOperator('-',
      new AstNumber(1.1, ObjTypeFunda::eDouble),
      new AstNumber(2.2, ObjTypeFunda::eDouble)),
    1.1-2.2, "");
  TEST_GEN_IR_IN_IMPLICIT_FOO_RET_DOUBLE(
    new AstOperator('*',
      new AstNumber(1.1, ObjTypeFunda::eDouble),
      new AstNumber(2.2, ObjTypeFunda::eDouble)),
    1.1*2.2, "");
  TEST_GEN_IR_IN_IMPLICIT_FOO_RET_DOUBLE(
    new AstOperator('/',
      new AstNumber(2.5, ObjTypeFunda::eDouble),
      new AstNumber(2.0, ObjTypeFunda::eDouble)),
    2.5/2.0, "");
}

TEST(IrGenTest, MAKE_TEST_NAME2(
    GIVEN_an_addrOf_or_dereference_operator,
    THEN_addrOf_returns_the_objects_address_as_pointer_AND_dereference_returns_the_object_pointed_to_by_an_pointer)) {

  string spec = "Example: addrOf followed by dereferencing is a nop";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstOperator(AstOperator::eDeref,
      new AstOperator('&',
        new AstDataDef("x",
          new AstObjTypeQuali(ObjType::eMutable, new AstObjTypeSymbol(ObjTypeFunda::eInt)),
          new AstNumber(42, ObjTypeFunda::eInt)))),
    42, "");

  spec = "Example: addrOf followed by dereferencing is a nop";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstOperator(AstOperator::eDeref,
      new AstOperator(AstOperator::eDeref,
        new AstOperator('&',
          new AstOperator('&',
            new AstDataDef("x",
              new AstObjTypeQuali(ObjType::eMutable, new AstObjTypeSymbol(ObjTypeFunda::eInt)),
              new AstNumber(42, ObjTypeFunda::eInt)))))),
    42, "");

  spec = "Example: when modifying an data object x through a pointer p to it, "
    "then x has the new value";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstSeq(
      new AstDataDef("x",
        new AstObjTypeQuali(ObjType::eMutable, new AstObjTypeSymbol(ObjTypeFunda::eInt))),
      new AstDataDef("p",
        new AstObjTypePtr(new AstObjTypeQuali(ObjType::eMutable, new AstObjTypeSymbol(ObjTypeFunda::eInt))),
        new AstOperator('&', new AstSymbol("x"))),
      new AstOperator("=",
        new AstOperator(AstOperator::eDeref, new AstSymbol("p")),
        new AstNumber(42)),
      new AstSymbol("x")),
    42, "");

  spec = "Example: Reading the value of a immutable local data object x\n"
    "through a pointer p to it.";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstSeq(
      new AstDataDef("x", ObjTypeFunda::eInt,
        new AstNumber(42, ObjTypeFunda::eInt)),
      new AstDataDef("p",
        new AstObjTypePtr(new AstObjTypeSymbol(ObjTypeFunda::eInt)),
        new AstOperator('&', new AstSymbol("x"))),
      new AstOperator(AstOperator::eDeref, new AstSymbol("p"))),
    42, "");

  spec = "Example: Address of a temporary object; a literal number";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstSeq(
      new AstDataDef("p",
        new AstObjTypePtr(new AstObjTypeSymbol(ObjTypeFunda::eInt)),
        new AstOperator('&', new AstNumber(42))),
      new AstOperator(AstOperator::eDeref, new AstSymbol("p"))),
    42, spec);

  spec = "Example: Address of a temproary object; an arithmetic operator";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstSeq(
      new AstDataDef("p",
        new AstObjTypePtr(new AstObjTypeSymbol(ObjTypeFunda::eInt)),
        new AstOperator('&',
          new AstOperator('+', new AstNumber(1), new AstNumber(2)))),
      new AstOperator(AstOperator::eDeref, new AstSymbol("p"))),
    1+2, "");
}

TEST(IrGenTest, MAKE_TEST_NAME(
    an_short_circuit_perator,
    genIrInImplicitMain,
    does_not_evaluate_rhs_operand_if_after_evaluating_lhs_operand_the_result_of_the_operator_is_already_known)) {

  string spec = "Example: lhs of 'and' operator is false -> rhs is not evaluated";
  TEST_GEN_IR_IN_IMPLICIT_FOO_RET_BOOL(
    new AstSeq(
      new AstDataDef("x",
        new AstObjTypeQuali(ObjType::eMutable, new AstObjTypeSymbol(ObjTypeFunda::eBool)),
        new AstNumber(1, ObjTypeFunda::eBool)),
      new AstOperator(AstOperator::eAnd,
        new AstNumber(0, ObjTypeFunda::eBool),
        new AstOperator("=<", new AstSymbol("x"), new AstNumber(0, ObjTypeFunda::eBool))),
      new AstSymbol("x")),
    true, "");

  spec = "Example: lhs of 'and' operator is true -> rhs is evaluated";
  TEST_GEN_IR_IN_IMPLICIT_FOO_RET_BOOL(
    new AstSeq(
      new AstDataDef("x",
        new AstObjTypeQuali(ObjType::eMutable, new AstObjTypeSymbol(ObjTypeFunda::eBool)),
        new AstNumber(1, ObjTypeFunda::eBool)),
      new AstOperator(AstOperator::eAnd,
        new AstNumber(1, ObjTypeFunda::eBool),
        new AstOperator("=<", new AstSymbol("x"), new AstNumber(0, ObjTypeFunda::eBool))),
      new AstSymbol("x")),
    false, "");

  spec = "Example: lhs of 'or' operator is true -> rhs is not evaluated";
  TEST_GEN_IR_IN_IMPLICIT_FOO_RET_BOOL(
    new AstSeq(
      new AstDataDef("x",
        new AstObjTypeQuali(ObjType::eMutable, new AstObjTypeSymbol(ObjTypeFunda::eBool)),
        new AstNumber(1, ObjTypeFunda::eBool)),
      new AstOperator(AstOperator::eOr,
        new AstNumber(1, ObjTypeFunda::eBool),
        new AstOperator("=<", new AstSymbol("x"), new AstNumber(0, ObjTypeFunda::eBool))),
      new AstSymbol("x")),
    true, "");

  spec = "Example: lhs of 'or' operator is false -> rhs is evaluated";
  TEST_GEN_IR_IN_IMPLICIT_FOO_RET_BOOL(
    new AstSeq(
      new AstDataDef("x",
        new AstObjTypeQuali(ObjType::eMutable, new AstObjTypeSymbol(ObjTypeFunda::eBool)),
        new AstNumber(1, ObjTypeFunda::eBool)),
      new AstOperator(AstOperator::eOr,
        new AstNumber(0, ObjTypeFunda::eBool),
        new AstOperator("=<", new AstSymbol("x"), new AstNumber(0, ObjTypeFunda::eBool))),
      new AstSymbol("x")),
    false, "");
}

TEST(IrGenTest, MAKE_TEST_NAME(
    a_cast,
    genIrInImplicitMain,
    succeeds)) {

  string spec = "bool -> bool: is a nop";
  TEST_GEN_IR_IN_IMPLICIT_FOO_RET_BOOL(
    new AstCast(
      new AstObjTypeSymbol(ObjTypeFunda::eBool),
      new AstNumber(1, ObjTypeFunda::eBool)),
    true, spec);

  spec = "bool -> char: false is 0";
  TEST_GEN_IR_IN_IMPLICIT_FOO_RET_CHAR(
    new AstCast(
      new AstObjTypeSymbol(ObjTypeFunda::eChar),
      new AstNumber(0, ObjTypeFunda::eBool)),
    0, spec);

  spec = "bool -> char: true is 1";
  TEST_GEN_IR_IN_IMPLICIT_FOO_RET_CHAR(
    new AstCast(
      new AstObjTypeSymbol(ObjTypeFunda::eChar),
      new AstNumber(1, ObjTypeFunda::eBool)),
    1, spec);

  spec = "bool -> int: false is 0";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstCast(
      new AstObjTypeSymbol(ObjTypeFunda::eInt),
      new AstNumber(0, ObjTypeFunda::eBool)),
    0, spec);

  spec = "bool -> int: true is 1";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstCast(
      new AstObjTypeSymbol(ObjTypeFunda::eInt),
      new AstNumber(1, ObjTypeFunda::eBool)),
    1, spec);

  spec = "bool -> double: true is 1.0";
  TEST_GEN_IR_IN_IMPLICIT_FOO_RET_DOUBLE(
    new AstCast(
      new AstObjTypeSymbol(ObjTypeFunda::eDouble),
      new AstNumber(1, ObjTypeFunda::eBool)),
    1.0, spec);

  spec = "char -> char: is a nop";
  TEST_GEN_IR_IN_IMPLICIT_FOO_RET_CHAR(
    new AstCast(
      new AstObjTypeSymbol(ObjTypeFunda::eChar),
      new AstNumber('x', ObjTypeFunda::eChar)),
    'x', spec);

  spec = "char (above 128) -> int";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstCast(
      new AstObjTypeSymbol(ObjTypeFunda::eInt),
      new AstNumber(200, ObjTypeFunda::eChar)),
    200, spec);

  spec = "char (above 128) -> double";
  TEST_GEN_IR_IN_IMPLICIT_FOO_RET_DOUBLE(
    new AstCast(
      new AstObjTypeSymbol(ObjTypeFunda::eDouble),
      new AstNumber(200, ObjTypeFunda::eChar)),
    200.0, spec);

  spec = "int -> bool: 0 is false";
  TEST_GEN_IR_IN_IMPLICIT_FOO_RET_BOOL(
    new AstCast(
      new AstObjTypeSymbol(ObjTypeFunda::eBool),
      new AstNumber(0, ObjTypeFunda::eInt)),
    false, spec);

  spec = "int -> bool: not 0 is true";
  TEST_GEN_IR_IN_IMPLICIT_FOO_RET_BOOL(
    new AstCast(
      new AstObjTypeSymbol(ObjTypeFunda::eBool),
      new AstNumber(42, ObjTypeFunda::eInt)),
    true, spec);

  spec = "int -> char";
  TEST_GEN_IR_IN_IMPLICIT_FOO_RET_CHAR(
    new AstCast(
      new AstObjTypeSymbol(ObjTypeFunda::eChar),
      new AstNumber(200, ObjTypeFunda::eInt)),
    200, spec);

  spec = "int -> int: is a nop";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstCast(
      new AstObjTypeSymbol(ObjTypeFunda::eInt),
      new AstNumber(42, ObjTypeFunda::eInt)),
    42, spec);

  spec = "int -> double";
  TEST_GEN_IR_IN_IMPLICIT_FOO_RET_DOUBLE(
    new AstCast(
      new AstObjTypeSymbol(ObjTypeFunda::eDouble),
      new AstNumber(42, ObjTypeFunda::eInt)),
    42.0, spec);

  spec = "double -> bool: 0.0 is false";
  TEST_GEN_IR_IN_IMPLICIT_FOO_RET_BOOL(
    new AstCast(
      new AstObjTypeSymbol(ObjTypeFunda::eBool),
      new AstNumber(0.0, ObjTypeFunda::eDouble)),
    false, spec);

  spec = "double -> bool: not 0.0 is true";
  TEST_GEN_IR_IN_IMPLICIT_FOO_RET_BOOL(
    new AstCast(
      new AstObjTypeSymbol(ObjTypeFunda::eBool),
      new AstNumber(42.77, ObjTypeFunda::eDouble)),
    true, spec);

  spec = "double -> char";
  TEST_GEN_IR_IN_IMPLICIT_FOO_RET_CHAR(
    new AstCast(
      new AstObjTypeSymbol(ObjTypeFunda::eChar),
      new AstNumber(200.22, ObjTypeFunda::eDouble)),
    200, spec);

  spec = "double -> int";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstCast(
      new AstObjTypeSymbol(ObjTypeFunda::eInt),
      new AstNumber(42.77, ObjTypeFunda::eDouble)),
    42, spec);

  spec = "double -> double: is a nop";
  TEST_GEN_IR_IN_IMPLICIT_FOO_RET_DOUBLE(
    new AstCast(
      new AstObjTypeSymbol(ObjTypeFunda::eDouble),
      new AstNumber(42.77, ObjTypeFunda::eDouble)),
    42.77, spec);
}


TEST(IrGenTest, MAKE_TEST_NAME2(
    GIVEN_a_dot_assignment_expression,
    THEN_its_value_is_the_lhs_data_object)) {

  string spec = "Reading from the dot assignment expression gives the value "
    "of the lhs after the assignment, which in turn equals value of the rhs.";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstSeq(
      new AstDataDef("foo",
        new AstObjTypeQuali(ObjType::eMutable, new AstObjTypeSymbol(ObjTypeFunda::eInt))),
      new AstOperator("=<",
        new AstSymbol("foo"),
        new AstNumber(42))),
    42, spec);

  spec = "Writing to the dot assignment expression modifies the lhs data object.";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstSeq(
      new AstDataDef("foo",
        new AstObjTypeQuali(ObjType::eMutable, new AstObjTypeSymbol(ObjTypeFunda::eInt))),
      new AstOperator('=',
        new AstOperator("=<",
          new AstSymbol("foo"),
          new AstNumber(42)),
        new AstNumber(77)),
      new AstSymbol("foo")),
    77, spec);
}

TEST(IrGenTest, MAKE_TEST_NAME2(
    GIVEN_a_seq_expression,
    THEN_its_lhs_is_evaluated_AND_then_its_rhs_is_evaluated_and_its_value_is_returned)) {

  string spec = "Reading from the seq expression gives the value of the rhs";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstSeq(
      new AstNumber(42),
      new AstNumber(77)),
    77, spec);

  spec = "Writing to the seq assignment expression modifies the rhs data object.";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstSeq(
      new AstDataDef("foo",
        new AstObjTypeQuali(ObjType::eMutable, new AstObjTypeSymbol(ObjTypeFunda::eInt))),
      new AstOperator('=',
        new AstSeq(
          new AstNumber(42),
          new AstSymbol("foo")),
        new AstNumber(77)),
      new AstSymbol("foo")),
    77, spec);
}

TEST(IrGenTest, MAKE_TEST_NAME(
    a_seq_with_some_expressions_not_having_a_value_but_the_last_having_a_value,
    genIrInImplicitMain,
    returns_the_result_of_the_last_expression)) {
  // the normal use case of the sequence operator is tested in a separte test
  // together with all other operators

  string spec = "Sequence containing a function definition";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstSeq(
      pe.mkFunDef("foo", ObjTypeFunda::eInt, new AstNumber(0)),
      new AstNumber(42)),
    42, spec);
}

TEST(IrGenTest, MAKE_TEST_NAME(
    a_function_definition,
    genIr,
    adds_the_function_definition_to_the_module_with_the_correct_signature)) {

  string spec = "Example: zero arguments";
  {
    // setup
    // IrGen is currently dumb and expects an expression having a value at
    // the end of a seq, thus provide one altought not needed for this test
    TestingIrGen UUT;
    ParserExt pe(UUT.m_env, *UUT.m_errorHandler);
    unique_ptr<AstObject> ast(
      pe.mkFunDef("foo", ObjTypeFunda::eInt, new AstNumber(42)));
    UUT.m_semanticAnalizer.analyze(*ast.get());

    // execute
    auto module = UUT.genIr(*ast);

    // verify
    Function* functionIr = module->getFunction(".foo");
    EXPECT_TRUE(functionIr!=NULL)
      << amendAst(ast) << amendSpec(spec);
    EXPECT_EQ(Type::getInt32Ty(llvmContext), functionIr->getReturnType())
      << amendAst(ast) << amendSpec(spec);
    EXPECT_EQ(functionIr->arg_size(), 0U)
      << amendAst(ast) << amendSpec(spec);
  }

  spec = "Example: two arguments";
  {
    // setup
    // IrGen is currently dumb and expects an expression having a value at
    // the end of a seq, thus provide one altought not needed for this test
    TestingIrGen UUT;
    ParserExt pe(UUT.m_env, *UUT.m_errorHandler);
    unique_ptr<AstObject> ast(
      pe.mkFunDef(
        "foo",
        AstFunDef::createArgs(
          new AstDataDef("arg1", ObjTypeFunda::eInt),
          new AstDataDef("arg2", ObjTypeFunda::eInt)),
        new AstObjTypeSymbol(ObjTypeFunda::eInt),
        new AstNumber(42)));
    UUT.m_semanticAnalizer.analyze(*ast.get());

    // execute
    auto module = UUT.genIr(*ast);

    // verify
    Function* functionIr = module->getFunction(".foo");
    EXPECT_TRUE(functionIr!=NULL)
      << amendAst(ast) << amendSpec(spec);
    EXPECT_EQ(Type::getInt32Ty(llvmContext), functionIr->getReturnType())
      << amendAst(ast) << amendSpec(spec);
    EXPECT_EQ(functionIr->arg_size(), 2U)
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
    unique_ptr<AstObject> ast(
      pe.mkFunDef("foo", ObjTypeFunda::eInt,
        new AstNumber(77)));
    UUT.m_semanticAnalizer.analyze(*ast.get());

    // execute
    auto module = UUT.genIr(*ast);

    // verify
    Function* functionIr = module->getFunction(".foo");
    EXPECT_TRUE(functionIr!=NULL)
      << amendAst(ast) << amendSpec(spec);
    EXPECT_EQ(Type::getInt32Ty(llvmContext), functionIr->getReturnType())
      << amendAst(ast) << amendSpec(spec);
    EXPECT_EQ(functionIr->arg_size(), 0U)
      << amendAst(ast) << amendSpec(spec);
  }

  spec = "Example: 1 int arg, 1 bool arg, ret type void";
  {
    // setup
    // IrGen is currently dumb and expects an expression having a value at
    // the end of a seq, thus provide one altought not needed for this test
    TestingIrGen UUT;
    ParserExt pe(UUT.m_env, *UUT.m_errorHandler);
    unique_ptr<AstObject> ast(
      pe.mkFunDef("foo",
        AstFunDef::createArgs(
          new AstDataDef("arg1", ObjTypeFunda::eInt),
          new AstDataDef("arg2", ObjTypeFunda::eBool)),
        new AstObjTypeSymbol(ObjTypeFunda::eVoid),
        new AstNop()));
    UUT.m_semanticAnalizer.analyze(*ast.get());

    // execute
    auto module = UUT.genIr(*ast);

    // verify
    Function* functionIr = module->getFunction(".foo");
    EXPECT_TRUE(functionIr!=NULL)
      << amendAst(ast) << amendSpec(spec);
    EXPECT_EQ(Type::getVoidTy(llvmContext), functionIr->getReturnType())
      << amendAst(ast) << amendSpec(spec);
    EXPECT_EQ(2U, functionIr->arg_size())
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
  unique_ptr<AstObject> ast(
    pe.mkFunDef("foo", ObjTypeFunda::eVoid,
      new AstNop()));
  UUT.m_semanticAnalizer.analyze(*ast.get());

  // execute
  auto module = UUT.genIr(*ast);

  // verify
  // todo: via e.g. global variable verify that function really was called
  ExecutionEngineApater ee(move(module));
  EXPECT_NO_THROW( ee.jitExecFunction(".foo"))
    << amendAst(ast) << amend(&ee.module());
}

TEST(IrGenTest, MAKE_TEST_NAME2(
    GIVEN_a_function_defintion_returning_a_value,
    THEN_JIT_executing_it_returns_that_value)) {

  string spec = "Example: zero arguments, returning a literal";
  TEST_GEN_IR_0ARG(
    pe.mkFunDef("foo", ObjTypeFunda::eInt,
      new AstNumber(42)),
    spec, int, ".foo", 42)

  spec = "Example: one argument which is returned";
  TEST_GEN_IR_1ARG(
    pe.mkFunDef("foo",
      AstFunDef::createArgs(
        new AstDataDef("x", ObjTypeFunda::eInt)),
      new AstObjTypeSymbol(ObjTypeFunda::eInt),
      new AstSymbol("x")),
    spec, int, ".foo", int, 42, 42);

  spec = "Example: two arguments, returns the product";
  TEST_GEN_IR_2ARG(
    pe.mkFunDef("foo",
      AstFunDef::createArgs(
        new AstDataDef("x", ObjTypeFunda::eInt),
        new AstDataDef("y", ObjTypeFunda::eInt)),
      new AstObjTypeSymbol(ObjTypeFunda::eInt),
      new AstOperator('*',
        new AstSymbol("x"),
        new AstSymbol("y"))),
    spec, int, ".foo", int, int, 3*4, 3, 4);

  spec = "Example: multiple arguments, mixed argument types";
  TEST_GEN_IR_2ARG(
    pe.mkFunDef("foo",
      AstFunDef::createArgs(
        new AstDataDef("condition", ObjTypeFunda::eBool),
        new AstDataDef("thenValue", ObjTypeFunda::eInt)),
      new AstObjTypeSymbol(ObjTypeFunda::eInt),
      new AstIf(
        new AstSymbol("condition"),
        new AstSymbol("thenValue"),
        new AstNumber(77))),
    spec, int, ".foo", bool, int, true ? 42 : 77, true, 42);

  spec = "Example: nested function with same name as an outer function, calling outer";
  TEST_GEN_IR_0ARG(
    pe.mkFunDef("foo", ObjTypeFunda::eInt,
      new AstSeq(
        pe.mkFunDef("foo", ObjTypeFunda::eInt, new AstNumber(42)),
        new AstNumber(77))),
    spec, int, ".foo", 77);

  spec = "Example: nested function with same name as an outer function, calling inner";
  TEST_GEN_IR_0ARG(
    pe.mkFunDef("foo", ObjTypeFunda::eInt,
      new AstSeq(
        pe.mkFunDef("foo", ObjTypeFunda::eInt, new AstNumber(42)),
        new AstNumber(77))),
    spec, int, ".foo.foo", 42);
}

TEST(IrGenTest, MAKE_TEST_NAME(
    a_call_to_a_terminating_recursive_function,
    genIrInImplicitMain,
    returns_result_of_that_function_call)) {
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstSeq(
      pe.mkFunDef("fact",
        AstFunDef::createArgs(
          new AstDataDef("x", ObjTypeFunda::eInt)),
        new AstObjTypeSymbol(ObjTypeFunda::eInt),
        new AstIf(
          new AstOperator("==", new AstSymbol("x"), new AstNumber(0)),
          new AstNumber(1),
          new AstOperator(AstOperator::eMul,
            new AstSymbol("x"),
            new AstFunCall(
              new AstSymbol("fact"),
              new AstCtList(new AstOperator("-", new AstSymbol("x"), new AstNumber(1))))))),
      new AstFunCall(new AstSymbol("fact"), new AstCtList(new AstNumber(2)))),
    2*1, "");
}

TEST(IrGenTest, MAKE_TEST_NAME2(
    GIVEN_a_function_call_before_that_functions_defintion,
    THEN_that_changes_nothing)) {

  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstSeq(
      new AstDataDef("x", ObjTypeFunda::eInt,
        new AstFunCall(new AstSymbol("foo"))),
      pe.mkFunDef("foo", ObjTypeFunda::eInt,
        new AstNumber(42)),
      new AstSymbol("x")),
    42, "");
}

TEST(IrGenTest, MAKE_TEST_NAME(
    a_nested_function_definition_AND_calls_to_both,
    genIrInImplicitMain,
    succeeds)) {

  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstSeq(
      pe.mkFunDef("foo", ObjTypeFunda::eInt,
        new AstSeq(
          pe.mkFunDef("bar", ObjTypeFunda::eInt,
            new AstNumber(42)),
          new AstFunCall(new AstSymbol("bar")))),
      new AstFunCall(new AstSymbol("foo"))),
    42, "");
}

TEST(IrGenTest, MAKE_TEST_NAME(
    a_immutable_local_data_object_definition_of_foo_being_initialized_with_x,
    genIrInImplicitMain,
    returns_x_rvalue)) {
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstDataDef("foo", ObjTypeFunda::eInt,
      new AstNumber(42)),
    42, "");
}

TEST(IrGenTest, MAKE_TEST_NAME(
    a_data_objec_definition_WITH_noinit,
    genIr,
    it_succeeds)) {
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstSeq(
      pe.mkFunDef("foo",
        AstFunDef::createArgs(new AstDataDef("x", ObjTypeFunda::eInt)),
        new AstObjTypeSymbol(ObjTypeFunda::eInt),
        new AstOperator('/', new AstSymbol("x"), new AstSymbol("x"))),
      new AstDataDef("x", ObjTypeFunda::eInt, AstDataDef::noInit),
      new AstFunCall(new AstSymbol("foo"), new AstCtList(new AstSymbol("x")))),
    1 /* x/x is always 1 */, "");
}

TEST(IrGenTest, MAKE_TEST_NAME(
    GIVEN_a_mutable_local_data_object_definition_expression,
    genIrInImplicitMain,
    returns_x_lvalue)) {

  string spec = "Reading from the data definition expression gives the value "
    "of the data object after initialization, which in turn equals value of the initializer.";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstDataDef("foo",
      new AstObjTypeQuali(ObjType::eMutable, new AstObjTypeSymbol(ObjTypeFunda::eInt)),
      new AstNumber(42)),
    42, spec);

  spec = "Writing to the data object modifies it";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstSeq(
      new AstDataDef("foo",
        new AstObjTypeQuali(ObjType::eMutable, new AstObjTypeSymbol(ObjTypeFunda::eInt)),
        new AstNumber(42)),
      new AstOperator('=',
        new AstSymbol("foo"),
        new AstNumber(77)),
      new AstSymbol("foo")),
    77, spec);

  spec = "Writing to the data definition expression modifies the defined data object.";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstSeq(
      new AstOperator('=',
        new AstDataDef("foo",
          new AstObjTypeQuali(ObjType::eMutable, new AstObjTypeSymbol(ObjTypeFunda::eInt)),
          new AstNumber(42)),
        new AstNumber(77)),
      new AstSymbol("foo")),
    77, spec);
}

TEST(IrGenTest, MAKE_TEST_NAME(
    a_static_data_object_definition_being_initialized_with_x,
    genIrInImplicitMain,
    returns_x)) {
  string spec = "Example: immutable static data object";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstDataDef("foo",
      new AstObjTypeSymbol(ObjTypeFunda::eInt), StorageDuration::eStatic,
      new AstNumber(42)),
    42, "");

  spec = "Example: mutable static data object";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstDataDef("foo",
      new AstObjTypeQuali(ObjType::eMutable, new AstObjTypeSymbol(ObjTypeFunda::eInt)), StorageDuration::eStatic,
      new AstNumber(42)),
    42, "");
}

TEST(IrGenTest, MAKE_TEST_NAME3(
    GIVEN_a_static_data_object_definition,
    THEN_the_initialization_is_ignored_at_runtime_when_control_flow_reaches_it,
    BECAUSE_initialization_of_an_static_data_object_is_done_at_program_startup)) {

  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstSeq(
      new AstDataDef("spy_sum",
        new AstObjTypeQuali(ObjType::eMutable, new AstObjTypeSymbol(ObjTypeFunda::eInt)),
        StorageDuration::eStatic),

      pe.mkFunDef("foo", ObjTypeFunda::eVoid,
        new AstSeq(
          // Init UUT with true. This test is about that this happens _not_
          // when control flow reaches this place
          new AstDataDef("UUT",
            new AstObjTypeQuali(ObjTypeFunda::eMutable, new AstObjTypeSymbol(ObjTypeFunda::eBool)),
            StorageDuration::eStatic,
            new AstNumber(1, ObjTypeFunda::eBool)),
          new AstIf(
            new AstSymbol("UUT"), // true only the first time ...
            new AstSeq(
              new AstOperator('=',  // ... since the then clause sets UUT to false
                new AstSymbol("UUT"),
                new AstNumber(0, ObjTypeFunda::eBool)),
              new AstOperator('=',
                new AstSymbol("spy_sum"),
                new AstOperator('+', new AstSymbol("spy_sum"), new AstNumber(1))))))),

      new AstFunCall(new AstSymbol("foo")), // 'spy_sum' should be increased
      new AstFunCall(new AstSymbol("foo")), // 'spy_sum' should _not_ be increased
      new AstSymbol("spy_sum")),            // should have been increased once
                                            // -> value should be 1
    1, "");
}

TEST(IrGenTest, MAKE_TEST_NAME2(
    GIVEN_a_reference_to_an_static_data_object_before_its_defintion,
    THEN_that_changes_nothing)) {

  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstSeq(
      new AstDataDef("x", ObjTypeFunda::eInt,
        new AstSymbol("foo")),
      new AstDataDef("foo",
        new AstObjTypeSymbol(ObjTypeFunda::eInt),
        StorageDuration::eStatic,
        new AstNumber(42)),
      new AstSymbol("x")),
    42, "");
}

TEST(IrGenTest, MAKE_TEST_NAME2(
    GIVEN_a_data_object_defintion_with_an_explict_or_implicit_initialization,
    THEN_a_later_read_of_that_data_object_delivers_its_current_value)) {

  string spec = "Example: local immutable data object";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstSeq(
      new AstDataDef("x", ObjTypeFunda::eInt,
        new AstNumber(42)),
      new AstSymbol("x")),
    42, spec);

  spec = "Example: local mutable data object";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstSeq(
      new AstDataDef("x",
        new AstObjTypeQuali(ObjType::eMutable, new AstObjTypeSymbol(ObjTypeFunda::eInt)),
        new AstNumber(42)),
      new AstSymbol("x")),
    42, spec);

  spec = "Example: static immutable data object";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstSeq(
      new AstDataDef("x",
        new AstObjTypeSymbol(ObjTypeFunda::eInt),
        StorageDuration::eStatic,
        new AstNumber(42)),
      new AstSymbol("x")),
    42, spec);

  spec = "Example: static mutable data object";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstSeq(
      new AstDataDef("x",
        new AstObjTypeQuali(ObjType::eMutable, new AstObjTypeSymbol(ObjTypeFunda::eInt)),
        StorageDuration::eStatic,
        new AstNumber(42)),
      new AstSymbol("x")),
    42, spec);
}

TEST(IrGenTest, MAKE_TEST_NAME2(
    GIVEN_an_assignmnt_to_a_mutable_data_object,
    THEN_a_later_read_of_of_that_data_object_delivers_the_new_value)) {

  string spec = "Example: local data object and using operator '='";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstSeq(
      new AstDataDef("foo",
        new AstObjTypeQuali(ObjType::eMutable, new AstObjTypeSymbol(ObjTypeFunda::eInt)),
        new AstNumber(42)),
      new AstOperator('=',
        new AstSymbol("foo"),
        new AstNumber(77)),
      new AstSymbol("foo")),
    77, spec);

  spec = "Example: local data object and using operator '=<'";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstSeq(
      new AstDataDef("foo",
        new AstObjTypeQuali(ObjType::eMutable, new AstObjTypeSymbol(ObjTypeFunda::eInt)),
        new AstNumber(42)),
      new AstOperator("=<",
        new AstSymbol("foo"),
        new AstNumber(77)),
      new AstSymbol("foo")),
    77, spec);

  spec = "Example: static data object and using operator '='";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstSeq(
      new AstDataDef("foo",
        new AstObjTypeQuali(ObjType::eMutable, new AstObjTypeSymbol(ObjTypeFunda::eInt)),
        StorageDuration::eStatic,
        new AstNumber(42)),
      new AstOperator('=',
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
    new AstDataDef("foo",
      new AstObjTypeQuali(ObjType::eMutable, new AstObjTypeSymbol(ObjTypeFunda::eInt))),
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
    new AstSeq(
      new AstDataDef("x", ObjTypeFunda::eInt, new AstNumber(42)),
      pe.mkFunDef("foo",
        AstFunDef::createArgs(
          new AstDataDef("x",
            new AstObjTypeQuali(ObjType::eMutable, new AstObjTypeSymbol(ObjTypeFunda::eInt)))),
        new AstObjTypeSymbol(ObjTypeFunda::eVoid),
        new AstOperator('=',
          new AstSymbol("x"),
          new AstNumber(77))),
      new AstSymbol("x")),
    42, spec);

  spec = "variable 'x' local to a function shadows 'global' variable also named 'x'";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstSeq(
      new AstDataDef("x", ObjTypeFunda::eInt, new AstNumber(42)),
      pe.mkFunDef("foo", ObjTypeFunda::eInt,
        new AstDataDef("x", ObjTypeFunda::eInt,
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
    new AstSeq(
      pe.mkFunDef("foo", ObjTypeFunda::eVoid,
        new AstReturn()),
      new AstFunCall(new AstSymbol("foo")),
      new AstNumber(42)),
    42, spec);

  spec = "Example: Early return in the then clause of an if expression";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstSeq(
      new AstIf(
        new AstNumber(1, ObjTypeFunda::eBool),
        new AstReturn(new AstNumber(42))),
      new AstNumber(0)),
    42, spec);

  spec = "Example: Early return in the else clause of an if expression";
  TEST_GEN_IR_IN_IMPLICIT_MAIN(
    new AstSeq(
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
    new AstSeq(
      new AstDataDef("x",
        new AstObjTypeQuali(ObjType::eMutable, new AstObjTypeSymbol(ObjTypeFunda::eInt)),
        new AstNumber(1)),
      new AstLoop(
        new AstOperator('!', new AstOperator("==", new AstSymbol("x"), new AstNumber(0))),
        new AstOperator('=',
          new AstSymbol("x"),
          new AstOperator('-', new AstSymbol("x"), new AstNumber(1)))),
      new AstSymbol("x")),
    0, spec);
}
