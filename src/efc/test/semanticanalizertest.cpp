#include "test.h"
#include "../semanticanalizer.h"
#include "../ast.h"
#include "../objtype.h"
#include "../errorhandler.h"
#include <memory>
#include <string>
using namespace testing;
using namespace std;

void testTransformThrows(AstNode* ast, const string& spec) {
  ENV_ASSERT_TRUE( ast!=NULL );
  ErrorHandler errorHandler;
  SemanticAnalizer UUT(errorHandler);
  EXPECT_ANY_THROW(UUT.transform(*ast)) << amendSpec(spec) << amendAst(ast);
}

#define TEST_TRANSFORM_THROWS(ast, spec)                                \
  {                                                                     \
    SCOPED_TRACE("transform called from here (via TEST_TRANSFORM_THROWS)"); \
    testTransformThrows(ast, spec);                                     \
  }

TEST(SemanticAnalizerTest, MAKE_TEST_NAME(
    a_trivial_AST,
    transform,
    returns_the_same_AST)) {
  ErrorHandler errorHandler;
  SemanticAnalizer UUT(errorHandler);
  AstNode* oldAst = new AstNumber(42);
  AstNode* newAst = UUT.transform(*oldAst);
  EXPECT_EQ(oldAst, newAst) <<
    "old AST: " << amendAst(oldAst) << "\n" <<
    "new AST: " << amendAst(newAst);
}

TEST(SemanticAnalizerTest, MAKE_TEST_NAME4(
    an_data_obj_def_WITH_an_initializer_of_a_larger_type,
    transform,
    throws,
    BECAUSE_there_are_no_narrowing_implicit_conversions)) {
  TEST_TRANSFORM_THROWS(
    new AstDataDef(
      new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eBool)),
      new AstNumber(42, ObjTypeFunda::eInt)),
    "");
}

TEST(SemanticAnalizerTest, MAKE_TEST_NAME4(
    an_data_obj_def_WITH_an_initializer_of_a_smaller_type,
    transform,
    throws,
    BECAUSE_currently_there_are_no_implicit_widening_conversions)) {
  TEST_TRANSFORM_THROWS(
    new AstDataDef(
      new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt)),
      new AstNumber(0, ObjTypeFunda::eBool)),
    "");
}

TEST(SemanticAnalizerTest, MAKE_TEST_NAME4(
    a_sequence_with_zero_args,
    transform,
    throws,
    BECAUSE_currently_there_is_no_NOP_operation)) {
  TEST_TRANSFORM_THROWS(new AstOperator(';'), "");
}

