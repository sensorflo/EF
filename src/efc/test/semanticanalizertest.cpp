#include "test.h"
#include "../semanticanalizer.h"
#include "../ast.h"
#include "../objtype.h"
#include "../env.h"
#include "../errorhandler.h"
#include "../astdefaultiterator.h"
#include "../parserext.h"
#include <memory>
#include <string>
using namespace testing;
using namespace std;

class TestingSemanticAnalizer : public SemanticAnalizer {
public:
  TestingSemanticAnalizer(Env& env, ErrorHandler& errorHandler) :
    SemanticAnalizer(env, errorHandler) {};
  using SemanticAnalizer::m_errorHandler;
  using SemanticAnalizer::m_env;
};

void testAstTraversalThrows(AstNode* ast, const string& spec) {
  ENV_ASSERT_TRUE( ast!=NULL );
  Env env;
  ErrorHandler errorHandler;
  TestingSemanticAnalizer UUT(env, errorHandler);
  EXPECT_ANY_THROW( UUT.analyze(*ast) ) << amendSpec(spec) << amendAst(ast);
}

#define TEST_ASTTRAVERSAL_THROWS(ast, spec)                                \
  {                                                                     \
    SCOPED_TRACE("transform called from here (via TEST_ASTTRAVERSAL_THROWS)"); \
    testAstTraversalThrows(ast, spec);                                     \
  }

void testAstTraversalReportsError(TestingSemanticAnalizer& UUT, AstNode* ast,
  Error::No expectedErrorNo, const string& spec) {
  // setup
  ENV_ASSERT_TRUE( ast!=NULL );
  bool foreignThrow = false;
  string excptionwhat;

  // exercise
  try {
    UUT.analyze(*ast);
  }

  // verify that ...

  // ... no foreign exception is thrown
  catch (BuildError& ) { /* nop */  }
  catch (exception& e) { foreignThrow = true; excptionwhat = e.what(); }
  catch (exception* e) { foreignThrow = true; if ( e ) { excptionwhat = e->what(); } }
  catch (...)          { foreignThrow = true; }
  EXPECT_FALSE( foreignThrow ) <<
    amendSpec(spec) << amendAst(ast) << amend(UUT.m_errorHandler) <<
    "\nexceptionwhat: " << excptionwhat;

  // ... only exactly one error is reported
  const ErrorHandler::Container& errors = UUT.m_errorHandler.errors();
  EXPECT_EQ(1, errors.size()) <<
    "Expecting exactly one error\n" <<
    amendSpec(spec) << amend(UUT.m_errorHandler) << amendAst(ast);

  // ... and that that one error has the expected ErrorNo
  EXPECT_EQ(expectedErrorNo, errors.front()->no()) <<
    amendSpec(spec) << amend(UUT.m_errorHandler) << amendAst(ast);
}

#define TEST_ASTTRAVERSAL_REPORTS_ERROR(ast, expectedErrorNo, spec)     \
  {                                                                     \
    SCOPED_TRACE("transform called from here (via TEST_ASTTRAVERSAL_REPORTS_ERROR)"); \
    Env env;                                                            \
    ErrorHandler errorHandler;                                          \
    TestingSemanticAnalizer UUT(env, errorHandler);                     \
    ParserExt pe(UUT.m_env, UUT.m_errorHandler);                        \
    testAstTraversalReportsError(UUT, ast, expectedErrorNo, spec);      \
  }

TEST(SemanticAnalizerTest, MAKE_TEST_NAME4(
    an_data_obj_def_WITH_an_initializer_of_a_larger_type,
    transform,
    throws,
    BECAUSE_there_are_no_narrowing_implicit_conversions)) {
  TEST_ASTTRAVERSAL_THROWS(
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
  TEST_ASTTRAVERSAL_THROWS(
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
  TEST_ASTTRAVERSAL_THROWS(new AstOperator(';'), "");
}

TEST(SemanticAnalizerTest, MAKE_TEST_NAME(
    a_simple_AstDataDecl,
    transform,
    inserts_an_appropriate_SymbolTableEntry_into_Env)) {

  // setup
  Env env;
  ErrorHandler errorHandler;
  TestingSemanticAnalizer UUT(env, errorHandler);
  auto_ptr<AstNode> ast(
    new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt)));

  // exercise
  UUT.analyze(*ast);

  // verify
  shared_ptr<SymbolTableEntry> stentry;
  env.find("x", stentry);
  EXPECT_TRUE( stentry.get() ) << amendAst(ast.get());
  EXPECT_EQ( ObjType::eFullMatch,
    stentry->objType().match( ObjTypeFunda(ObjTypeFunda::eInt)));
}

