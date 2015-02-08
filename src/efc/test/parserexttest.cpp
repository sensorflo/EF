#include "test.h"
#include "../parserext.h"
#include "../env.h"
#include "../ast.h"
#include "../errorhandler.h"
#include "../objtype.h"
using namespace testing;
using namespace std;

TEST(ParserExtTest, MAKE_TEST_NAME(
    an_already_declared_function_named_foo,
    mkFunDecl_WITH_name_foo_AND_different_signature,
    throws) ) {
  // setup
  Env env;
  ErrorHandler errorHandler;
  ParserExt UUT(env, errorHandler);
  UUT.mkFunDecl("foo");

  // exercise & verify
  EXPECT_ANY_THROW(
    UUT.mkFunDecl("foo",
      new AstArgDecl("arg1", new ObjTypeFunda(ObjTypeFunda::eInt))));
}

TEST(ParserExtTest, MAKE_TEST_NAME4(
    an_already_definded_function_named_foo,
    mkFunDef_WITH_same_name_AND_same_signature,
    throws,
    BECAUSE_an_object_can_only_be_defined_once)) {
  // setup
  Env env;
  ErrorHandler errorHandler;
  ParserExt UUT(env, errorHandler);
  pair<AstFunDecl*,SymbolTableEntry*> pair1 = UUT.mkFunDecl("foo");
  UUT.mkFunDef(pair1, new AstNumber(77));
  pair<AstFunDecl*,SymbolTableEntry*> pair2 = UUT.mkFunDecl("foo");

  // exercise & verify
  EXPECT_ANY_THROW(
    UUT.mkFunDef(pair2, new AstNumber(77)));
}
