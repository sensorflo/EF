#include "test.h"
#include "../ast.h"
using namespace std;
using namespace testing;

TEST(AstTest,test_toString) {
  // AstNumber
  EXPECT_EQ( "42", AstNumber(42).toStr() );

  // AstSymbol
  EXPECT_EQ( "foo", AstSymbol(new string("foo")).toStr() );

  // AstSeq
  EXPECT_EQ( "seq()", AstSeq().toStr() );
  EXPECT_EQ( "seq(42)", AstSeq(new AstNumber(42)).toStr() );
  EXPECT_EQ( "seq(42 77)", AstSeq(new AstNumber(42), new AstNumber(77)).toStr() );

  // AstOperator
  EXPECT_EQ( "+(42 77)",
    AstOperator('+',new AstNumber(42), new AstNumber(77)).toStr() );

  // AstIf
  EXPECT_EQ( "if(seq(x) seq(1))", AstIf(
      new AstSeq(new AstSymbol(new string("x"))),
      new AstSeq(new AstNumber(1))).toStr() );
  EXPECT_EQ( "if(seq(x) seq(1) seq(0))", AstIf(
      new AstSeq(new AstSymbol(new string("x"))),
      new AstSeq(new AstNumber(1)),
      new AstSeq(new AstNumber(0))).toStr() );

  // AstDataDecl
  EXPECT_EQ( "decldata(foo)", AstDataDecl("foo").toStr() );
  EXPECT_EQ( "decldata(foo mut)", AstDataDecl("foo",AstDataDecl::eAlloca).toStr() );
  
  // AstDataDef
  EXPECT_EQ( "data(decldata(foo))", AstDataDef(new AstDataDecl("foo")).toStr() );
  EXPECT_EQ( "data(decldata(foo mut) seq(42))", AstDataDef(
      new AstDataDecl("foo",AstDataDecl::eAlloca),
      new AstSeq(new AstNumber(42))).toStr() );
  
  // AstCtList
  EXPECT_EQ( "", AstCtList().toStr() );
  EXPECT_EQ( "42", AstCtList(new AstNumber(42)).toStr() );
  EXPECT_EQ( "42 77", AstCtList(new AstNumber(42),new AstNumber(77)).toStr() );

  // AstFunDecl
  EXPECT_EQ( "declfun(foo ())", AstFunDecl("foo").toStr() );
  EXPECT_EQ( "declfun(foo ())", AstFunDecl("foo", new list<string>()).toStr() );
  list<string>* args = new list<string>();
  args->push_back("arg1");
  EXPECT_EQ( "declfun(foo (arg1))", AstFunDecl("foo", args).toStr() );
  args = new list<string>();
  args->push_back("arg1");
  args->push_back("arg2");
  EXPECT_EQ( "declfun(foo (arg1 arg2))", AstFunDecl("foo", args).toStr() );

  // AstFunDef
  EXPECT_EQ( "fun(declfun(foo ()) seq())", AstFunDef(new AstFunDecl("foo"),new AstSeq()).toStr() );

  // AstFunCall
  EXPECT_EQ( "foo()", AstFunCall("foo", new AstCtList()).toStr() );
  EXPECT_EQ( "foo(42 77)", AstFunCall("foo",
      new AstCtList(new AstNumber(42), new AstNumber(77))).toStr() );
}
