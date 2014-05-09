#include "test.h"
#include "../irbuilderast.h"
#include "llvm/IR/Module.h"
#include <memory>
using namespace testing;
using namespace std;
using namespace llvm;

class TestingIrBuilderAst : public IrBuilderAst {
public:
  using IrBuilderAst::m_module;
};

void testbuilAndRunModule(AstSeq* astSeq, int expectedResult,
  const string& spec = "") {
  // setup
  ENV_ASSERT_TRUE( astSeq!=NULL );
  auto_ptr<AstSeq> astSeqAp(astSeq);
  IrBuilderAst UUT;

  // execute & verify
  EXPECT_EQ(expectedResult, UUT.buildAndRunModule(*astSeq)) << spec;
}

TEST(IrBuilderAstTest, MAKE_TEST_NAME(
    a_seq_with_one_or_more_numbers,
    buildAndRunModule,
    returns_the_last_number)) {
  testbuilAndRunModule(new AstSeq(new AstNumber(42)), 42);
  testbuilAndRunModule(new AstSeq(new AstNumber(11), new AstNumber(22)), 22);
}

TEST(IrBuilderAstTest, MAKE_TEST_NAME(
    a_seq_with_one_or_more_expressions_built_with_literal_numbers,
    buildAndRunModule,
    returns_the_result_of_the_last_expression)) {
  testbuilAndRunModule(
    new AstSeq(
      new AstOperator('+', new AstNumber(2), new AstNumber(3))),
    2+3);
  testbuilAndRunModule(
    new AstSeq(
      new AstOperator('+', new AstNumber(2), new AstNumber(3)),
      new AstOperator('*', new AstNumber(4), new AstNumber(5))),
    /*2+3,*/ 4*5);
}

TEST(IrBuilderAstTest, MAKE_TEST_NAME(
    a_seq_with_some_expressions_not_having_a_value_but_the_last_having_a_value,
    buildAndRunModule,
    returns_the_result_of_the_last_expression)) {
  string spec = "Sequence containing a function definition";
  testbuilAndRunModule(
    new AstSeq(
      new AstFunDef("foo", new AstSeq()),
      new AstNumber(42)),
    42, spec);
}

TEST(IrBuilderAstTest, MAKE_TEST_NAME(
    function_declaration,
    buildModule,
    adds_the_declaration_to_the_module_with_correct_signature)) {
  // setup
  // IrBuilder is currently dumb and expects an expression having a value at
  // the end of a seq, thus provide one altought not needed for this test
  auto_ptr<AstSeq> astSeq(new AstSeq(new AstFunDecl("foo"), new AstNumber(42)));
  TestingIrBuilderAst UUT;

  // execute
  UUT.buildModule(*astSeq);

  // verify
  Function* functionIr = UUT.m_module->getFunction("foo");
  EXPECT_TRUE(functionIr!=NULL);
  EXPECT_EQ(Type::getInt32Ty(getGlobalContext()), functionIr->getReturnType());
  EXPECT_EQ(functionIr->arg_size(), 0);
}
