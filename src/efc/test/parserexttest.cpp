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

