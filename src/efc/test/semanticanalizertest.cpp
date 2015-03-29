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
#include <sstream>
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
  if ( ! errors.empty() ) {
    EXPECT_EQ(expectedErrorNo, errors.front()->no()) <<
      amendSpec(spec) << amend(UUT.m_errorHandler) << amendAst(ast);
  }
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
    an_node_whose_childs_need_an_implicit_type_conversion_AND_the_rhs_is_of_larger_type,
    transform,
    reports_eNoImplicitConversion,
    BECAUSE_there_are_no_narrowing_implicit_conversions)) {
  string spec = "Example: Data object definition";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstDataDef(
      new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eBool)),
      new AstNumber(42, ObjTypeFunda::eInt)),
    Error::eNoImplicitConversion, spec);

  spec = "Example: Binary math operator";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstOperator('+',
      new AstNumber(0, ObjTypeFunda::eBool),
      new AstNumber(77, ObjTypeFunda::eInt)),
    Error::eNoImplicitConversion, spec);

  spec = "Example: If else clause (note that there is an excpetion if one type "
    "is noreturn, see other specifications)";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstIf(new AstNumber(0, ObjTypeFunda::eBool),
      new AstNumber(0, ObjTypeFunda::eBool),
      new AstNumber(77, ObjTypeFunda::eInt)),
    Error::eNoImplicitConversion, spec);

  spec = "Example: Body of a function definition must match function's return type";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    pe.mkFunDef(
      pe.mkFunDecl( "foo", new ObjTypeFunda(ObjTypeFunda::eBool)),
      new AstNumber(42)),
    Error::eNoImplicitConversion, spec);
}

TEST(SemanticAnalizerTest, MAKE_TEST_NAME4(
    an_node_whose_childs_need_an_implicit_type_conversion_AND_the_rhs_is_of_smaller_type,
    transform,
    reports_eNoImplicitConversion,
    BECAUSE_currently_there_are_no_implicit_widening_conversions)) {
  string spec = "Example: Data object definition";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstDataDef(
      new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt)),
      new AstNumber(0, ObjTypeFunda::eBool)),
    Error::eNoImplicitConversion, "");

  spec = "Example: Binary math operator";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstOperator('+',
      new AstNumber(42, ObjTypeFunda::eInt),
      new AstNumber(1, ObjTypeFunda::eBool)),
    Error::eNoImplicitConversion, spec);

  spec = "Example: If else clause (note that there is an excpetion if one type "
    "is noreturn, see other specifications)";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstIf(new AstNumber(0, ObjTypeFunda::eBool),
      new AstNumber(77, ObjTypeFunda::eInt),
      new AstNumber(0, ObjTypeFunda::eBool)),
    Error::eNoImplicitConversion, spec);

  spec = "Example: Body of a function definition must match function's return type";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    pe.mkFunDef(
      pe.mkFunDecl( "foo", new ObjTypeFunda(ObjTypeFunda::eInt)),
      new AstNumber(0, ObjTypeFunda::eBool)),
    Error::eNoImplicitConversion, spec);
}


TEST(SemanticAnalizerTest, MAKE_TEST_NAME4(
    an_flow_control_expresssion_WITH_a_condition_whose_type_is_not_bool,
    transform,
    reports_eNoImplicitConversion,
    BECAUSE_currently_there_are_no_implicit_widening_conversions)) {

  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstIf(new AstNumber(0, ObjTypeFunda::eInt),
      new AstNumber(42),
      new AstNumber(77)),
    Error::eNoImplicitConversion, "");

  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstLoop(new AstNumber(0, ObjTypeFunda::eInt),
      new AstNop()),
    Error::eNoImplicitConversion, "");
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
    a_simple_example_of_a_reference_to_an_unknown_name,
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

TEST(SemanticAnalizerTest, MAKE_TEST_NAME4(
    a_data_object_definition_named_x_in_a_block_AND_a_symbol_named_x_after_the_block,
    transform,
    reports_eUnknownName,
    BECAUSE_the_scope_of_data_objects_declarations_is_that_of_the_enclosing_block)) {

  string spec = "Example: Then clause of an if expression is a block.";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstOperator(';',
      new AstIf(
        new AstNumber(0, ObjTypeFunda::eBool),
        new AstDataDef(new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt)))),
      new AstSymbol("x")),
    Error::eUnknownName, spec);

  spec = "Example: Else clause of an if expression is a block.";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstOperator(';',
      new AstIf(
        new AstNumber(0, ObjTypeFunda::eBool),
        new AstNumber(42, ObjTypeFunda::eInt),
        new AstDataDef(new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt)))),
      new AstSymbol("x")),
    Error::eUnknownName, spec);

  spec = "Example: The whole if expesssion is a block. I.e. data object's declared "
    "in the condition can be referenced in both clauses but not after the if expression.";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstOperator(';',
      new AstIf(
        new AstDataDef(new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eBool))),
        new AstNop()),
      new AstSymbol("x")),
    Error::eUnknownName, spec);

  spec = "Example: Body of a loop expression is a block.";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstOperator(';',
      new AstLoop(
        new AstNumber(0, ObjTypeFunda::eBool),
        new AstDataDef(new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt)))),
      new AstSymbol("x")),
    Error::eUnknownName, spec);

  spec = "Example: The whole loop expesssion is a block. I.e. data object's declared "
    "in the condition can be referenced in the body but not after the loop expression.";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstOperator(';',
      new AstLoop(
        new AstDataDef(new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eBool))),
        new AstNop()),
      new AstSymbol("x")),
    Error::eUnknownName, spec);

  spec = "Example: Body of a function definition is a block. Example with a parameter.";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstOperator(';',
      pe.mkFunDef(
        pe.mkFunDecl(
          "foo",
          new ObjTypeFunda(ObjTypeFunda::eVoid),
          new AstArgDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt))),
        new AstNop()),
      new AstSymbol("x")),
    Error::eUnknownName, spec);

  spec = "Example: Body of a function definition is a block. Example with a local data object";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstOperator(';',
      pe.mkFunDef(
        pe.mkFunDecl( "foo", new ObjTypeFunda(ObjTypeFunda::eInt)),
        new AstDataDef(new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt)))),
      new AstSymbol("x")),
    Error::eUnknownName, spec);
}

