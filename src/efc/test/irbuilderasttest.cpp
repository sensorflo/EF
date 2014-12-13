#include "test.h"
#include "../irbuilderast.h"
#include "../parserext.h"
#include "llvm/IR/Module.h"
#include <memory>
using namespace testing;
using namespace std;
using namespace llvm;

enum ECmpOp {
  eEq, eNe, eLt, eGt, eGe, eLe
};

class TestingIrBuilderAst : public IrBuilderAst {
public:
  TestingIrBuilderAst() : IrBuilderAst( *(m_env = new Env()) ) {};
  ~TestingIrBuilderAst() { delete m_env; };
  using IrBuilderAst::jitExecFunction;
  using IrBuilderAst::jitExecFunction1Arg;
  using IrBuilderAst::jitExecFunction2Arg;
  using IrBuilderAst::m_module;
  Env* m_env;
};

string amendAst(const AstNode* ast) {
  assert(ast);
  return string("\nInput AST in its canonical form:\n") + ast->toStr() + "\n";
}

string amendAst(const auto_ptr<AstSeq>& ast) {
  return amendAst(ast.get());
}

void testbuilAndRunModule(AstSeq* astSeq, int expectedResult,
  const string& spec = "", ECmpOp cmpOp = eEq ) {

  // setup
  ENV_ASSERT_TRUE( astSeq!=NULL );
  auto_ptr<AstSeq> astSeqAp(astSeq);
  TestingIrBuilderAst UUT;

  // execute
  int result = UUT.buildAndRunModule(*astSeq);

  // verify
  switch (cmpOp) {
  case eEq: EXPECT_EQ(expectedResult, result) << amendSpec(spec) << amendAst(astSeq); break;
  case eNe: EXPECT_NE(expectedResult, result) << amendSpec(spec) << amendAst(astSeq); break;
  case eGt: EXPECT_GT(expectedResult, result) << amendSpec(spec) << amendAst(astSeq); break;
  case eLt: EXPECT_LT(expectedResult, result) << amendSpec(spec) << amendAst(astSeq); break;
  case eGe: EXPECT_GE(expectedResult, result) << amendSpec(spec) << amendAst(astSeq); break;
  case eLe: EXPECT_LE(expectedResult, result) << amendSpec(spec) << amendAst(astSeq); break;
  }
}

#define TEST_BUILD_AND_RUN_MODULE(astSeq, expectedResult, spec) \
  {\
    SCOPED_TRACE("testbuilAndRunModule called from here (via TEST_BUILD_AND_RUN_MODULE)"); \
    testbuilAndRunModule(astSeq, expectedResult, spec);\
  }

#define TEST_BUILD_AND_RUN_MODULE_CMPOP(astSeq, expectedResult, spec, cmpop) \
  {\
    SCOPED_TRACE("testbuilAndRunModule called from here (via TEST_BUILD_AND_RUN_MODULE_CMPOP)"); \
    testbuilAndRunModule(astSeq, expectedResult, spec, cmpop);  \
  }

TEST(IrBuilderAstTest, MAKE_TEST_NAME(
    a_seq_with_one_or_more_numbers,
    buildAndRunModule,
    returns_the_last_number)) {
  TEST_BUILD_AND_RUN_MODULE(new AstSeq(new AstNumber(42)), 42, "");
  TEST_BUILD_AND_RUN_MODULE(new AstSeq(new AstNumber(11), new AstNumber(22)), 22, "");
}

