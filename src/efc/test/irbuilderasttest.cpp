#include "test.h"
#include "../irbuilderast.h"
using namespace testing;

TEST(IrBuilderAstTest, MAKE_TEST_NAME(
    a_seq_with_one_number,
    buildAndRunModule,
    returns_that_number)) {
  // setup
  IrBuilderAst UUT;
  AstSeq* astSeq = new AstSeq(new AstNumber(42));
  ENV_ASSERT_TRUE( astSeq!=NULL );

  // execute & verify
  EXPECT_EQ(42, UUT.buildAndRunModule(*astSeq));

  // tear down
  delete astSeq;
}
