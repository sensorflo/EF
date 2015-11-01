#include "test.h"
#include "../ast.h"
#include "../env.h"
#include "../parserext.h"
#include "../errorhandler.h"
#include "../object.h"
using namespace std;
using namespace testing;

TEST(AstTest, MAKE_TEST_NAME(a,b,c)) {
  // setup
  Env env;
  ErrorHandler errorHandler;
  ParserExt pe(env, errorHandler);
  const auto memberX = new AstDataDef("x", ObjTypeFunda::eInt);
  const auto memberY = new AstDataDef("y", ObjTypeFunda::eInt);
  unique_ptr<AstClassDef> UUT(pe.mkClassDef("Foo", memberX, memberY));
  shared_ptr<Entity> entityX, entityY;
  size_t indexX = 0, indexY = 0;

  // exercise
  UUT->find("x", &entityX, &indexX);
  UUT->find("y", &entityY, &indexY);

  // verify
  EXPECT_EQ(0, indexX) << amendAst(UUT.get()) << amend(env);
  EXPECT_EQ(memberX->object(), entityX.get()) << amendAst(UUT.get()) << amend(env);
  EXPECT_EQ(1, indexY) << amendAst(UUT.get()) << amend(env);
  EXPECT_EQ(memberY->object(), entityY.get()) << amendAst(UUT.get()) << amend(env);
}