TEST(IrBuilderAstTest, MAKE_TEST_NAME(
    an_operator,
    buildAndRunModule,
    returns_the_result_of_that_operator)) {

  // not
  TEST_BUILD_AND_RUN_MODULE(
    new AstSeq(new AstOperator(AstOperator::eNot, new AstNumber(2))), 0, "");
  TEST_BUILD_AND_RUN_MODULE_CMPOP(
    new AstSeq(new AstOperator(AstOperator::eNot, new AstNumber(0))), 0, "", eNe);

  // and
  TEST_BUILD_AND_RUN_MODULE(
    new AstSeq(new AstOperator(AstOperator::eAnd, new AstNumber(0), new AstNumber(0))), 0, "");
  TEST_BUILD_AND_RUN_MODULE(
    new AstSeq(new AstOperator(AstOperator::eAnd, new AstNumber(0), new AstNumber(2))), 0, "");
  TEST_BUILD_AND_RUN_MODULE(
    new AstSeq(new AstOperator(AstOperator::eAnd, new AstNumber(2), new AstNumber(0))), 0, "");
  TEST_BUILD_AND_RUN_MODULE_CMPOP(
    new AstSeq(new AstOperator(AstOperator::eAnd, new AstNumber(2), new AstNumber(2))), 0, "", eNe);

  // or
  TEST_BUILD_AND_RUN_MODULE(
    new AstSeq(new AstOperator(AstOperator::eOr, new AstNumber(0), new AstNumber(0))), 0, "");
  TEST_BUILD_AND_RUN_MODULE_CMPOP(
    new AstSeq(new AstOperator(AstOperator::eOr, new AstNumber(0), new AstNumber(2))), 0, "", eNe);
  TEST_BUILD_AND_RUN_MODULE_CMPOP(
    new AstSeq(new AstOperator(AstOperator::eOr, new AstNumber(2), new AstNumber(0))), 0, "", eNe);
  TEST_BUILD_AND_RUN_MODULE_CMPOP(
    new AstSeq(new AstOperator(AstOperator::eOr, new AstNumber(2), new AstNumber(2))), 0, "", eNe);

  // + - * /
  TEST_BUILD_AND_RUN_MODULE(
    new AstSeq(new AstOperator('+', new AstNumber(1), new AstNumber(2))),
    1+2, "");
  TEST_BUILD_AND_RUN_MODULE(
    new AstSeq(new AstOperator('-', new AstNumber(1), new AstNumber(2))),
    1-2, "");
  TEST_BUILD_AND_RUN_MODULE(
    new AstSeq(new AstOperator('*', new AstNumber(1), new AstNumber(2))),
    1*2, "");
  TEST_BUILD_AND_RUN_MODULE(
    new AstSeq(new AstOperator('/', new AstNumber(6), new AstNumber(3))),
    6/3, "");
}

TEST(IrBuilderAstTest, MAKE_TEST_NAME(
    an_AstOperator_like_it_results_from_op_call_syntax_constructs,
    buildAndRunModule,
    returns_the_correct_result)) {

  // such operators are
  // - n-ary
  // - have AstSeq as childs

  //null-ary not "!()" is invalid 
  string spec = "null-ary or: or() = false";
  TEST_BUILD_AND_RUN_MODULE(
    new AstSeq(new AstOperator(AstOperator::eOr)),
    0, spec);
  spec = "null-ary and: and() = true";
  TEST_BUILD_AND_RUN_MODULE_CMPOP(
    new AstSeq(new AstOperator(AstOperator::eAnd)),
    0, spec, eNe);
  spec = "null-ary minus: -() = 0";
  TEST_BUILD_AND_RUN_MODULE(
    new AstSeq(new AstOperator('-')),
    0, spec);
  spec = "null-ary minus: +() = 0";
  TEST_BUILD_AND_RUN_MODULE(
    new AstSeq(new AstOperator('+')),
    0, spec);
  spec = "null-ary mult: *() = 1";
  TEST_BUILD_AND_RUN_MODULE(
    new AstSeq(new AstOperator('*')),
    1, spec);
  //null-ary div "/()" is invalid


  // unary not "!(x)" is the normal case and has been tested above
  spec = "unary or: or(x) = bool(x)";
  TEST_BUILD_AND_RUN_MODULE_CMPOP(
    new AstSeq(
      new AstOperator(AstOperator::eOr,
        new AstNumber(2))),
    0, spec, eNe);
  spec = "unary and: and(x) = bool(x)";
  TEST_BUILD_AND_RUN_MODULE_CMPOP(
    new AstSeq(
      new AstOperator(AstOperator::eAnd,
        new AstNumber(2))),
    0, spec, eNe);
  spec = "unary minus: -(x)";
  TEST_BUILD_AND_RUN_MODULE(
    new AstSeq(
      new AstOperator('-',
        new AstNumber(1))),
    -1, spec);
  spec = "unary plus: +(x) = x";
  TEST_BUILD_AND_RUN_MODULE(
    new AstSeq(
      new AstOperator('+',
        new AstNumber(1))),
    +1, spec);
  spec = "spec = unary mult: *(x) = x";
  TEST_BUILD_AND_RUN_MODULE(
    new AstSeq(
      new AstOperator('*',
        new AstNumber(1))),
    1, spec);
  // unary div "/(x)" is invalid

  // n-ary not "!(x y z)" is invalid
  spec = "n-ary plus: +(1,2,3)";
  TEST_BUILD_AND_RUN_MODULE(
    new AstSeq(
      new AstOperator('+',
        new AstNumber(1),
        new AstNumber(2),
        new AstNumber(3))),
    1+2+3, spec);
  spec = "n-ary minus: -(1,2,3)";
  TEST_BUILD_AND_RUN_MODULE(
    new AstSeq(
      new AstOperator('-',
        new AstNumber(1),
        new AstNumber(2),
        new AstNumber(3))),
    1-2-3, spec);
  spec = "n-ary mult: *(1,2,3)";
  TEST_BUILD_AND_RUN_MODULE(
    new AstSeq(
      new AstOperator('*',
        new AstNumber(1),
        new AstNumber(2),
        new AstNumber(3))),
    1*2*3, spec);
  spec = "n-ary div: /(24,4,3)";
  TEST_BUILD_AND_RUN_MODULE(
    new AstSeq(
      new AstOperator('/',
        new AstNumber(24),
        new AstNumber(4),
        new AstNumber(3))),
    24/4/3, spec);
}