TEST(SemanticAnalizerTest, MAKE_TEST_NAME3(
    a_data_definition_in_a_block,
    transform,
    inserts_a_symbol_table_entry_with_the_scope_of_the_block)) {

  string spec = "Example: An if expressions with an if clause (being a "
    "distinct block) and an else clause (being a distinct block), each "
    "block defines its own distinct data object.";
  {
    // setup
    Env env;
    ErrorHandler errorHandler;
    TestingSemanticAnalizer UUT(env, errorHandler);

    AstDataDef* thenClauseDef =
      new AstDataDef( new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt)));
    AstSymbol* thenClauseSymbol = new AstSymbol("x");

    AstDataDef* elseClauseDef =
      new AstDataDef( new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt)));
    AstSymbol* elseClauseSymbol = new AstSymbol("x");

    unique_ptr<AstValue> ast{
      new AstIf(
        new AstNumber(0, ObjTypeFunda::eBool),
        new AstOperator(';', thenClauseDef, thenClauseSymbol ),
        new AstOperator(';', elseClauseDef, elseClauseSymbol ))};

    // exercise
    UUT.analyze(*ast.get());

    // verify
    SymbolTableEntry* thenClauseStentry = thenClauseDef->decl().stentry();
    SymbolTableEntry* elseClauseStentry = elseClauseDef->decl().stentry();
    EXPECT_EQ( thenClauseSymbol->stentry(), thenClauseStentry )
      << amendSpec(spec) << amendAst(ast);
    EXPECT_EQ( elseClauseSymbol->stentry(), elseClauseStentry )
      << amendSpec(spec) << amendAst(ast);
    EXPECT_NE( thenClauseStentry, elseClauseStentry )
      << amendSpec(spec) << amendAst(ast);
    EXPECT_TRUE( NULL!=thenClauseStentry )
      << amendSpec(spec) << amendAst(ast);
    EXPECT_TRUE( NULL!=elseClauseStentry )
      << amendSpec(spec) << amendAst(ast);
  }

  spec = "Example: An outer data object definition and a symbol referencing it after "
    "an inner block, and an inner block containing a data definition and "
    "a symbol referencing the inner data object. Concrete example: block created by "
    "the then clause of an if expression.";
  {
    // setup
    Env env;
    ErrorHandler errorHandler;
    TestingSemanticAnalizer UUT(env, errorHandler);
    ParserExt pe(env, errorHandler);

    AstDataDef* outerDef =
      new AstDataDef( new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt)));
    AstSymbol* outerSymbol = new AstSymbol("x");

    AstDataDef* innerDef =
      new AstDataDef( new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt)));
    AstSymbol* innerSymbol = new AstSymbol("x");

    unique_ptr<AstValue> ast{
      pe.mkOperatorTree(";",
        outerDef,
        new AstIf(
          new AstNumber(0, ObjTypeFunda::eBool),
          new AstOperator(';', innerDef, innerSymbol)),
        outerSymbol)};

    // exercise
    UUT.analyze(*ast.get());

    // verify
    SymbolTableEntry* outerStentry = outerDef->decl().stentry();
    SymbolTableEntry* innerStentry = innerDef->decl().stentry();
    EXPECT_EQ( outerSymbol->stentry(), outerStentry )
      << amendSpec(spec) << amendAst(ast);
    EXPECT_EQ( innerSymbol->stentry(), innerStentry )
      << amendSpec(spec) << amendAst(ast);
    EXPECT_NE( outerStentry, innerStentry )
      << amendSpec(spec) << amendAst(ast);
    EXPECT_TRUE( NULL!=outerStentry )
      << amendSpec(spec) << amendAst(ast);
    EXPECT_TRUE( NULL!=innerStentry )
      << amendSpec(spec) << amendAst(ast);
  }

  spec = "Example: An if expression is an own distinct block, i.e. a data "
    "object definition in its condition is visible in the then and in the "
    "else clause.";
  {
    // setup
    Env env;
    ErrorHandler errorHandler;
    TestingSemanticAnalizer UUT(env, errorHandler);
    ParserExt pe(env, errorHandler);

    AstDataDef* def =
      new AstDataDef( new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eBool)));
    AstSymbol* thenClauseSymbol = new AstSymbol("x");
    AstSymbol* elseClauseSymbol = new AstSymbol("x");

    unique_ptr<AstValue> ast{new AstIf( def, thenClauseSymbol, elseClauseSymbol)};

    // exercise
    UUT.analyze(*ast.get());

    // verify
    SymbolTableEntry* defStentry = def->decl().stentry();
    EXPECT_EQ( thenClauseSymbol->stentry(), defStentry )
      << amendSpec(spec) << amendAst(ast);
    EXPECT_EQ( elseClauseSymbol->stentry(), defStentry )
      << amendSpec(spec) << amendAst(ast);
    EXPECT_TRUE( NULL!=defStentry )
      << amendSpec(spec) << amendAst(ast);
  }

  spec = "Example: The body of an loop expression is a bock.";
  {
    // setup
    Env env;
    ErrorHandler errorHandler;
    TestingSemanticAnalizer UUT(env, errorHandler);
    AstDataDef* def =
      new AstDataDef( new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt)));
    AstSymbol* symbol = new AstSymbol("x");
    unique_ptr<AstValue> ast{
      new AstLoop(
        new AstNumber(0, ObjTypeFunda::eBool),
        new AstOperator(';', def, symbol))};

    // exercise
    UUT.analyze(*ast.get());

    // verify
    SymbolTableEntry* defStentry = def->decl().stentry();
    EXPECT_EQ( symbol->stentry(), defStentry)
      << amendSpec(spec) << amendAst(ast);
    EXPECT_TRUE( NULL!=defStentry )
      << amendSpec(spec) << amendAst(ast);
  }

  spec = "Example: An loop expression is an own distinct block, i.e. a data "
    "object definition in its condition is visible in the body.";
  {
    // setup
    Env env;
    ErrorHandler errorHandler;
    TestingSemanticAnalizer UUT(env, errorHandler);
    ParserExt pe(env, errorHandler);

    AstDataDef* def =
      new AstDataDef( new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eBool)));
    AstSymbol* symbol = new AstSymbol("x");

    unique_ptr<AstValue> ast{new AstLoop( def, symbol)};

    // exercise
    UUT.analyze(*ast.get());

    // verify
    SymbolTableEntry* defStentry = def->decl().stentry();
    EXPECT_EQ( symbol->stentry(), def->decl().stentry() )
      << amendSpec(spec) << amendAst(ast);
    EXPECT_TRUE( NULL!=defStentry )
      << amendSpec(spec) << amendAst(ast);
  }
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
      pe.mkFunDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt)),
      pe.mkFunDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt)));

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
        make_shared<ObjTypeFunda>(ObjTypeFunda::eInt))));

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
      pe.mkFunDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt)),
      new AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt))),
    Error::eIncompatibleRedaclaration, spec);

  spec = "Example: First fundamental type, then function type";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstOperator(';',
      new AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt)),
      pe.mkFunDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt))),
    Error::eIncompatibleRedaclaration, spec);
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
        new ObjTypeFunda(ObjTypeFunda::eInt),
        new AstArgDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt)),
        new AstArgDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt))),
      new AstNumber(42)),
    Error::eRedefinition, spec);

  spec = "Example: a parameter and a local variable in the function's body";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    pe.mkFunDef(
      pe.mkFunDecl(
        "foo",
        new ObjTypeFunda(ObjTypeFunda::eInt),
        new AstArgDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt))),
      new AstDataDef(new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable)))),
    Error::eRedefinition, spec);

  spec = "Example: two functions";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstOperator(';',
      pe.mkFunDef(
        pe.mkFunDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt)),
        new AstNumber(42)),
      pe.mkFunDef(
        pe.mkFunDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt)),
        new AstNumber(42))),
    Error::eRedefinition, spec);
}

