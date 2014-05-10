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
  EXPECT_EQ( "seq(42 77)", AstSeq(new AstNumber(42), new AstNumber(77)).toStr() );

  // AstOperator
  EXPECT_EQ( "+(42 77)",
    AstOperator('+',new AstNumber(42), new AstNumber(77)).toStr() );

  // AstCtList
  EXPECT_EQ( "", AstCtList().toStr() );
  EXPECT_EQ( "42", AstCtList(new AstNumber(42)).toStr() );
  EXPECT_EQ( "42 77", AstCtList(new AstNumber(42),new AstNumber(77)).toStr() );

  // AstFunDecl
  EXPECT_EQ( "declfun(foo)", AstFunDecl("foo").toStr() );

  // AstFunDef
  EXPECT_EQ( "fun(declfun(foo) seq())", AstFunDef(new AstFunDecl("foo"),new AstSeq()).toStr() );

  // AstFunCall
  EXPECT_EQ( "foo()", AstFunCall("foo", new AstCtList()).toStr() );
  EXPECT_EQ( "foo(42 77)", AstFunCall("foo",
      new AstCtList(new AstNumber(42), new AstNumber(77))).toStr() );
}