TEST(IrBuilderAstTest, MAKE_TEST_NAME(
    an_empty_seq,
    buildAndRunModule,
    throws)) {
  auto_ptr<AstSeq> astSeq(new AstSeq());
  TestingIrBuilderAst UUT;
  EXPECT_ANY_THROW(UUT.buildAndRunModule(*astSeq)) << amendAst(astSeq);
}

TEST(IrBuilderAstTest, MAKE_TEST_NAME(
    a_seq_with_some_expressions_not_having_a_value_but_the_last_having_a_value,
    buildAndRunModule,
    returns_the_result_of_the_last_expression)) {
  string spec = "Sequence containing a function declaration";
  TEST_BUILD_AND_RUN_MODULE(
    new AstSeq(
      new AstFunDecl("foo"),
      new AstNumber(42)),
    42, spec);
}

TEST(IrBuilderAstTest, MAKE_TEST_NAME(
    a_function_declaration,
    buildModule,
    adds_the_function_declaration_to_the_module_with_the_correct_signature)) {

  string spec = "Example: zero arguments";
  {
    // setup
    // IrBuilder is currently dumb and expects an expression having a value at
    // the end of a seq, thus provide one altought not needed for this test
    auto_ptr<AstSeq> astSeq(new AstSeq(new AstFunDecl("foo"), new AstNumber(42)));
    TestingIrBuilderAst UUT;

    // execute
    UUT.buildModule(*astSeq);

    // verify
    Function* functionIr = UUT.m_module->getFunction("foo");
    EXPECT_TRUE(functionIr!=NULL)
      << amendAst(astSeq) << amendSpec(spec);
    EXPECT_EQ(Type::getInt32Ty(getGlobalContext()), functionIr->getReturnType())
      << amendAst(astSeq) << amendSpec(spec);
    EXPECT_EQ(functionIr->arg_size(), 0)
      << amendAst(astSeq) << amendSpec(spec);
  }

  spec = "Example: two arguments";
  {
    // setup
    // IrBuilder is currently dumb and expects an expression having a value at
    // the end of a seq, thus provide one altought not needed for this test
    auto_ptr<AstSeq> astSeq(
      new AstSeq(
        new AstFunDecl(
          "foo",
          new AstArgDecl("arg1", new ObjTypeFunda(ObjTypeFunda::eInt)),
          new AstArgDecl("arg2", new ObjTypeFunda(ObjTypeFunda::eInt))),
        new AstNumber(42)));
    TestingIrBuilderAst UUT;

    // execute
    UUT.buildModule(*astSeq);

    // verify
    Function* functionIr = UUT.m_module->getFunction("foo");
    EXPECT_TRUE(functionIr!=NULL)
      << amendAst(astSeq) << amendSpec(spec);
    EXPECT_EQ(Type::getInt32Ty(getGlobalContext()), functionIr->getReturnType())
      << amendAst(astSeq) << amendSpec(spec);
    EXPECT_EQ(functionIr->arg_size(), 2)
      << amendAst(astSeq) << amendSpec(spec);
  }
}