TEST(SemanticAnalizerTest, MAKE_TEST_NAME4(
    a_nested_function_definition,
    transform,
    succeeds_AND_inserts_both_function_names_in_the_global_namespace,
    BECAUSE_currently_for_simplicity_there_are_no_nested_namespaces)) {

  // setup
  Env env;
  ErrorHandler errorHandler;
  TestingSemanticAnalizer UUT(env, errorHandler);
  ParserExt pe(env, errorHandler);
  AstNode* ast =
    pe.mkFunDef( pe.mkFunDecl("outer", new ObjTypeFunda(ObjTypeFunda::eInt)),
      new AstOperator(';',
        pe.mkFunDef( pe.mkFunDecl("inner", new ObjTypeFunda(ObjTypeFunda::eInt)), new AstNumber(42)),
        new AstNumber(42)));

  // exercise
  UUT.analyze(*ast);

  // verify
  EXPECT_TRUE( errorHandler.hasNoErrors() ) << amendAst(ast);

  ObjTypeFun funType{ ObjTypeFun::createArgs(), make_shared<ObjTypeFunda>(ObjTypeFunda::eInt)};

  shared_ptr<SymbolTableEntry> stentryOuter;
  env.find("outer", stentryOuter);
  EXPECT_TRUE( stentryOuter.get() ) << amendAst(ast);
  EXPECT_TRUE( stentryOuter->objType().matchesFully(funType) );

  shared_ptr<SymbolTableEntry> stentryInner;
  env.find("inner", stentryInner);
  EXPECT_TRUE( stentryInner.get() ) << amendAst(ast);
  EXPECT_TRUE( stentryInner->objType().matchesFully(funType) );

  // tear down
  delete ast;
}

// aka temporary objects are immutable
TEST(SemanticAnalizerTest, MAKE_TEST_NAME2(
    GIVEN_an_access_to_a_sequence_operator,
    THEN_the_access_value_of_seqs_rhs_equals_that_outer_access_value_AND_the_access_value_of_the_lhs_is_eIgnore))
{
  // setup
  Env env;
  ErrorHandler errorHandler;
  TestingSemanticAnalizer UUT(env, errorHandler);
  AstValue* lhsAst = new AstNumber(42);
  AstValue* rhsAst = new AstSymbol("x");
  unique_ptr<AstValue> ast{
    new AstOperator(';',
      new AstDataDef(
        new AstDataDecl("x",
          new ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable))),
      new AstOperator('=', // imposes write access onto the seq operator
        new AstOperator(';', lhsAst, rhsAst), // the seq operator under test
        new AstNumber(77)))};

  // exercise
  UUT.analyze(*ast.get());

  // verify
  EXPECT_EQ( eIgnore, lhsAst->access()) << amendAst(ast);
  EXPECT_EQ( eWrite, rhsAst->access()) << amendAst(ast);
}

// aka temporary objects are immutable
TEST(SemanticAnalizerTest, MAKE_TEST_NAME(
    an_arithmetic_or_logical_operator,
    transform,
    sets_the_objectType_of_the_AstOperator_node_to_the_type_of_the_two_operands_however_with_an_immutable_qualifier)) {

  // this specification only looks at operands with identical types. Other
  // specifications specify how to introduce implicit conversions when the
  // type of the operands don't match

  string spec = "Example: Arithmetic operator (+) with operands of type int -> "
    "result temporary object must be of type int";
  {
    // setup
    Env env;
    ErrorHandler errorHandler;
    TestingSemanticAnalizer UUT(env, errorHandler);
    ParserExt pe(UUT.m_env, UUT.m_errorHandler);
    unique_ptr<AstValue> ast{
      new AstOperator('+',
        new AstNumber(0, ObjTypeFunda::eInt),
        new AstNumber(0, ObjTypeFunda::eInt))};

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_MATCHES_FULLY( ObjTypeFunda(ObjTypeFunda::eInt), ast->objType()) <<
      amendAst(ast) << amendSpec(spec);
  }

  spec = "Example: Logical operator (&&) with operands of type bool -> "
    "result temporary object must be of type bool";
  {
    // setup
    Env env;
    ErrorHandler errorHandler;
    TestingSemanticAnalizer UUT(env, errorHandler);
    ParserExt pe(UUT.m_env, UUT.m_errorHandler);
    unique_ptr<AstValue> ast{
      new AstOperator("&&",
        new AstNumber(0, ObjTypeFunda::eBool),
        new AstNumber(0, ObjTypeFunda::eBool))};

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_MATCHES_FULLY( ObjTypeFunda(ObjTypeFunda::eBool), ast->objType()) <<
      amendAst(ast) << amendSpec(spec);
  }

  spec = "Example: two muttable operands -> result temporary object is still immutable";
  {
    // setup
    Env env;
    ErrorHandler errorHandler;
    TestingSemanticAnalizer UUT(env, errorHandler);
    ParserExt pe(UUT.m_env, UUT.m_errorHandler);
    AstValue* opAst =
      new AstOperator('+',
        new AstSymbol("x"),
        new AstSymbol("x"));
    unique_ptr<AstValue> ast{
      new AstOperator(';',
        new AstDataDef(
          new AstDataDecl("x",
            new ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable))),
        opAst)};

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_MATCHES_FULLY( ObjTypeFunda(ObjTypeFunda::eInt), opAst->objType()) <<
      amendAst(ast) << amendSpec(spec);
  }

  spec = "Example: Sequence operator (;) ignores the type of the lhs operand";
  {
    // setup
    Env env;
    ErrorHandler errorHandler;
    TestingSemanticAnalizer UUT(env, errorHandler);
    ParserExt pe(UUT.m_env, UUT.m_errorHandler);
    unique_ptr<AstValue> ast{
      new AstOperator(';',
        new AstNumber(0, ObjTypeFunda::eInt),
        new AstNumber(0, ObjTypeFunda::eBool))};

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_MATCHES_FULLY( ObjTypeFunda(ObjTypeFunda::eBool), ast->objType()) <<
      amendAst(ast) << amendSpec(spec);
  }
}

