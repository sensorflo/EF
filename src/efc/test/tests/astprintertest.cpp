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
  EXPECT_TOSTR_EQ( ":42", AstBlock(new AstNumber(42)), spec);

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
  EXPECT_TOSTR_EQ( "=(x 77)"   , AstOperator('='                , new AstSymbol("x"), new AstNumber(77)), spec);
  EXPECT_TOSTR_EQ( ".=(x 77)"  , AstOperator(".="               , new AstSymbol("x"), new AstNumber(77)), spec);
  EXPECT_TOSTR_EQ( "+(42 77)"  , AstOperator('+'                , new AstNumber(42), new AstNumber(77)), spec);
  EXPECT_TOSTR_EQ( "!(42)"     , AstOperator('!'                , new AstNumber(42)), spec);
  EXPECT_TOSTR_EQ( "!(42)"     , AstOperator("not"              , new AstNumber(42)), spec);
  EXPECT_TOSTR_EQ( "and(42 77)", AstOperator("&&"               , new AstNumber(42), new AstNumber(77)), spec);
  EXPECT_TOSTR_EQ( "and(42 77)", AstOperator("and"              , new AstNumber(42), new AstNumber(77)), spec);
  EXPECT_TOSTR_EQ( "or(42 77)" , AstOperator("||"               , new AstNumber(42), new AstNumber(77)), spec);
  EXPECT_TOSTR_EQ( "or(42 77)" , AstOperator("or"               , new AstNumber(42), new AstNumber(77)), spec);
  EXPECT_TOSTR_EQ( "*(x)"      , AstOperator(AstOperator::eDeref, new AstSymbol("x")), spec);;
  EXPECT_TOSTR_EQ( "&(x)"      , AstOperator('&'                , new AstSymbol("x")), spec);;

  spec = "AstSeq";
  EXPECT_TOSTR_EQ( ";42"         , AstSeq(new AstNumber(42)),
    spec + " - As a special case, when there's only one operand, ommit the parentheses");
  EXPECT_TOSTR_EQ( ";(42 77)"    , AstSeq(new AstNumber(42), new AstNumber(77)), spec);;
  EXPECT_TOSTR_EQ( ";(42 77 88)" , AstSeq(new AstNumber(42), new AstNumber(77), new AstNumber(88)), spec);;

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

  spec = "AstDataDef";
  EXPECT_TOSTR_EQ( "data(foo int ())", AstDataDef("foo",
      ObjTypeFunda::eInt), spec);
  EXPECT_TOSTR_EQ( "data(foo mut-int (42))", AstDataDef("foo",
      make_shared<ObjTypeQuali>(
        ObjType::eMutable,
        make_shared<ObjTypeFunda>(ObjTypeFunda::eInt)),
      new AstNumber(42)), spec);
  EXPECT_TOSTR_EQ( "data(foo int noinit)", AstDataDef("foo",
      ObjTypeFunda::eInt, AstDataDef::noInit), spec);
  
  spec = "AstCtList";
  EXPECT_TOSTR_EQ( "", AstCtList(), spec);
  EXPECT_TOSTR_EQ( "42", AstCtList(new AstNumber(42)), spec);
  EXPECT_TOSTR_EQ( "42 77", AstCtList(new AstNumber(42),new AstNumber(77)), spec);

  spec = "AstFunDef";
  EXPECT_TOSTR_EQ( "fun(foo () int 42)",
    AstFunDef("foo",
      AstFunDef::createArgs(),
      make_shared<ObjTypeFunda>(ObjTypeFunda::eInt),
      new AstNumber(42)), spec);
  EXPECT_TOSTR_EQ( "fun(foo () bool 1bool)",
    AstFunDef("foo",
      AstFunDef::createArgs(),
      make_shared<ObjTypeFunda>(ObjTypeFunda::eBool),
      new AstNumber(true, ObjTypeFunda::eBool)), spec);
  EXPECT_TOSTR_EQ( "fun(foo ((arg1 int)) int 42)",
    AstFunDef("foo",
      AstFunDef::createArgs(
        new AstDataDef("arg1", ObjTypeFunda::eInt)),
      make_shared<ObjTypeFunda>(ObjTypeFunda::eInt),
      new AstNumber(42)), spec);
  EXPECT_TOSTR_EQ( "fun(foo ((arg1 int) (arg2 int)) int 42)",
    AstFunDef("foo",
      AstFunDef::createArgs(
        new AstDataDef("arg1", ObjTypeFunda::eInt),
        new AstDataDef("arg2", ObjTypeFunda::eInt)),
      make_shared<ObjTypeFunda>(ObjTypeFunda::eInt),
      new AstNumber(42)), spec);

  spec = "AstFunCall";
  EXPECT_TOSTR_EQ( "call(foo)", AstFunCall(new AstSymbol("foo"), new AstCtList()), spec);
  EXPECT_TOSTR_EQ( "call(foo 42 77)", AstFunCall(new AstSymbol("foo"),
      new AstCtList(new AstNumber(42), new AstNumber(77))), spec);

  spec = "AstObjTypeSymbol";
  EXPECT_TOSTR_EQ( "int", AstObjTypeSymbol(ObjTypeFunda::eInt), spec);
  EXPECT_TOSTR_EQ( "foo", AstObjTypeSymbol("foo"), spec);

  spec = "AstObjTypeQuali";
  EXPECT_TOSTR_EQ( "mut-int",
    AstObjTypeQuali(ObjType::eMutable, new AstObjTypeSymbol(ObjTypeFunda::eInt)), spec);
  EXPECT_TOSTR_EQ( "mut-foo",
    AstObjTypeQuali(ObjType::eMutable, new AstObjTypeSymbol("foo")), spec);

  spec = "AstObjTypePtr";
  EXPECT_TOSTR_EQ( "raw*int",
    AstObjTypePtr(new AstObjTypeSymbol(ObjTypeFunda::eInt)), spec);
  EXPECT_TOSTR_EQ( "raw*foo",
    AstObjTypePtr(new AstObjTypeSymbol("foo")), spec);

  spec = "AstClassDef";
  EXPECT_TOSTR_EQ( "class(foo)", AstClassDef("foo"), spec);
  EXPECT_TOSTR_EQ( "class(foo data(m1 int ()) data(m2 int ()))",
    AstClassDef("foo",
      new AstDataDef("m1", ObjTypeFunda::eInt),
      new AstDataDef("m2", ObjTypeFunda::eInt)), spec);
}
