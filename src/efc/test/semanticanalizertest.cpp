#include "test.h"
#include "../semanticanalizer.h"
#include "../ast.h"
#include "../objtype.h"
#include <memory>
#include <string>
using namespace testing;
using namespace std;

TEST(SemanticAnalizerTest, MAKE_TEST_NAME(
    a_trivial_AST,
    transform,
    returns_the_same_AST)) {
  SemanticAnalizer UUT;
  AstNode* oldAst = new AstNumber(42);
  AstNode* newAst = UUT.transform(*oldAst);
  EXPECT_EQ(oldAst, newAst) <<
    "old AST: " << amendAst(oldAst) << "\n" <<
    "new AST: " << amendAst(newAst);
}