// aka temporary objects are immutable
TEST(SemanticAnalizerTest, MAKE_TEST_NAME(
    a_non_dot_assignment_operator,
    transform,
    sets_the_objectType_of_the_AstOperator_node_to_void)) {
  // setup
  Env env;
  ErrorHandler errorHandler;
  TestingSemanticAnalizer UUT(env, errorHandler);
  ParserExt pe(UUT.m_env, UUT.m_errorHandler);
  AstValue* assignmentAst =
    new AstOperator("=",
      new AstSymbol("foo"),
      new AstNumber(77));
  unique_ptr<AstValue> ast{
    new AstOperator(';',
      new AstDataDef(
        new AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable))),
      assignmentAst)};

  // exercise
  UUT.analyze(*ast.get());

  // verify
  EXPECT_MATCHES_FULLY( ObjTypeFunda(ObjTypeFunda::eVoid), assignmentAst->objType()) <<
    amendAst(ast);
}

TEST(SemanticAnalizerTest, MAKE_TEST_NAME(
    a_dot_assignment_operator,
    transform,
    sets_the_objectType_of_the_AstOperator_node_to_lhs_which_includes_being_mutable)) {

  // setup
  Env env;
  ErrorHandler errorHandler;
  TestingSemanticAnalizer UUT(env, errorHandler);
  ParserExt pe(UUT.m_env, UUT.m_errorHandler);
  AstValue* dotAssignmentAst =
    new AstOperator(".=",
      new AstSymbol("x"),
      new AstNumber(0, ObjTypeFunda::eInt));
  unique_ptr<AstValue> ast{
    new AstOperator(';',
      new AstDataDef(
        new AstDataDecl("x",
          new ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable))),
      dotAssignmentAst)};

  // exercise
  UUT.analyze(*ast.get());

  // verify
  EXPECT_MATCHES_FULLY( ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable), dotAssignmentAst->objType()) <<
    amendAst(ast);
}

// aka temporary objects are immutable
TEST(SemanticAnalizerTest, MAKE_TEST_NAME(
    an_comparision_operator,
    transform,
    sets_the_objectType_of_the_AstOperator_node_to_immutable_bool)) {
  // setup
  Env env;
  ErrorHandler errorHandler;
  TestingSemanticAnalizer UUT(env, errorHandler);
  ParserExt pe(UUT.m_env, UUT.m_errorHandler);
  unique_ptr<AstValue> ast{
    new AstOperator("==",
      new AstNumber(42),
      new AstNumber(77))};

  // exercise
  UUT.analyze(*ast.get());

  // verify
  EXPECT_MATCHES_FULLY( ObjTypeFunda(ObjTypeFunda::eBool), ast->objType()) <<
    amendAst(ast);
}

// aka temporary objects are immutable
TEST(SemanticAnalizerTest, MAKE_TEST_NAME(
    a_function_call,
    transform,
    sets_the_objectType_of_the_AstFunCall_node_to_the_return_type_of_the_called_function)) {

  string spec = "Example: function with no args returning bool";
  {
    // setup
    Env env;
    ErrorHandler errorHandler;
    TestingSemanticAnalizer UUT(env, errorHandler);
    ParserExt pe(UUT.m_env, UUT.m_errorHandler);
    AstFunCall* funCall = new AstFunCall(new AstSymbol("foo"));
    unique_ptr<AstValue> ast{
      new AstOperator(';',
        pe.mkFunDef(
          pe.mkFunDecl("foo", new ObjTypeFunda(ObjTypeFunda::eBool)),
          new AstNumber(0, ObjTypeFunda::eBool)),
        funCall)};

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_MATCHES_FULLY( ObjTypeFunda(ObjTypeFunda::eBool), funCall->objType()) <<
      amendAst(ast) << amendSpec(spec);
  }

  spec = "Example: function with no args returning 'mutable' int -> "
    "obj type of expession is still immutable as all temporary objects.";
  {
    // setup
    Env env;
    ErrorHandler errorHandler;
    TestingSemanticAnalizer UUT(env, errorHandler);
    ParserExt pe(UUT.m_env, UUT.m_errorHandler);
    AstFunCall* funCall = new AstFunCall(new AstSymbol("foo"));
    unique_ptr<AstValue> ast{
      new AstOperator(';',
        pe.mkFunDef(
          pe.mkFunDecl("foo", new ObjTypeFunda(ObjTypeFunda::eBool, ObjType::eMutable)),
          new AstNumber(0, ObjTypeFunda::eBool)),
        funCall)};

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_MATCHES_FULLY( ObjTypeFunda(ObjTypeFunda::eBool), funCall->objType()) <<
      amendAst(ast) << amendSpec(spec);
  }
}

TEST(SemanticAnalizerTest, MAKE_TEST_NAME(
    a_cast,
    transform,
    sets_the_objectType_of_the_AstCast_node_to_the_type_of_the_cast)) {

  {
    // setup
    Env env;
    ErrorHandler errorHandler;
    TestingSemanticAnalizer UUT(env, errorHandler);
    ParserExt pe(UUT.m_env, UUT.m_errorHandler);
    unique_ptr<AstValue> ast{
      new AstCast(
        new ObjTypeFunda(ObjTypeFunda::eInt),
        new AstNumber(0, ObjTypeFunda::eBool))};

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_MATCHES_FULLY( ObjTypeFunda(ObjTypeFunda::eInt), ast->objType()) <<
      amendAst(ast);
  }
}