TEST(IrBuilderAstTest, MAKE_TEST_NAME(
    a_function_definition,
    buildModule,
    adds_the_definition_to_the_module_with_the_correct_signature)) {

  string spec = "Example: zero arguments";
  {
    // setup
    // IrBuilder is currently dumb and expects an expression having a value at
    // the end of a seq, thus provide one altought not needed for this test
    auto_ptr<AstSeq> astSeq(new AstSeq(
        new AstFunDef(new AstFunDecl("foo"), new AstNumber(77)),
        new AstNumber(42)));
    TestingIrBuilderAst UUT;

    // execute
    UUT.buildModule(*astSeq);

    // verify
    Function* functionIr = UUT.m_module->getFunction("foo");
    EXPECT_TRUE(functionIr!=NULL)
      << amendAst(astSeq) << amendSpec(spec);
    EXPECT_EQ(Type::getInt32Ty(getGlobalContext()), functionIr->getReturnType())
      << amendAst(astSeq) << amendSpec(spec);
    EXPECT_EQ(functionIr->arg_size(), 0)
      << amendAst(astSeq) << amendSpec(spec);
  }

  spec = "Example: two arguments";
  {
    // setup
    // IrBuilder is currently dumb and expects an expression having a value at
    // the end of a seq, thus provide one altought not needed for this test
    list<string>* args = new list<string>();
    args->push_back("arg1");
    args->push_back("arg2");
    auto_ptr<AstSeq> astSeq( new AstSeq(
        new AstFunDef(
          new AstFunDecl(
            "foo",
            new AstArgDecl("arg1", new ObjTypeFunda(ObjTypeFunda::eInt)),
            new AstArgDecl("arg2", new ObjTypeFunda(ObjTypeFunda::eInt))),
          new AstNumber(77)),
        new AstNumber(42)));
    TestingIrBuilderAst UUT;

    // execute
    UUT.buildModule(*astSeq);

    // verify
    Function* functionIr = UUT.m_module->getFunction("foo");
    EXPECT_TRUE(functionIr!=NULL)
      << amendAst(astSeq) << amendSpec(spec);
    EXPECT_EQ(Type::getInt32Ty(getGlobalContext()), functionIr->getReturnType())
      << amendAst(astSeq) << amendSpec(spec);
    EXPECT_EQ(functionIr->arg_size(), args->size())
      << amendAst(astSeq) << amendSpec(spec);
  }
}

TEST(IrBuilderAstTest, MAKE_TEST_NAME(
    a_function_definition_foo_with_body_returning_a_value_x,
    buildModule,
    JIT_executing_foo_returns_x)) {

  string spec = "Example: zero arguments";
  {
    // setup
    // IrBuilder is currently dumb and expects an expression having a value at
    // the end of a seq, thus provide one altought not needed for this test
    auto_ptr<AstSeq> astSeq(new AstSeq(
        new AstFunDef(new AstFunDecl("foo"), new AstNumber(77)),
        new AstNumber(42)));
    TestingIrBuilderAst UUT;

    // execute
    UUT.buildModule(*astSeq);

    // verify
    EXPECT_EQ( 77, UUT.jitExecFunction("foo") )
      << amendAst(astSeq) << amendSpec(spec);
  }

  spec = "Example: one argument, which however is ignored";
  {
    // setup
    // IrBuilder is currently dumb and expects an expression having a value at
    // the end of a seq, thus provide one altought not needed for this test
    auto_ptr<AstSeq> astSeq(
      new AstSeq(
        new AstFunDef(
          new AstFunDecl(
            "foo",
            new AstArgDecl("arg1", new ObjTypeFunda(ObjTypeFunda::eInt))),
          new AstNumber(42)),
        new AstNumber(77)));
    TestingIrBuilderAst UUT;

    // execute
    UUT.buildModule(*astSeq);

    // verify
    EXPECT_EQ( 42, UUT.jitExecFunction1Arg("foo", 256) )
      << amendAst(astSeq) << amendSpec(spec);
  }
}

TEST(IrBuilderAstTest, MAKE_TEST_NAME(
    a_function_definition_foo_with_argument_x_with_body_returning_x,
    buildModule,
    JIT_executing_foo_returns_x)) {
  // setup
  // IrBuilder is currently dumb and expects an expression having a value at
  // the end of a seq, thus provide one altought not needed for this test
  auto_ptr<AstSeq> astSeq(
    new AstSeq(
      new AstFunDef(
        new AstFunDecl(
          "foo",
          new AstArgDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt))),
        new AstSymbol(new string("x"))),
      new AstNumber(77)));
  TestingIrBuilderAst UUT;

  // execute
  UUT.buildModule(*astSeq);

  // verify
  int x = 256;
  EXPECT_EQ( x, UUT.jitExecFunction1Arg("foo", x) )
    << amendAst(astSeq);
}

