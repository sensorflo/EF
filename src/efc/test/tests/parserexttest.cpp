#include "test.h"
#include "../parserext.h"
#include "../env.h"
#include "../errorhandler.h"
#include "../ast.h"
#include <memory>
using namespace testing;
using namespace std;

TEST(ParserExtTest, MAKE_TEST_NAME2(
    mkOperatorTree_WITH_a_CtList,
    returns_an_AST_tree_of_Operator_nodes_with_two_child_nodes_each)) {
  Env env;
  ErrorHandler errorHandler;
  ParserExt UUT(env, errorHandler);

  string spec = "Example with left associative operator and two childs in CtList";
  {
    AstCtList* ctList = new AstCtList(
      new AstNumber(42),
      new AstNumber(77));
    unique_ptr<AstValue> opTree{ UUT.mkOperatorTree("*", ctList) };
    EXPECT_EQ( "*(42 77)", opTree->toStr()) << amendSpec(spec) <<
      "CtList: " << ctList->toStr();
  }

  spec = "Example with left associative operator and more than two childs in CtList";
  {
    AstCtList* ctList = new AstCtList(
      new AstNumber(42),
      new AstNumber(77),
      new AstNumber(88));
    unique_ptr<AstValue> opTree{ UUT.mkOperatorTree("+", ctList) };
    EXPECT_EQ( "+(+(42 77) 88)", opTree->toStr()) << amendSpec(spec) <<
      "CtList: " << ctList->toStr();
  }

  spec = "Example with right associative operator and two childs in CtList";
  {
    AstCtList* ctList = new AstCtList(
      new AstNumber(42),
      new AstNumber(77));
    unique_ptr<AstValue> opTree{ UUT.mkOperatorTree("=", ctList) };
    EXPECT_EQ( "=(42 77)", opTree->toStr()) << amendSpec(spec) <<
      "CtList: " << ctList->toStr();
  }

  spec = "Example with right associative operator and more than two childs in CtList";
  {
    AstCtList* ctList = new AstCtList(
      new AstNumber(42),
      new AstNumber(77),
      new AstNumber(88));
    unique_ptr<AstValue> opTree{ UUT.mkOperatorTree("=", ctList) };
    EXPECT_EQ( "=(42 =(77 88))", opTree->toStr()) << amendSpec(spec) <<
      "CtList: " << ctList->toStr();
  }
}