TEST(SemanticAnalizerTest, MAKE_TEST_NAME2(
    GIVEN_a_return_expression,
    THEN_the_objType_is_noreturn)) {

  // setup
  Env env;
  ErrorHandler errorHandler;
  TestingSemanticAnalizer UUT(env, errorHandler);
  ParserExt pe(UUT.m_env, UUT.m_errorHandler);
  AstValue* retAst = new AstReturn(new AstNumber(42, ObjTypeFunda::eInt));
  unique_ptr<AstValue> ast{
    pe.mkFunDef(
      pe.mkFunDecl( "foo", new ObjTypeFunda(ObjTypeFunda::eInt)),
      retAst)};

  // exercise
  UUT.analyze(*ast.get());

  // verify
  EXPECT_MATCHES_FULLY( ObjTypeFunda(ObjTypeFunda::eNoreturn), retAst->objType()) <<
    amendAst(ast);
}

TEST(SemanticAnalizerTest, MAKE_TEST_NAME2(
    GIVEN_a_return_expression_whose_retVal_ObjType_does_match_functions_return_ObjType,
    THEN_it_succeeds)) {

  string spec = "Example: return expression as last (and only) expression in fun's body";
  {
    // setup
    Env env;
    ErrorHandler errorHandler;
    TestingSemanticAnalizer UUT(env, errorHandler);
    ParserExt pe(UUT.m_env, UUT.m_errorHandler);
    AstNode* ast =
      pe.mkFunDef(
        pe.mkFunDecl( "foo", new ObjTypeFunda(ObjTypeFunda::eInt)),
        new AstReturn(new AstNumber(42, ObjTypeFunda::eInt)));

    // exercise
    UUT.analyze(*ast);

    // verify
    EXPECT_TRUE( errorHandler.hasNoErrors() ) << amendAst(ast) << amendSpec(spec);
  }

  spec = "Example: Early return, return expression in an if clause";
  {
    // setup
    Env env;
    ErrorHandler errorHandler;
    TestingSemanticAnalizer UUT(env, errorHandler);
    ParserExt pe(UUT.m_env, UUT.m_errorHandler);
    AstNode* ast =
      pe.mkFunDef(
        pe.mkFunDecl( "foo", new ObjTypeFunda(ObjTypeFunda::eInt)),
        new AstOperator(';',
          new AstIf(
            new AstNumber(0, ObjTypeFunda::eBool),
            new AstReturn(new AstNumber(42, ObjTypeFunda::eInt))), // early return
          new AstNumber(77, ObjTypeFunda::eInt)));
    // exercise
    UUT.analyze(*ast);

    // verify
    EXPECT_TRUE( errorHandler.hasNoErrors() ) << amendAst(ast);
  }

  spec = "Example: Nested functions with different return types.";
  {
    // setup
    Env env;
    ErrorHandler errorHandler;
    TestingSemanticAnalizer UUT(env, errorHandler);
    ParserExt pe(UUT.m_env, UUT.m_errorHandler);
    AstNode* ast =
      pe.mkFunDef(
        pe.mkFunDecl( "outer", new ObjTypeFunda(ObjTypeFunda::eInt)),
        new AstOperator(';',

          pe.mkFunDef(
            pe.mkFunDecl( "inner", new ObjTypeFunda(ObjTypeFunda::eBool)),
            new AstReturn(new AstNumber(0, ObjTypeFunda::eBool))),

          new AstReturn(new AstNumber(42, ObjTypeFunda::eInt))));

    // exercise
    UUT.analyze(*ast);

    // verify
    EXPECT_TRUE( errorHandler.hasNoErrors() ) << amendAst(ast);
  }
}

TEST(SemanticAnalizerTest, MAKE_TEST_NAME2(
    GIVEN_a_return_expression_whose_retVal_ObjType_does_not_match_functions_return_ObjType,
    THEN_it_reports_eNoImplicitConversion)) {

  string spec = "Example: return expression as last (and only) expression in fun's body";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    pe.mkFunDef(
      pe.mkFunDecl( "foo", new ObjTypeFunda(ObjTypeFunda::eInt)),
      new AstReturn(new AstNumber(42, ObjTypeFunda::eBool))),
    Error::eNoImplicitConversion, "");

  spec = "Example: Early return, return expression in an if clause";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    pe.mkFunDef(
      pe.mkFunDecl( "foo", new ObjTypeFunda(ObjTypeFunda::eInt)),
      new AstOperator(';',
        new AstIf(
          new AstNumber(0, ObjTypeFunda::eBool),
          new AstReturn(new AstNumber(0, ObjTypeFunda::eBool))), // early return
        new AstNumber(77, ObjTypeFunda::eInt))),
    Error::eNoImplicitConversion, "");

  spec = "Example: Nested function. Return expression in body of inner function "
    "does not match return type of its function, but would match that of the "
    "outer function. Naturally that is still an error.";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    pe.mkFunDef(
      pe.mkFunDecl( "outer", new ObjTypeFunda(ObjTypeFunda::eInt)),
      new AstOperator(';',

        pe.mkFunDef(
          pe.mkFunDecl( "inner", new ObjTypeFunda(ObjTypeFunda::eBool)),
          new AstReturn(new AstNumber(0, ObjTypeFunda::eInt))), // return under test

        new AstNumber(42))),
    Error::eNoImplicitConversion, "");
}