TEST(IrBuilderAstTest, MAKE_TEST_NAME(
    a_function_definition_foo_with_body_returning_a_simple_calculation_with_its_arguments,
    buildModule,
    JIT_executing_foo_returns_result_of_that_calculation)) {

  // Culcultation: x*y

  // setup
  // IrBuilder is currently dumb and expects an expression having a value at
  // the end of a seq, thus provide one altought not needed for this test
  auto_ptr<AstSeq> astSeq(
    new AstSeq(
      new AstFunDef(
        new AstFunDecl(
          "foo",
          new AstArgDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt)),
          new AstArgDecl("y", new ObjTypeFunda(ObjTypeFunda::eInt))),
        new AstOperator('*',
          new AstSymbol(new string("x")),
          new AstSymbol(new string("y")))),
      new AstNumber(77)));
  TestingIrBuilderAst UUT;

  // execute
  UUT.buildModule(*astSeq);

  // verify
  int x = 2;
  int y = 3;
  EXPECT_EQ( x*y, UUT.jitExecFunction2Arg("foo", x, y) ) << amendAst(astSeq);
}

TEST(IrBuilderAstTest, MAKE_TEST_NAME(
    a_function_definition_foo_with_argument_x_and_with_body_containing_a_simple_assignement_to_x_and_as_last_expr_in_bodies_seq_x,
    buildModule,
    JIT_executing_foo_returns_result_of_that_calculation)) {

  // Culcultation: x = x+1, return x

  // setup
  // IrBuilder is currently dumb and expects an expression having a value at
  // the end of a seq, thus provide one altought not needed for this test
  auto_ptr<AstSeq> astSeq(
    new AstSeq(
      new AstFunDef(
        new AstFunDecl(
          "foo",
          new AstArgDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt))),
        new AstSeq(
          new AstOperator('=',
            new AstSymbol(new string("x")),
            new AstOperator('+',
              new AstSymbol(new string("x")),
              new AstNumber(1))),
          new AstSymbol(new string("x")))),
      new AstNumber(77)));
  TestingIrBuilderAst UUT;

  // execute
  UUT.buildModule(*astSeq);

  // verify
  int x = 2;
  EXPECT_EQ( x+1, UUT.jitExecFunction1Arg("foo", x)) << amendAst(astSeq);
}

TEST(IrBuilderAstTest, MAKE_TEST_NAME(
    a_reference_to_an_inexistent_data_object,
    buildModule,
    throws)) {

  string spec = "Example: at global scope";
  {
    auto_ptr<AstSeq> astSeq(new AstSeq(new AstSymbol(new string("x"))));
    TestingIrBuilderAst UUT;
    EXPECT_ANY_THROW(UUT.buildAndRunModule(*astSeq))
      << amendAst(astSeq) << amendSpec(spec);
  }

  spec = "Example: within a function body";
  {
    auto_ptr<AstSeq> astSeq(
      new AstSeq(
        new AstFunDef(
          new AstFunDecl("foo"),
          new AstSymbol(new string("x"))),
        new AstNumber(42)));
    TestingIrBuilderAst UUT;
    EXPECT_ANY_THROW(UUT.buildAndRunModule(*astSeq))
      << amendAst(astSeq) << amendSpec(spec);
  }
}

TEST(IrBuilderAstTest, MAKE_TEST_NAME(
    multiple_identical_function_declarations,
    buildAndRunModule,
    succeeds)) {
  TEST_BUILD_AND_RUN_MODULE(
    new AstSeq(
      new AstFunDecl("foo"),
      new AstFunDecl("foo"),
      new AstNumber(42)),
    42, "");
}

TEST(IrBuilderAstTest, MAKE_TEST_NAME(
    a_function_declaration_and_a_matching_function_definition,
    buildAndRunModule,
    succeeds)) {
  TEST_BUILD_AND_RUN_MODULE(
    new AstSeq(
      new AstFunDecl("foo"),
      new AstFunDef(new AstFunDecl("foo"), new AstNumber(42)),
      new AstFunCall("foo")),
    42, "");
}

TEST(IrBuilderAstTest, MAKE_TEST_NAME(
    re_declaration_of_an_identifier_with_different_type,
    buildAndRunModule,
    throws)) {
  string spec = "First function type, then fundamental type";
  {
    TestingIrBuilderAst UUT;
    ParserExt parserExt(*UUT.m_env);
    auto_ptr<AstSeq> astSeq(
      new AstSeq(
        parserExt.createAstFunDecl("foo").first,
        new AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt)),
        new AstNumber(42)));
    EXPECT_ANY_THROW(UUT.buildAndRunModule(*astSeq)) <<
      amendSpec(spec) << amendAst(astSeq);
  }

  spec = "First fundamental type, then function type";
  {
    TestingIrBuilderAst UUT;
    ParserExt parserExt(*UUT.m_env);
    auto_ptr<AstSeq> astSeq(
      new AstSeq(
        new AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt)),
        parserExt.createAstFunDecl("foo").first,
        new AstNumber(42)));
    EXPECT_ANY_THROW(UUT.buildAndRunModule(*astSeq)) <<
      amendSpec(spec) << amendAst(astSeq);
  }
}

