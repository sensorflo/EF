#include "test.h"
#include "../irbuilderast.h"
#include <memory>
using namespace testing;
using namespace std;

TEST(IrBuilderAstTest, MAKE_TEST_NAME(
    a_seq_with_one_or_more_numbers,
    buildAndRunModule,
    returns_the_last_number)) {
  // example 1
  {
    // setup
    IrBuilderAst UUT;
    auto_ptr<AstSeq> astSeq(new AstSeq(new AstNumber(42)));
    ENV_ASSERT_TRUE( astSeq.get()!=NULL );

    // execute & verify
    EXPECT_EQ(42, UUT.buildAndRunModule(*astSeq.get()));
  }

  // example 2
  {
    // setup
    IrBuilderAst UUT;
    auto_ptr<AstSeq> astSeq(new AstSeq(new AstNumber(11), new AstNumber(22)));
    ENV_ASSERT_TRUE( astSeq.get()!=NULL );

    // execute & verify
    EXPECT_EQ(22, UUT.buildAndRunModule(*astSeq));
  }
}

TEST(IrBuilderAstTest, MAKE_TEST_NAME(
    a_seq_with_one_or_more_expressions_built_with_literal_numbers,
    buildAndRunModule,
    returns_the_result_of_the_last_expression)) {

  // example 1
  {
    // setup
    IrBuilderAst UUT;
    auto_ptr<AstSeq> astSeq(new AstSeq(new AstOperator('+', new AstNumber(2), new AstNumber(3))));
    ENV_ASSERT_TRUE( astSeq.get()!=NULL );

    // execute & verify
    EXPECT_EQ(2+3, UUT.buildAndRunModule(*astSeq.get()));
  }
}