TEST(SemanticAnalizerTest, MAKE_TEST_NAME2(
    GIVEN_an_if_expression_with_else_clause_AND_none_of_the_two_clauses_is_of_obj_type_noreturn,
    THEN_the_if_expressions_obj_type_is_exactly_that_of_either_of_its_two_clauses)) {

  // this specification only looks at operands with identical types. Other
  // specifications specify how to introduce implicit conversions when the
  // type of the operands don't match

  string spec = "Example: both clauses of type bool -> whole if is of type bool";
  {
    // setup
    Env env;
    ErrorHandler errorHandler;
    TestingSemanticAnalizer UUT(env, errorHandler);
    ParserExt pe(UUT.m_env, UUT.m_errorHandler);
    unique_ptr<AstValue> ast{
      new AstIf(
        new AstNumber(0, ObjTypeFunda::eBool),
        new AstNumber(1, ObjTypeFunda::eBool),
        new AstNumber(0, ObjTypeFunda::eBool))};

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_MATCHES_FULLY( ObjTypeFunda(ObjTypeFunda::eBool), ast->objType()) <<
      amendAst(ast);
  }

  spec = "Example: both clauses of type void -> whole if is of type void";
  {
    // setup
    Env env;
    ErrorHandler errorHandler;
    TestingSemanticAnalizer UUT(env, errorHandler);
    ParserExt pe(UUT.m_env, UUT.m_errorHandler);
    unique_ptr<AstValue> ast{
      new AstIf(
        new AstNumber(0, ObjTypeFunda::eBool),
        new AstNop(),
        new AstNop())};

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_MATCHES_FULLY( ObjTypeFunda(ObjTypeFunda::eVoid), ast->objType()) <<
      amendAst(ast);
  }

  spec = "Example: both clauses are of type mutable-bool -> whole if is of type mutable-bool";
  {
    // setup
    Env env;
    ErrorHandler errorHandler;
    TestingSemanticAnalizer UUT(env, errorHandler);
    ParserExt pe(UUT.m_env, UUT.m_errorHandler);
    AstValue* if_ = new AstIf(
      new AstNumber(0, ObjTypeFunda::eBool),
      new AstSymbol("x"),
      new AstSymbol("x"));
    unique_ptr<AstValue> ast{
      new AstOperator(';',
        new AstDataDecl("x",
          new ObjTypeFunda(ObjTypeFunda::eBool, ObjType::eMutable)),
        if_)};

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_MATCHES_FULLY( ObjTypeFunda(ObjTypeFunda::eBool, ObjType::eMutable), ast->objType()) <<
      amendAst(ast);
  }
}

TEST(SemanticAnalizerTest, MAKE_TEST_NAME2(
    GIVEN_an_if_expression_with_else_clause_AND_either_of_the_two_clauses_is_of_obj_type_noreturn,
    THEN_the_if_expressions_obj_type_is_exactly_that_of_the_other_clause)) {

  string spec = "Example: then clause of type bool, else clause of type noreturn -> "
    "the if expression's type is bool.";
  {
    // setup
    Env env;
    ErrorHandler errorHandler;
    TestingSemanticAnalizer UUT(env, errorHandler);
    ParserExt pe(UUT.m_env, UUT.m_errorHandler);
    AstValue* astIf =
      new AstIf(
        new AstNumber(0, ObjTypeFunda::eBool),
        new AstNumber(1, ObjTypeFunda::eBool),
        new AstReturn(new AstNumber(42, ObjTypeFunda::eInt)));
    unique_ptr<AstValue> ast{
      pe.mkFunDef(
        pe.mkFunDecl( "foo", new ObjTypeFunda(ObjTypeFunda::eInt)),
        new AstOperator(';',
          astIf,
          new AstNumber(42)))};

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_MATCHES_FULLY( ObjTypeFunda(ObjTypeFunda::eBool), astIf->objType()) <<
      amendAst(ast);
  }

  spec = "Example: then clause of type noreturn, else clause of type bool -> "
    "the if expression's type is bool.";
  {
    // setup
    Env env;
    ErrorHandler errorHandler;
    TestingSemanticAnalizer UUT(env, errorHandler);
    ParserExt pe(UUT.m_env, UUT.m_errorHandler);
    AstValue* astIf =
      new AstIf(
        new AstNumber(0, ObjTypeFunda::eBool),
        new AstReturn(new AstNumber(42, ObjTypeFunda::eInt)),
        new AstNumber(1, ObjTypeFunda::eBool));
    unique_ptr<AstValue> ast{
      pe.mkFunDef(
        pe.mkFunDecl( "foo", new ObjTypeFunda(ObjTypeFunda::eInt)),
        new AstOperator(';',
          astIf,
          new AstNumber(42)))};

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_MATCHES_FULLY( ObjTypeFunda(ObjTypeFunda::eBool), astIf->objType()) <<
      amendAst(ast);
  }

  spec = "Example: both clauses of type noreturn -> the if expression's type is noreturn.";
  {
    // setup
    Env env;
    ErrorHandler errorHandler;
    TestingSemanticAnalizer UUT(env, errorHandler);
    ParserExt pe(UUT.m_env, UUT.m_errorHandler);
    AstValue* astIf =
      new AstIf(
        new AstNumber(0, ObjTypeFunda::eBool),
        new AstReturn(new AstNumber(42, ObjTypeFunda::eInt)),
        new AstReturn(new AstNumber(42, ObjTypeFunda::eInt)));
    unique_ptr<AstValue> ast{
      pe.mkFunDef(
        pe.mkFunDecl( "foo", new ObjTypeFunda(ObjTypeFunda::eInt)),
        astIf)};

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_MATCHES_FULLY( ObjTypeFunda(ObjTypeFunda::eNoreturn), astIf->objType()) <<
      amendAst(ast);
  }
}

TEST(SemanticAnalizerTest, MAKE_TEST_NAME3(
    GIVEN_an_if_expression_with_no_else_clause,
    THEN_the_if_expressions_obj_type_is_void,
    BECAUSE_the_type_of_the_inexistent_else_clause_is_void)) {

  string spec = "Example: then clause is of type bool -> if expression is of type void.";
  {
    // setup
    Env env;
    ErrorHandler errorHandler;
    TestingSemanticAnalizer UUT(env, errorHandler);
    ParserExt pe(UUT.m_env, UUT.m_errorHandler);
    unique_ptr<AstValue> ast{
      new AstIf(
        new AstNumber(0, ObjTypeFunda::eBool),
        new AstNumber(1, ObjTypeFunda::eInt))};

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_MATCHES_FULLY( ObjTypeFunda(ObjTypeFunda::eVoid), ast->objType()) <<
      amendAst(ast);
  }

  spec = "Example: then clause is of type noreturn -> if expression is of type void.";
  {
    // setup
    Env env;
    ErrorHandler errorHandler;
    TestingSemanticAnalizer UUT(env, errorHandler);
    ParserExt pe(UUT.m_env, UUT.m_errorHandler);
    AstValue* astIf =
      new AstIf(
        new AstNumber(0, ObjTypeFunda::eBool),
        new AstReturn(new AstNumber(42, ObjTypeFunda::eInt)));
    unique_ptr<AstValue> ast{
      pe.mkFunDef(
        pe.mkFunDecl( "foo", new ObjTypeFunda(ObjTypeFunda::eInt)),
        new AstOperator(';',
          astIf,
          new AstNumber(42)))};

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_MATCHES_FULLY( ObjTypeFunda(ObjTypeFunda::eVoid), astIf->objType()) <<
      amendAst(ast);
  }
}