TEST(IrBuilderAstTest, MAKE_TEST_NAME(
    a_function_call_to_an_defined_function,
    buildAndRunModule,
    returns_result_of_that_function_call)) {

  string spec = "Trivial function with zero arguments returning a constant";
  TEST_BUILD_AND_RUN_MODULE(
    new AstSeq(
      new AstFunDef(
        new AstFunDecl("foo"),
        new AstNumber(42)),
      new AstFunCall("foo")),
    42, spec);

  spec = "Simple function with one argument which is ignored and a constant is returned";
  TEST_BUILD_AND_RUN_MODULE(
    new AstSeq(
      new AstFunDef(
        new AstFunDecl("foo", new AstArgDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt))),
        new AstNumber(42)),
      new AstFunCall("foo", new AstCtList(new AstNumber(0)))),
    42, spec);

  spec = "Simple function with two arguments whichs sum is returned";
  TEST_BUILD_AND_RUN_MODULE(
    new AstSeq(
      new AstFunDef(
        new AstFunDecl(
          "add",
          new AstArgDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt)),
          new AstArgDecl("y", new ObjTypeFunda(ObjTypeFunda::eInt))),
        new AstOperator('+',
          new AstSymbol(new string("x")),
          new AstSymbol(new string("y")))),
      new AstFunCall("add", new AstCtList(new AstNumber(1), new AstNumber(2)))),
    1+2, spec);
}

TEST(IrBuilderAstTest, MAKE_TEST_NAME(
    a_function_call_to_an_undefined_function,
    buildAndRunModule,
    throws)) {
  auto_ptr<AstSeq> astSeq(new AstSeq(new AstFunCall("foo")));
  TestingIrBuilderAst UUT;
  EXPECT_ANY_THROW(UUT.buildAndRunModule(*astSeq)) << amendAst(astSeq);
}

TEST(IrBuilderAstTest, MAKE_TEST_NAME(
    a_function_call_to_an_defined_function_WITH_incorrect_number_of_arguments,
    buildAndRunModule,
    throws)) {

  string spec = "Function foo expects one arg, but zero where passed on call";
  {
    auto_ptr<AstSeq> astSeq( new AstSeq(
        new AstFunDef(
          new AstFunDecl("foo", new AstArgDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt))),
          new AstNumber(42)),
        new AstFunCall("foo")));
    TestingIrBuilderAst UUT;
    EXPECT_ANY_THROW(UUT.buildAndRunModule(*astSeq)) << amendSpec(spec) << amendAst(astSeq);
  }

  spec = "Function foo expects no args, but one arg was passed on call";
  {
    auto_ptr<AstSeq> astSeq( new AstSeq(
        new AstFunDef(
          new AstFunDecl("foo"),
          new AstNumber(42)),
        new AstFunCall("foo", new AstCtList(new AstNumber(0)))));
    TestingIrBuilderAst UUT;
    EXPECT_ANY_THROW(UUT.buildAndRunModule(*astSeq)) << amendSpec(spec) << amendAst(astSeq);
  }
}

TEST(IrBuilderAstTest, MAKE_TEST_NAME(
    a_inmutable_data_object_definition_of_foo_being_initialized_with_x,
    buildAndRunModule,
    returns_x_rvalue)) {

  string spec = "Value of definition expression should equal initializer's value";
  TEST_BUILD_AND_RUN_MODULE(
    new AstSeq(
      new AstDataDef(
        new AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt)),
        new AstNumber(42))),
    42, spec);

  spec = "Value of definition expression is an rvalue and thus _not_ "
    "assignable to";
  {
    auto_ptr<AstSeq> astSeq(
      new AstSeq(
        new AstOperator('=',
          new AstDataDef(
            new AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt)),
            new AstNumber(42)),
          new AstNumber(77)),
        new AstSymbol(new string("foo"))));
    TestingIrBuilderAst UUT;
    EXPECT_ANY_THROW(UUT.buildAndRunModule(*astSeq)) << amendAst(astSeq);
  }
}

