#include "test.h"
#include "../ast.h"
using namespace std;
using namespace testing;

class TestingAstObjTypeSymbol : public AstObjTypeSymbol {
public:
  using AstObjTypeSymbol::toName;
  using AstObjTypeSymbol::toType;
};

TEST(AstObjTypeSymbolTest, MAKE_TEST_NAME1(toName)) {
  EXPECT_EQ("int", TestingAstObjTypeSymbol::toName(ObjTypeFunda::eInt));
}

TEST(AstObjTypeSymbolTest, MAKE_TEST_NAME1(toType)) {
  EXPECT_EQ(ObjTypeFunda::eInt, TestingAstObjTypeSymbol::toType("int"));
}
