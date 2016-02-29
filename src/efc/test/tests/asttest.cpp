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
  EXPECT_EQ("int", TestingAstObjTypeSymbol::toName(ObjType::eInt));
}

TEST(AstObjTypeSymbolTest, MAKE_TEST_NAME1(toType)) {
  EXPECT_EQ(ObjType::eInt, TestingAstObjTypeSymbol::toType("int"));
}

TEST(AstObjTypeSymbolTest, MAKE_TEST_NAME1(
    createDefaultAstObjectForSemanticAnalizer)) {

  // examples: fundamental types
  {
    vector<ObjType::EType> inputTypes = {
      ObjType::eBool, ObjType::eChar, ObjType::eInt,
      ObjType::eDouble};

    for (const auto type : inputTypes ) {
      // setup
      const auto UUT = make_unique<AstObjTypeSymbol>(type);

      // exercise
      const auto defaultValue = unique_ptr<AstNumber>(
        dynamic_cast<AstNumber*>(
          UUT->createDefaultAstObjectForSemanticAnalizer()));

      // verify
      EXPECT_TRUE( defaultValue != nullptr );
      EXPECT_EQ( 0.0, defaultValue->value());
      EXPECT_TRUE( defaultValue->declaredAstObjType().objType().
        matchesFully(ObjTypeFunda(type)));
    }
  }

  // example: objType of created AstNumber is always immutable
  {
    // setup
    const auto UUT = make_unique<AstObjTypeQuali>(ObjType::eMutable,
      new AstObjTypeSymbol(ObjType::eInt));

    // exercise
    const auto defaultValue = unique_ptr<AstNumber>(
      dynamic_cast<AstNumber*>(
        UUT->createDefaultAstObjectForSemanticAnalizer()));

    // verify
    EXPECT_TRUE( defaultValue != nullptr );
    EXPECT_EQ( 0.0, defaultValue->value());
    EXPECT_TRUE( defaultValue->declaredAstObjType().objType().
      matchesFully(ObjTypeFunda(ObjType::eInt)));
  }

  // example: pointer
  {
    // setup
    const auto UUT = make_unique<AstObjTypePtr>(
      new AstObjTypeSymbol(ObjType::eInt));

    // exercise
    const auto defaultValue = unique_ptr<AstNumber>(
      dynamic_cast<AstNumber*>(
        UUT->createDefaultAstObjectForSemanticAnalizer()));

    // verify
    EXPECT_TRUE( defaultValue != nullptr );
    EXPECT_EQ( 0.0, defaultValue->value());
    EXPECT_TRUE( defaultValue->declaredAstObjType().objType().
      matchesFully(ObjTypeFunda(ObjType::eNullptr)));
  }
}
