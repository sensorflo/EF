#include "test.h"
#include "../parserext.h"
#include "../env.h"
#include "../ast.h"
#include "../objtype.h"
using namespace testing;
using namespace std;

TEST(ParserExtTest, MAKE_TEST_NAME(
    an_already_declared_function_named_foo,
    createAstFunDecl_WITH_name_foo_AND_different_signature,
    throws) ) {
  // setup
  Env env;
  ParserExt UUT(env);
  UUT.createAstFunDecl("foo");

  // exercise & verify
  EXPECT_ANY_THROW(
    UUT.createAstFunDecl("foo",
      new AstArgDecl("arg1", new ObjTypeFunda(ObjTypeFunda::eInt))));
}

TEST(ParserExtTest, MAKE_TEST_NAME4(
    an_already_definded_function_named_foo,
    createAstFunDef_WITH_same_name_AND_same_signature,
    throws,
    BECAUSE_an_object_can_only_be_defined_once)) {
  // setup
  Env env;
  ParserExt UUT(env);
  pair<AstFunDecl*,SymbolTableEntry*> pair1 = UUT.createAstFunDecl("foo");
  UUT.createAstFunDef(pair1.first, new AstSeq(new AstNumber(77)), *pair1.second);
  pair<AstFunDecl*,SymbolTableEntry*> pair2 = UUT.createAstFunDecl("foo");

  // exercise & verify
  EXPECT_ANY_THROW(
    UUT.createAstFunDef(pair2.first, new AstSeq(new AstNumber(77)), *pair2.second));
}