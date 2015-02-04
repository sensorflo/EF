#include "test.h"
#include "../ast.h"
using namespace std;
using namespace testing;

TEST(AstTest, MAKE_TEST_NAME2(
    toStr,
    returns_the_canonical_string_representation_of_the_AST)) {

  string spec = "AstNumber";
  EXPECT_EQ( "42", AstNumber(42).toStr() ) << amendSpec(spec);

  spec = "AstCast";
  EXPECT_EQ( "cast(bool 0)",
    AstCast(new AstNumber(0), ObjTypeFunda::eBool).toStr()) << amendSpec(spec);

  spec = "AstSymbol";
  EXPECT_EQ( "foo", AstSymbol(new string("foo")).toStr()) << amendSpec(spec);

  spec = "AstSeq";
  EXPECT_EQ( "seq()", AstSeq().toStr() ) << amendSpec(spec);
  EXPECT_EQ( "seq(42)", AstSeq(new AstNumber(42)).toStr() ) << amendSpec(spec);
  EXPECT_EQ( "seq(42 77)", AstSeq(new AstNumber(42), new AstNumber(77)).toStr() ) << amendSpec(spec);

  spec = "AstOperator";
  EXPECT_EQ( "+(42 77)"  , AstOperator('+'       , new AstNumber(42), new AstNumber(77)).toStr() ) << amendSpec(spec);
  EXPECT_EQ( "not(42)"   , AstOperator('!'       , new AstNumber(42)).toStr() ) << amendSpec(spec);
  EXPECT_EQ( "not(42)"   , AstOperator("not"     , new AstNumber(42)).toStr() ) << amendSpec(spec);
  EXPECT_EQ( "and(42 77)", AstOperator("&&"      , new AstNumber(42), new AstNumber(77)).toStr() ) << amendSpec(spec);
  EXPECT_EQ( "and(42 77)", AstOperator("and"     , new AstNumber(42), new AstNumber(77)).toStr() ) << amendSpec(spec);
  EXPECT_EQ( "or(42 77)" , AstOperator("||"      , new AstNumber(42), new AstNumber(77)).toStr() ) << amendSpec(spec);
  EXPECT_EQ( "or(42 77)" , AstOperator("or"      , new AstNumber(42), new AstNumber(77)).toStr() ) << amendSpec(spec);

  spec = "AstIf";
  EXPECT_EQ( "if(x seq(1))", AstIf(
      new AstSymbol(new string("x")),
      new AstSeq(new AstNumber(1))).toStr() ) << amendSpec(spec);
  EXPECT_EQ( "if(x seq(1) seq(0))", AstIf(
      new AstSymbol(new string("x")),
      new AstSeq(new AstNumber(1)),
      new AstSeq(new AstNumber(0))).toStr() ) << amendSpec(spec);

  spec = "AstDataDecl";
  EXPECT_EQ( "decldata(foo int)", AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt)).toStr() ) << amendSpec(spec);
  EXPECT_EQ( "decldata(foo int-mut)", AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable)).toStr() ) << amendSpec(spec);
  
  spec = "AstDataDef";
  EXPECT_EQ( "data(decldata(foo int) ())", AstDataDef(new AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt))).toStr() ) << amendSpec(spec);
  EXPECT_EQ( "data(decldata(foo int-mut) (42))", AstDataDef(
      new AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable)),
      new AstNumber(42)).toStr() ) << amendSpec(spec);
  
  spec = "AstCtList";
  EXPECT_EQ( "", AstCtList().toStr() ) << amendSpec(spec);
  EXPECT_EQ( "42", AstCtList(new AstNumber(42)).toStr() ) << amendSpec(spec);
  EXPECT_EQ( "42 77", AstCtList(new AstNumber(42),new AstNumber(77)).toStr() ) << amendSpec(spec);

  spec = "AstFunDecl";
  EXPECT_EQ( "declfun(foo () int)", AstFunDecl("foo").toStr()) << amendSpec(spec);
  EXPECT_EQ( "declfun(foo () int)", AstFunDecl("foo", new list<AstArgDecl*>()).toStr()) << amendSpec(spec);
  EXPECT_EQ( "declfun(foo ((arg1 int-mut)) int)", AstFunDecl("foo", new AstArgDecl("arg1", new ObjTypeFunda(ObjTypeFunda::eInt))).toStr()) << amendSpec(spec);
  EXPECT_EQ( "declfun(foo ((arg1 int-mut) (arg2 int-mut)) int)", AstFunDecl("foo", new AstArgDecl("arg1", new ObjTypeFunda(ObjTypeFunda::eInt)), new AstArgDecl("arg2", new ObjTypeFunda(ObjTypeFunda::eInt))).toStr()) << amendSpec(spec);

  spec = "AstFunDef";
  EXPECT_EQ( "fun(declfun(foo () int) seq())", AstFunDef(new AstFunDecl("foo"),new AstSeq()).toStr() ) << amendSpec(spec);

  spec = "AstFunCall";
  EXPECT_EQ( "foo()", AstFunCall(new AstSymbol(new string("foo")), new AstCtList()).toStr() ) << amendSpec(spec);
  EXPECT_EQ( "foo(42 77)", AstFunCall(new AstSymbol(new string("foo")),
      new AstCtList(new AstNumber(42), new AstNumber(77))).toStr() ) << amendSpec(spec);
}
