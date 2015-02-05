#include "test.h"
#include "../astprinter.h"
#include "../ast.h"
using namespace std;
using namespace testing;

#define EXPECT_TOSTR_EQ(expected_str, ast, spec) \
  EXPECT_EQ( expected_str, AstPrinter::toStr(ast)) << amendSpec(spec);

TEST(AstPrinterTest, MAKE_TEST_NAME2(
    toStr,
    returns_the_canonical_string_representation_of_the_AST)) {

  string spec = "AstNumber";
  EXPECT_TOSTR_EQ( "42", AstNumber(42), spec);
  EXPECT_TOSTR_EQ( "0bool", AstNumber(0, ObjTypeFunda::eBool), spec);
  EXPECT_TOSTR_EQ( "1bool", AstNumber(1, ObjTypeFunda::eBool), spec);

  spec = "AstCast";
  EXPECT_TOSTR_EQ( "cast(bool 0)",
    AstCast(new AstNumber(0), ObjTypeFunda::eBool), spec);

  spec = "AstSymbol";
  EXPECT_TOSTR_EQ( "foo", AstSymbol(new string("foo")), spec);

  spec = "AstOperator";
  EXPECT_TOSTR_EQ( "+(42 77)"  , AstOperator('+'       , new AstNumber(42), new AstNumber(77)), spec);
  EXPECT_TOSTR_EQ( "not(42)"   , AstOperator('!'       , new AstNumber(42)), spec);
  EXPECT_TOSTR_EQ( "not(42)"   , AstOperator("not"     , new AstNumber(42)), spec);
  EXPECT_TOSTR_EQ( "and(42 77)", AstOperator("&&"      , new AstNumber(42), new AstNumber(77)), spec);
  EXPECT_TOSTR_EQ( "and(42 77)", AstOperator("and"     , new AstNumber(42), new AstNumber(77)), spec);
  EXPECT_TOSTR_EQ( "or(42 77)" , AstOperator("||"      , new AstNumber(42), new AstNumber(77)), spec);
  EXPECT_TOSTR_EQ( "or(42 77)" , AstOperator("or"      , new AstNumber(42), new AstNumber(77)), spec);
  EXPECT_TOSTR_EQ( ";(42 77)"  , AstOperator(';'       , new AstNumber(42), new AstNumber(77)), spec);;

  spec = "AstIf";
  EXPECT_TOSTR_EQ( "if(x 1)",
    AstIf(
      new AstSymbol(new string("x")),
      new AstNumber(1)), spec);
  EXPECT_TOSTR_EQ( "if(x 1 0)",
    AstIf(
      new AstSymbol(new string("x")),
      new AstNumber(1),
      new AstNumber(0)), spec);

  spec = "AstDataDecl";
  EXPECT_TOSTR_EQ( "decldata(foo int)", AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt)), spec);
  EXPECT_TOSTR_EQ( "decldata(foo int-mut)", AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable)), spec);
  
  spec = "AstDataDef";
  EXPECT_TOSTR_EQ( "data(decldata(foo int) ())", AstDataDef(new AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt))), spec);
  EXPECT_TOSTR_EQ( "data(decldata(foo int-mut) (42))", AstDataDef(
      new AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable)),
      new AstNumber(42)), spec);
  
  spec = "AstCtList";
  EXPECT_TOSTR_EQ( "", AstCtList(), spec);
  EXPECT_TOSTR_EQ( "42", AstCtList(new AstNumber(42)), spec);
  EXPECT_TOSTR_EQ( "42 77", AstCtList(new AstNumber(42),new AstNumber(77)), spec);

  spec = "AstFunDecl";
  EXPECT_TOSTR_EQ( "declfun(foo () int)", AstFunDecl("foo"), spec);
  EXPECT_TOSTR_EQ( "declfun(foo () int)", AstFunDecl("foo", new list<AstArgDecl*>()), spec);
  EXPECT_TOSTR_EQ( "declfun(foo ((arg1 int-mut)) int)", AstFunDecl("foo", new AstArgDecl("arg1", new ObjTypeFunda(ObjTypeFunda::eInt))), spec);
  EXPECT_TOSTR_EQ( "declfun(foo ((arg1 int-mut) (arg2 int-mut)) int)", AstFunDecl("foo", new AstArgDecl("arg1", new ObjTypeFunda(ObjTypeFunda::eInt)), new AstArgDecl("arg2", new ObjTypeFunda(ObjTypeFunda::eInt))), spec);

  spec = "AstFunDef";
  EXPECT_TOSTR_EQ( "fun(declfun(foo () int) 42)", AstFunDef(new AstFunDecl("foo"),new AstNumber(42)), spec);

  spec = "AstFunCall";
  EXPECT_TOSTR_EQ( "foo()", AstFunCall(new AstSymbol(new string("foo")), new AstCtList()), spec);
  EXPECT_TOSTR_EQ( "foo(42 77)", AstFunCall(new AstSymbol(new string("foo")),
      new AstCtList(new AstNumber(42), new AstNumber(77))), spec);
}