TEST(SemanticAnalizerTest, MAKE_TEST_NAME(
    a_data_redeclaration_with_matching_type,
    transform,
    succeeds_AND_inserts_an_appropriate_SymbolTableEntry_into_Env_once)) {

  // setup
  Env env;
  ErrorHandler errorHandler;
  TestingSemanticAnalizer UUT(env, errorHandler);
  AstNode* ast =
    new AstOperator(';',
      new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt)),
      new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt)));

  // exercise
  UUT.analyze(*ast);

  // verify
  EXPECT_TRUE( errorHandler.hasNoErrors() ) << amendAst(ast);
  shared_ptr<SymbolTableEntry> stentry;
  env.find("x", stentry);
  EXPECT_TRUE( stentry.get() ) << amendAst(ast);
  EXPECT_EQ( ObjType::eFullMatch,
    stentry->objType().match( ObjTypeFunda(ObjTypeFunda::eInt)));

  // tear down
  delete ast;
}

TEST(SemanticAnalizerTest, MAKE_TEST_NAME(
    a_function_redeclaration_with_matching_type,
    transform,
    succeeds_AND_inserts_an_appropriate_SymbolTableEntry_into_Env_once)) {

  // setup
  Env env;
  ErrorHandler errorHandler;
  TestingSemanticAnalizer UUT(env, errorHandler);
  ParserExt pe(env, errorHandler);
  AstNode* ast =
    new AstOperator(';',
      pe.mkFunDecl("foo"),
      pe.mkFunDecl("foo"));

  // exercise
  UUT.analyze(*ast);

  // verify
  EXPECT_TRUE( errorHandler.hasNoErrors() ) << amendAst(ast);
  shared_ptr<SymbolTableEntry> stentry;
  env.find("foo", stentry);
  EXPECT_TRUE( stentry.get() ) << amendAst(ast);
  EXPECT_EQ( ObjType::eFullMatch,
    stentry->objType().match(
      ObjTypeFun(
        ObjTypeFun::createArgs(),
        new ObjTypeFunda(ObjTypeFunda::eInt))));

  // tear down
  delete ast;
}

TEST(SemanticAnalizerTest, MAKE_TEST_NAME(
    a_redeclaration_with_non_matching_type,
    transform,
    reports_eIncompatibleRedaclaration)) {

  string spec = "Example: two different fundamental types";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstOperator(';',
      new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt)),
      new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eBool))),
    Error::eIncompatibleRedaclaration, "");

  spec = "Example: first function type, then fundamental type";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstOperator(';',
      pe.mkFunDecl("foo"),
      new AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt))),
    Error::eIncompatibleRedaclaration, spec);

  spec = "Example: First fundamental type, then function type";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstOperator(';',
      new AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt)),
      pe.mkFunDecl("foo")),
    Error::eIncompatibleRedaclaration, spec);
}

TEST(SemanticAnalizerTest, MAKE_TEST_NAME(
    a_reference_to_an_unknown_name,
    transform,
    reports_an_eErrUnknownName)) {
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstSymbol("x"),
    Error::eUnknownName, "");

  // separate test for symbol and funcal since currently the two do sadly not
  // share a common implementation
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstFunCall(new AstSymbol("foo")),
    Error::eUnknownName, "");
}

TEST(SemanticAnalizerTest, MAKE_TEST_NAME(
    redefinition_of_an_object_which_means_same_name_and_same_type,
    transform,
    reports_an_eRedefinition)) {

  string spec = "Example: two local variables in implicit main method";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstOperator(';',
      new AstDataDef(new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt))),
      new AstDataDef(new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt)))),
    Error::eRedefinition, spec);

  spec = "Example: two parameters";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    pe.mkFunDef(
      pe.mkFunDecl(
        "foo",
        new AstArgDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt)),
        new AstArgDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt))),
      new AstNumber(42)),
    Error::eRedefinition, spec);

  spec = "Example: a parameter and a local variable in the function's body";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    pe.mkFunDef(
      pe.mkFunDecl("foo", new AstArgDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt))),
      new AstDataDef(new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable)))),
    Error::eRedefinition, spec);

  spec = "Example: two functions";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstOperator(';',
      pe.mkFunDef(pe.mkFunDecl("foo"), new AstNumber(42)),
      pe.mkFunDef(pe.mkFunDecl("foo"), new AstNumber(42))),
    Error::eRedefinition, spec);
}