TEST(SemanticAnalizerTest, MAKE_TEST_NAME2(
    GIVEN_an_access_to_an_if_expression,
    THEN_the_access_value_of_both_clauses_equal_that_outer_access_value))
{
  string spec = "Example: read access";
  {
    // setup
    Env env;
    ErrorHandler errorHandler;
    TestingSemanticAnalizer UUT(env, errorHandler);
    AstValue* thenClauseAst = new AstSymbol("x");
    AstValue* elseClauseAst = new AstSymbol("x");
    unique_ptr<AstValue> ast{
      new AstOperator(';', // imposes read access on its rhs
        new AstDataDef(
          new AstDataDecl("x",
            new ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable))),
        new AstIf( // the if expression under test
          new AstNumber(0, ObjTypeFunda::eBool),
          thenClauseAst, elseClauseAst))};

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_EQ( eRead, thenClauseAst->access()) << amendAst(ast);
    EXPECT_EQ( eRead, thenClauseAst->access()) << amendAst(ast);
  }

  spec = "Example: write access";
  {
    // setup
    Env env;
    ErrorHandler errorHandler;
    TestingSemanticAnalizer UUT(env, errorHandler);
    AstValue* thenClauseAst = new AstSymbol("x");
    AstValue* elseClauseAst = new AstSymbol("x");
    unique_ptr<AstValue> ast{
      new AstOperator(';',
        new AstDataDef(
          new AstDataDecl("x",
            new ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable))),
        new AstOperator('=', // imposes write access onto the if expression
          new AstIf( // the if expression under test
            new AstNumber(0, ObjTypeFunda::eBool),
            thenClauseAst, elseClauseAst),
          new AstNumber(77)))};

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_EQ( eWrite, thenClauseAst->access()) << amendAst(ast);
    EXPECT_EQ( eWrite, thenClauseAst->access()) << amendAst(ast);
  }
}


TEST(SemanticAnalizerTest, MAKE_TEST_NAME3(
    GIVEN_an_loop_expression,
    THEN_the_loop_expressions_obj_type_is_void,
    BEAUSE_the_condition_might_prevent_the_block_from_ever_being_executed)) {

  string spec = "Example: obj type of body is int -> obj type of loop is still and always void";
  {
    // setup
    Env env;
    ErrorHandler errorHandler;
    TestingSemanticAnalizer UUT(env, errorHandler);
    unique_ptr<AstValue> ast{
      new AstLoop(
        new AstNumber(0, ObjTypeFunda::eBool),
        new AstNumber(42))};

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_MATCHES_FULLY( ObjTypeFunda(ObjTypeFunda::eVoid), ast->objType()) <<
      amendAst(ast);
  }

  spec = "Example: obj type of body is noret -> obj type of loop is still and always void";
  {
    // setup
    Env env;
    ErrorHandler errorHandler;
    TestingSemanticAnalizer UUT(env, errorHandler);
    ParserExt pe(UUT.m_env, UUT.m_errorHandler);
    AstLoop* loop =
      new AstLoop(
        new AstNumber(0, ObjTypeFunda::eBool),
        new AstReturn(new AstNumber(42)));
    unique_ptr<AstValue> ast{
        pe.mkFunDef(
          pe.mkFunDecl( "foo", new ObjTypeFunda(ObjTypeFunda::eInt)),
          new AstOperator(';', loop, new AstNumber(42)))};

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_MATCHES_FULLY( ObjTypeFunda(ObjTypeFunda::eVoid), loop->objType()) <<
      amendAst(ast);
  }
}

TEST(SemanticAnalizerTest, MAKE_TEST_NAME2(
    GIVEN_an_loop_expression,
    THEN_the_access_value_of_the_condition_is_eRead_AND_the_access_value_of_the_body_is_eIgnore))
{
  // setup
  Env env;
  ErrorHandler errorHandler;
  TestingSemanticAnalizer UUT(env, errorHandler);
  AstValue* condition = new AstNumber(0, ObjTypeFunda::eBool);
  AstValue* body = new AstNumber(42);
  unique_ptr<AstValue> ast{ new AstLoop( condition, body)};

  // exercise
  UUT.analyze(*ast.get());

  // verify
  EXPECT_EQ( eRead, condition->access()) << amendAst(ast);
  EXPECT_EQ( eIgnore, body->access()) << amendAst(ast);
}

TEST(SemanticAnalizerTest, MAKE_TEST_NAME(
    an_assignment_to_an_immutable_data_object,
    transform,
    reports_an_eWriteToImmutable)) {

  string spec = "Example: Assignment to an symbol standing for a previously "
    "defined immutable data object";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstOperator(';',
      new AstDataDef(
        new AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt)),
        new AstNumber(42)),
      new AstOperator('=',
        new AstSymbol("foo"),
        new AstNumber(77))),
    Error::eWriteToImmutable, "");

  spec = "Example: Assignment directly to the definition of an "
    "immutable data object";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstOperator('=',
      new AstDataDef(
        new AstDataDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt)),
        new AstNumber(42)),
      new AstNumber(77)),
    Error::eWriteToImmutable, "");

  spec = "Example: Assignment to the temporary object resulting from a "
    "math operator";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstOperator('=',
      new AstOperator('+',
        new AstNumber(42),
        new AstNumber(77)),
      new AstNumber(88)),
    Error::eWriteToImmutable, "");
}

