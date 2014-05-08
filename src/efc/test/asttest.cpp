#include "test.h"
#include "../ast.h"
using namespace std;
using namespace testing;

TEST(AstTest,test_toString) {
  // AstNumber
  EXPECT_EQ( "42", AstNumber(42).toStr() );

  // AstSeq
  EXPECT_EQ( "seq()", AstSeq().toStr() );
  EXPECT_EQ( "seq(42)", AstSeq(new AstNumber(42)).toStr() );
  EXPECT_EQ( "seq(42,77)", AstSeq(new AstNumber(42), new AstNumber(77)).toStr() );

  // AstOperator
  EXPECT_EQ( "+(42,77)",
    AstOperator('+',new AstNumber(42), new AstNumber(77)).toStr() );

  // AstFunDef
  EXPECT_EQ( "fun(foo,seq())", AstFunDef("foo",new AstSeq()).toStr() );
}