TEST(IrBuilderAstTest, MAKE_TEST_NAME(
    a_mutable_data_object_definition_of_foo_being_initialized_with_x,
    buildAndRunModule,
    returns_x_lvalue)) {

  string spec = "Value of definition expression should equal initializer's value";
  TEST_BUILD_AND_RUN_MODULE(
    new AstSeq(
      new AstDataDef(
        new AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable)),
        new AstNumber(42))),
    42, spec);

  spec = "Definition expresssion is an lvalue which concequently is assignable to";
  TEST_BUILD_AND_RUN_MODULE(
    new AstSeq(
      new AstOperator('=',
        new AstDataDef(
          new AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable)),
          new AstNumber(42)),
        new AstNumber(77)),
      new AstSymbol(new string("foo"))),
      77, spec);
}

TEST(IrBuilderAstTest, MAKE_TEST_NAME(
    foo_defined_as_data_object_followed_by_a_simple_expression_referencing_foo,
    buildAndRunModule,
    returns_result_of_that_expression)) {

  string spec = "value (aka immutable (aka const) data obj)";
  TEST_BUILD_AND_RUN_MODULE(
    new AstSeq(
      new AstDataDef(
        new AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt)),
        new AstNumber(42)),
      new AstOperator('+',
        new AstSymbol(new string("foo")),
        new AstNumber(77))),
    42+77, spec);

  spec = "variable (aka mutable data obj)";
  TEST_BUILD_AND_RUN_MODULE(
    new AstSeq(
      new AstDataDef(
        new AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable)),
        new AstNumber(42)),
      new AstOperator('+',
        new AstSymbol(new string("foo")),
        new AstNumber(77))),
    42+77, spec);
}

TEST(IrBuilderAstTest, MAKE_TEST_NAME(
    foo_defined_as_variable_initialized_with_x_followed_by_assigning_y_to_foo,
    buildAndRunModule,
    returns_y)) {

  string spec = "Example: Assignement 'foo=77' being last expression in sequence"; 
  TEST_BUILD_AND_RUN_MODULE(
    new AstSeq(
      new AstDataDef(
        new AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable)),
        new AstNumber(42)),
      new AstOperator('=',
        new AstSymbol(new string("foo")),
        new AstNumber(77))),
    77, spec);

  spec = "Example: After assignment 'foo=77', there is a further 'foo' expression"; 
  TEST_BUILD_AND_RUN_MODULE(
    new AstSeq(
      new AstDataDef(
        new AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable)),
        new AstNumber(42)),
      new AstOperator('=',
        new AstSymbol(new string("foo")),
        new AstNumber(77)),
      new AstSymbol(new string("foo"))),
    77, spec);
}

TEST(IrBuilderAstTest, MAKE_TEST_NAME4(
    foo_defined_as_value_followed_by_assigning_to_foo,
    buildAndRunModule,
    throws,
    BECAUSE_values_are_immutable_data_objects)) {
  auto_ptr<AstSeq> astSeq(
    new AstSeq(
      new AstDataDef(
        new AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt)),
        new AstNumber(42)),
      new AstOperator('=',
        new AstSymbol(new string("foo")),
        new AstNumber(77))));
  TestingIrBuilderAst UUT;
  EXPECT_ANY_THROW(UUT.buildAndRunModule(*astSeq)) << amendAst(astSeq);
}

TEST(IrBuilderAstTest, MAKE_TEST_NAME(
    a_value_definition_of_foo_with_no_explicit_initializer_followed_by_a_reference_to_foo,
    buildAndRunModule,
    returns_the_default)) {
  TEST_BUILD_AND_RUN_MODULE(
    new AstSeq(
      new AstDataDef(
        new AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable))),
      new AstSymbol(new string("foo"))),
    0, "");
}

TEST(IrBuilderAstTest, MAKE_TEST_NAME(
    multiple_identical_data_object_declaration_of_x,
    buildAndRunModule,
    succeeds)) {
  TEST_BUILD_AND_RUN_MODULE(
    new AstSeq(
      new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt)),
      new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt)),
      new AstNumber(42)),
    42, "");
}

TEST(IrBuilderAstTest, MAKE_TEST_NAME(
    multiple_data_obj_declaration_of_x_WITH_nonidentical_type,
    buildAndRunModule,
    throws)) {

  string spec = "Example: one uses 'val' (aka 'var const'), the other 'var' "
    "(aka 'val mutable'). The two also contribute to the type";
  {
    auto_ptr<AstSeq> astSeq(
      new AstSeq(
        new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eNoQualifier)),
        new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable)),
        new AstNumber(42)));
    TestingIrBuilderAst UUT;
    EXPECT_ANY_THROW(UUT.buildAndRunModule(*astSeq)) <<
      amendSpec(spec) << amendAst(astSeq);
  }
}