TEST(SemanticAnalizerTest, MAKE_TEST_NAME(
    a_function_call_to_an_defined_function_WITH_incorrect_an_argument_count_or_incorrect_argument_type,
    transform,
    reports_an_eInvalidArguments)) {

  string spec = "Function foo expects one arg, but zero where passed on call";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstOperator(';',
      pe.mkFunDef(
        pe.mkFunDecl(
          "foo",
          new ObjTypeFunda(ObjTypeFunda::eInt),
          new AstArgDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt))),
        new AstNumber(42)),
      new AstFunCall(new AstSymbol("foo"))),
    Error::eInvalidArguments, spec);

  spec = "Function foo expects no args, but one arg was passed on call";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstOperator(';',
      pe.mkFunDef(
        pe.mkFunDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt)),
        new AstNumber(42)),
      new AstFunCall(new AstSymbol("foo"), new AstCtList(new AstNumber(0)))),
    Error::eInvalidArguments, spec);

  spec = "Function foo expects one bool arg, but one int is passed";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstOperator(';',
      pe.mkFunDef(
        pe.mkFunDecl(
          "foo",
          new ObjTypeFunda(ObjTypeFunda::eInt),
          new AstArgDecl("x", new ObjTypeFunda(ObjTypeFunda::eBool))),
        new AstNumber(42)),
      new AstFunCall(new AstSymbol("foo"), new AstCtList(new AstNumber(0, ObjTypeFunda::eInt)))),
    Error::eInvalidArguments, spec);

  spec = "Function foo expects one int arg, but one bool is passed";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstOperator(';',
      pe.mkFunDef(
        pe.mkFunDecl(
          "foo",
          new ObjTypeFunda(ObjTypeFunda::eInt),
          new AstArgDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt))),
        new AstNumber(42)),
      new AstFunCall(new AstSymbol("foo"), new AstCtList(new AstNumber(0, ObjTypeFunda::eBool)))),
    Error::eInvalidArguments, spec);
}

TEST(SemanticAnalizerTest, MAKE_TEST_NAME(
    a_function_call_to_an_defined_function_WITH_correct_number_of_arguments_and_types,
    transform,
    succeeds)) {

  string spec = "Parameter is immutable, argument is mutable";
  {
    // setup
    Env env;
    ErrorHandler errorHandler;
    TestingSemanticAnalizer UUT(env, errorHandler);
    ParserExt pe(UUT.m_env, UUT.m_errorHandler);
    unique_ptr<AstValue> ast{
      pe.mkOperatorTree(";",
        pe.mkFunDef(
          pe.mkFunDecl(
            "foo",
            new ObjTypeFunda(ObjTypeFunda::eInt),
            new AstArgDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt))),
          new AstNumber(42)),
        new AstDataDef(
          new AstDataDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable))),
        new AstFunCall(new AstSymbol("foo"),
          new AstCtList(new AstSymbol("x"))))};

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_TRUE( errorHandler.hasNoErrors() ) << amendSpec(spec) << amendAst(ast);
  }

  spec = "Parameter is mutable, argument is immutable";
  {
    // setup
    Env env;
    ErrorHandler errorHandler;
    TestingSemanticAnalizer UUT(env, errorHandler);
    ParserExt pe(UUT.m_env, UUT.m_errorHandler);
    unique_ptr<AstValue> ast{
      new AstOperator(';',
        pe.mkFunDef(
          pe.mkFunDecl(
            "foo",
            new ObjTypeFunda(ObjTypeFunda::eInt),
            new AstArgDecl("x", new ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable))),
          new AstNumber(42)),
        new AstFunCall(new AstSymbol("foo"),
          new AstCtList(new AstNumber(0, ObjTypeFunda::eInt))))};

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_TRUE( errorHandler.hasNoErrors() ) << amendSpec(spec) << amendAst(ast);
  }
}

TEST(SemanticAnalizerTest, MAKE_TEST_NAME(
    an_operator_WITH_an_argument_whose_obj_type_does_not_have_that_operator_as_member,
    transform,
    reports_eNoSuchMember)) {
  // Note that after introducing implicit casts, the objec types of the
  // operands always match sauf qualifiers. That is specified by other
  // tests.

  // Also note that operators are semantically always really member method
  // calls, also for builtin types.

  string spec = "Example: Arithemtic operator (+) with argument not being of arithmetic class (here bool)";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstOperator('+',
      new AstNumber(0, ObjTypeFunda::eBool),
      new AstNumber(0, ObjTypeFunda::eBool)),
    Error::eNoSuchMember, spec);

  spec = "Example: Arithemtic operator (+) with argument not being of arithmetic class (here function)";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstOperator(';',
      pe.mkFunDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt)),
      new AstOperator('+',
        new AstSymbol("foo"),
        new AstSymbol("foo"))),
    Error::eNoSuchMember, spec);

  spec = "Example: Logic operator (&&) with argument not being bool (here int)";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstOperator("&&",
      new AstNumber(0, ObjTypeFunda::eInt),
      new AstNumber(0, ObjTypeFunda::eInt)),
    Error::eNoSuchMember, spec);

  spec = "Example: Comparision operator (==) with argument not being of scalar class (here function)";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstOperator(';',
      pe.mkFunDecl("foo", new ObjTypeFunda(ObjTypeFunda::eInt)),
      new AstOperator("==",
        new AstSymbol("foo"),
        new AstSymbol("foo"))),
    Error::eNoSuchMember, spec);
}

TEST(SemanticAnalizerTest, MAKE_TEST_NAME(
    an_cast_aka_constructor_WITH_an_argument_whose_obj_type_is_not_any_of_,
    transform,
    reports_eNoSuchMember)) {

  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstCast(new ObjTypeFunda(ObjTypeFunda::eVoid), new AstNumber(42)),
    Error::eNoSuchMember, "");
}

TEST(SemanticAnalizerTest, MAKE_TEST_NAME3(
    a_return_expression_outside_a_function_body,
    transform,
    reports_an_eNotInFunBodyContext)) {
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstReturn( new AstNumber(0)),
    Error::eNotInFunBodyContext, "");
}

TEST(SemanticAnalizerTest, MAKE_TEST_NAME4(
    an_expression_of_obj_type_noreturn_as_lhs_to_the_sequence_operator,
    transform,
    reports_an_eUnreachableCode,
    BECAUSE_the_rhs_can_not_be_reached)) {
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    pe.mkFunDef(
      pe.mkFunDecl( "foo", new ObjTypeFunda(ObjTypeFunda::eInt)),
      new AstOperator(';',
        new AstReturn(new AstNumber(42)),
        new AstNumber(77))),
    Error::eUnreachableCode, "");
}

TEST(SemanticAnalizerTest, MAKE_TEST_NAME(
    an_node_computing_an_value_at_runtime_AND_has_no_side_effects_AND_its_result_is_not_used,
    transform,
    reports_eComputedValueNotUsed)) {
  string spec = "Example: operator + being lhs of sequence operator";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstOperator(';',
      new AstOperator('+',
        new AstNumber(42),
        new AstNumber(77)),
      new AstNumber(11)),
    Error::eComputedValueNotUsed, "")
}
