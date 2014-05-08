#include "test.h"
#include "../irbuilderast.h"
#include <memory>
using namespace testing;
using namespace std;

void testbuilAndRunModule(AstSeq* astSeq, int expectedResult) {
  // setup
  ENV_ASSERT_TRUE( astSeq!=NULL );
  auto_ptr<AstSeq> astSeqAp(astSeq);
  IrBuilderAst UUT;

  // execute & verify
  EXPECT_EQ(expectedResult, UUT.buildAndRunModule(*astSeq));
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
    a_seq_containing_function_definitions,
    buildAndRunModule,
    returns______________)) {
  testbuilAndRunModule(
    new AstSeq(
      new AstFunDef("foo", new AstSeq()),
      new AstNumber(42)),
    42);
}