TEST(IrBuilderAstTest, MAKE_TEST_NAME2(
    GIVEN_a_pseudo_global_variable_named_x_AND_a_function_definition_also_defining_a_variable_x,
    THEN_the_inner_defintion_of_variable_x_shadows_the_outer_variable_x)) {

  // 'global' in quotes because there not really global variables
  // yet. Variables defined in 'global' scope really are local variables in
  // the implicit main method. 

  string spec = "function argument named 'x' shadows 'global' variable also named 'x'";
  TEST_BUILD_AND_RUN_MODULE(
    new AstSeq(
      new AstDataDef(new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt)), new AstNumber(42)),
      new AstFunDef(
        new AstFunDecl("foo", new AstArgDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt))),
        new AstOperator('=',
          new AstSymbol(new string("x")),
          new AstNumber(77))),
      new AstSymbol(new string("x"))),
    42, spec);

  spec = "variable 'x' local to a function shadows 'global' variable also named 'x'";
  TEST_BUILD_AND_RUN_MODULE(
    new AstSeq(
      new AstDataDef(new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt)), new AstNumber(42)),
      new AstFunDef(
        new AstFunDecl("foo"),
        new AstDataDef(
          new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt)),
          new AstNumber(77))),
      new AstSymbol(new string("x"))),
    42, spec);
}

TEST(IrBuilderAstTest, MAKE_TEST_NAME(
    double_definition_of_a_variable,
    buildAndRunModule,
    throws)) {

  string spec = "Example: two 'x' in 'global' scope";
  {
    auto_ptr<AstSeq> astSeq(
      new AstSeq(
        new AstDataDef(new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt))),
        new AstDataDef(new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt)))));
    TestingIrBuilderAst UUT;
    EXPECT_ANY_THROW(UUT.buildAndRunModule(*astSeq))
      << amendAst(astSeq) << amendSpec(spec);
  }

  spec = "Example: two 'x' in argument list of a function";
  {
    auto_ptr<AstSeq> astSeq(
      new AstSeq(
        new AstFunDef(
          new AstFunDecl(
            "foo",
            new AstArgDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt)),
            new AstArgDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt))),
          new AstNumber(42)),
        new AstNumber(42)));
    TestingIrBuilderAst UUT;
    EXPECT_ANY_THROW(UUT.buildAndRunModule(*astSeq))
      << amendAst(astSeq) << amendSpec(spec);
  }

  spec = "Example: 'x' as argument to a function and as local variable within";
  {
    auto_ptr<AstSeq> astSeq(
      new AstSeq(
        new AstFunDef(
          new AstFunDecl("foo", new AstArgDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt))),
          new AstDataDef(new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt)))),
        new AstNumber(42)));
    TestingIrBuilderAst UUT;
    EXPECT_ANY_THROW(UUT.buildAndRunModule(*astSeq))
      << amendAst(astSeq) << amendSpec(spec);
  }
}

TEST(IrBuilderAstTest, MAKE_TEST_NAME(
    an_if_else_expression_WITH_a_condition_evaluating_to_true,
    buildAndRunModule,
    returns_the_value_of_the_then_clause)) {
  TEST_BUILD_AND_RUN_MODULE(
    new AstSeq(
      new AstIf(
        new AstNumber(1), // condition
        new AstNumber(2), // then clause
        new AstNumber(3))), // else clause
    2, "");
}

TEST(IrBuilderAstTest, MAKE_TEST_NAME(
    an_if_else_expression_WITH_a_condition_evaluating_to_false,
    buildAndRunModule,
    returns_the_value_of_the_else_clause)) {
  TEST_BUILD_AND_RUN_MODULE(
    new AstSeq(
      new AstIf(
        new AstNumber(0), // condition
        new AstNumber(2), // then clause
        new AstNumber(3))), // else clause
    3, "");
}

TEST(IrBuilderAstTest, MAKE_TEST_NAME(
    an_if_expression_without_else_WITH_a_condition_evaluating_to_false,
    buildAndRunModule,
    returns_default_value_of_expressions_type)) {
  TEST_BUILD_AND_RUN_MODULE(
    new AstSeq(
      new AstIf(
        new AstNumber(0), // condition
        new AstNumber(2))), // then clause
    0, ""); // default for int
}
