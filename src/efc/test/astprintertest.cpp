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

  string spec = "AstNop";
  EXPECT_TOSTR_EQ( "nop", AstNop(), spec);

  spec = "AstBlock";
  EXPECT_TOSTR_EQ( "42", AstBlock(new AstNumber(42)), spec);

  spec = "AstNumber";
  EXPECT_TOSTR_EQ( "42", AstNumber(42), spec);
  EXPECT_TOSTR_EQ( "42.77d", AstNumber(42.77, ObjTypeFunda::eDouble), spec);
  EXPECT_TOSTR_EQ( "42d", AstNumber(42, ObjTypeFunda::eDouble), spec);
  EXPECT_TOSTR_EQ( "0bool", AstNumber(0, ObjTypeFunda::eBool), spec);
  EXPECT_TOSTR_EQ( "1bool", AstNumber(1, ObjTypeFunda::eBool), spec);
  EXPECT_TOSTR_EQ( "'x'", AstNumber('x', ObjTypeFunda::eChar), spec);

  spec = "AstCast";
  EXPECT_TOSTR_EQ( "bool(0)",
    AstCast(ObjTypeFunda::eBool, new AstNumber(0)), spec);
  EXPECT_TOSTR_EQ( "int(0bool)",
    AstCast(ObjTypeFunda::eInt, new AstNumber(0, ObjTypeFunda::eBool)), spec);

  spec = "AstSymbol";
  EXPECT_TOSTR_EQ( "foo", AstSymbol("foo"), spec);

  spec = "AstOperator";
  EXPECT_TOSTR_EQ( "=(x 77)"   , AstOperator('='       , new AstSymbol("x"), new AstNumber(77)), spec);
  EXPECT_TOSTR_EQ( ".=(x 77)"  , AstOperator(".="      , new AstSymbol("x"), new AstNumber(77)), spec);
  EXPECT_TOSTR_EQ( "+(42 77)"  , AstOperator('+'       , new AstNumber(42), new AstNumber(77)), spec);
  EXPECT_TOSTR_EQ( "!(42)"     , AstOperator('!'       , new AstNumber(42)), spec);
  EXPECT_TOSTR_EQ( "!(42)"     , AstOperator("not"     , new AstNumber(42)), spec);
  EXPECT_TOSTR_EQ( "and(42 77)", AstOperator("&&"      , new AstNumber(42), new AstNumber(77)), spec);
  EXPECT_TOSTR_EQ( "and(42 77)", AstOperator("and"     , new AstNumber(42), new AstNumber(77)), spec);
  EXPECT_TOSTR_EQ( "or(42 77)" , AstOperator("||"      , new AstNumber(42), new AstNumber(77)), spec);
  EXPECT_TOSTR_EQ( "or(42 77)" , AstOperator("or"      , new AstNumber(42), new AstNumber(77)), spec);
  EXPECT_TOSTR_EQ( ";(42 77)"  , AstOperator(';'       , new AstNumber(42), new AstNumber(77)), spec);;

  spec = "AstIf";
  EXPECT_TOSTR_EQ( "if(x 1)",
    AstIf(
      new AstSymbol("x"),
      new AstNumber(1)), spec);
  EXPECT_TOSTR_EQ( "if(x 1 0)",
    AstIf(
      new AstSymbol("x"),
      new AstNumber(1),
      new AstNumber(0)), spec);

  spec = "AstWhile";
  EXPECT_TOSTR_EQ( "while(x 1)",
    AstLoop(
      new AstSymbol("x"),
      new AstNumber(1)), spec);

  spec = "AstReturn";
  EXPECT_TOSTR_EQ( "return(42)",
    AstReturn(new AstNumber(42)), spec);

  spec = "AstDataDecl";
  EXPECT_TOSTR_EQ( "decldata(foo int)", AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt)), spec);
  EXPECT_TOSTR_EQ( "decldata(foo mut-int)", AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable)), spec);
  
  spec = "AstDataDef";
  EXPECT_TOSTR_EQ( "data(decldata(foo int) ())", AstDataDef(new AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt))), spec);
  EXPECT_TOSTR_EQ( "data(decldata(foo mut-int) (42))", AstDataDef(
      new AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable)),
      new AstNumber(42)), spec);
  
  spec = "AstCtList";
  EXPECT_TOSTR_EQ( "", AstCtList(), spec);
  EXPECT_TOSTR_EQ( "42", AstCtList(new AstNumber(42)), spec);
  EXPECT_TOSTR_EQ( "42 77", AstCtList(new AstNumber(42),new AstNumber(77)), spec);

  spec = "AstFunDecl";
  EXPECT_TOSTR_EQ( "declfun(foo () int)", AstFunDecl("foo"), spec);
  EXPECT_TOSTR_EQ( "declfun(foo () int)", AstFunDecl("foo", new list<AstArgDecl*>()), spec);
  EXPECT_TOSTR_EQ( "declfun(foo () bool)", AstFunDecl("foo", new list<AstArgDecl*>(), make_shared<ObjTypeFunda>(ObjTypeFunda::eBool)), spec);
  EXPECT_TOSTR_EQ( "declfun(foo ((arg1 mut-int)) int)", AstFunDecl("foo", new AstArgDecl("arg1", new ObjTypeFunda(ObjTypeFunda::eInt))), spec);
  EXPECT_TOSTR_EQ( "declfun(foo ((arg1 mut-int) (arg2 mut-int)) int)", AstFunDecl("foo", new AstArgDecl("arg1", new ObjTypeFunda(ObjTypeFunda::eInt)), new AstArgDecl("arg2", new ObjTypeFunda(ObjTypeFunda::eInt))), spec);

  spec = "AstFunDef";
  EXPECT_TOSTR_EQ( "fun(declfun(foo () int) 42)", AstFunDef(new AstFunDecl("foo"),new AstNumber(42)), spec);

  spec = "AstFunCall";
  EXPECT_TOSTR_EQ( "foo()", AstFunCall(new AstSymbol("foo"), new AstCtList()), spec);
  EXPECT_TOSTR_EQ( "foo(42 77)", AstFunCall(new AstSymbol("foo"),
      new AstCtList(new AstNumber(42), new AstNumber(77))), spec);
}
