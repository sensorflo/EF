#include "test.h"
#include "../semanticanalizer.h"
#include "../ast.h"
#include "../objtype.h"
#include "../env.h"
#include "../object.h"
#include "../errorhandler.h"
#include "../astdefaultiterator.h"
#include "../parserext.h"
#include <memory>
#include <string>
#include <sstream>
using namespace testing;
using namespace std;

/** \file
Since SemanticAnalizer and ParserExt don't have clearly separated
responsibilities, see the comments of the two, this test test responsibilities
which can be implemented by either of the two. */

class TestingSemanticAnalizer : public SemanticAnalizer {
public:
  TestingSemanticAnalizer(Env& env, ErrorHandler& errorHandler) :
    SemanticAnalizer(env, errorHandler) {};
  using SemanticAnalizer::m_errorHandler;
  using SemanticAnalizer::m_env;
};

void verifyAstTraversal(TestingSemanticAnalizer& UUT, AstNode* ast,
  Error::No expectedErrorNo, const string& spec, bool foreignThrow,
  string excptionwhat) {

  // verify no foreign exception where thrown
  EXPECT_FALSE( foreignThrow ) <<
    amendSpec(spec) << amendAst(ast) << amend(UUT.m_errorHandler) <<
    "\nexceptionwhat: " << excptionwhat;


  // verify that as expected no error was reported
  const ErrorHandler::Container& errors = UUT.m_errorHandler.errors();
  if ( expectedErrorNo == Error::eNone ) {
    EXPECT_TRUE(errors.empty()) <<
      "Expecting no error\n" <<
      amendSpec(spec) << amend(UUT.m_errorHandler) << amendAst(ast);
  }

  else {
    // verify that only exactly one error is reported
    EXPECT_EQ(1, errors.size()) <<
      "Expecting exactly one error\n" <<
      amendSpec(spec) << amend(UUT.m_errorHandler) << amendAst(ast);

    // ... and that that one error has the expected ErrorNo
    if ( ! errors.empty() ) {
      EXPECT_EQ(expectedErrorNo, errors.front()->no()) <<
        amendSpec(spec) << amend(UUT.m_errorHandler) << amendAst(ast);
    }
  }
}

#define TEST_ASTTRAVERSAL(ast, expectedErrorNo, spec)                   \
  Env env;                                                              \
  ErrorHandler errorHandler;                                            \
  TestingSemanticAnalizer UUT(env, errorHandler);                       \
  ParserExt pe(UUT.m_env, UUT.m_errorHandler);                          \
  bool foreignThrow = false;                                            \
  string excptionwhat;                                                  \
  AstNode* ast_ = nullptr;                                              \
  try {                                                                 \
    ast_ = (ast);                                                       \
    UUT.analyze(*ast_);                                                 \
  }                                                                     \
  catch (BuildError& ) { /* nop */  }                                   \
  catch (exception& e) { foreignThrow = true; excptionwhat = e.what(); } \
  catch (exception* e) { foreignThrow = true; if ( e ) { excptionwhat = e->what(); } } \
  catch (...)          { foreignThrow = true; }                         \
  verifyAstTraversal(UUT, ast_, expectedErrorNo, spec, foreignThrow, excptionwhat);

/** Actually does not only test traversal of AST, but also creation of it
with ParserExt, see also file's comment. */
#define TEST_ASTTRAVERSAL_REPORTS_ERROR(ast, expectedErrorNo, spec)     \
  {                                                                     \
    SCOPED_TRACE("testAstTraversal called from here (via TEST_ASTTRAVERSAL_REPORTS_ERROR)"); \
    TEST_ASTTRAVERSAL(ast, expectedErrorNo, spec)                       \
  }

/** Analogous to TEST_ASTTRAVERSAL_REPORTS_ERROR */
#define TEST_ASTTRAVERSAL_SUCCEEDS_WITHOUT_ERRORS(ast, spec)            \
  {                                                                     \
    SCOPED_TRACE("testAstTraversal called from here (via TEST_ASTTRAVERSAL_SUCCEEDS_WITHOUT_ERRORS)"); \
    TEST_ASTTRAVERSAL(ast, Error::eNone, spec)                          \
  }

TEST(SemanticAnalizerTest, MAKE_TEST_NAME4(
    an_node_whose_childs_need_an_implicit_type_conversion_AND_the_rhs_is_of_larger_type,
    transform,
    reports_eNoImplicitConversion,
    BECAUSE_there_are_no_narrowing_implicit_conversions)) {
  string spec = "Example: Data object definition";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstDataDef("x",
      ObjTypeFunda::eBool,
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
    pe.mkFunDef("foo", ObjTypeFunda::eBool, new AstNumber(42)),
    Error::eNoImplicitConversion, spec);
}

TEST(SemanticAnalizerTest, MAKE_TEST_NAME4(
    an_node_whose_childs_need_an_implicit_type_conversion_AND_the_rhs_is_of_smaller_type,
    transform,
    reports_eNoImplicitConversion,
    BECAUSE_currently_there_are_no_implicit_widening_conversions)) {
  string spec = "Example: Data object definition";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstDataDef("x", ObjTypeFunda::eInt,
      new AstNumber(0, ObjTypeFunda::eBool)),
    Error::eNoImplicitConversion, spec);

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
    pe.mkFunDef("foo", ObjTypeFunda::eInt,
      new AstNumber(0, ObjTypeFunda::eBool)),
    Error::eNoImplicitConversion, spec);
}

TEST(SemanticAnalizerTest, MAKE_TEST_NAME2(
    GIVEN_a_logical_and_or_logical_or_operator_WITH_an_rhs_of_type_noreturn,
    THEN_it_succeeds)) {

  TEST_ASTTRAVERSAL_SUCCEEDS_WITHOUT_ERRORS(
    pe.mkFunDef("foo", ObjTypeFunda::eBool,
      new AstOperator(
        "&&",
        new AstNumber(0, ObjTypeFunda::eBool),
        new AstReturn(new AstNumber(0, ObjTypeFunda::eBool)))),
    "");

  TEST_ASTTRAVERSAL_SUCCEEDS_WITHOUT_ERRORS(
    pe.mkFunDef("foo", ObjTypeFunda::eBool,
      new AstOperator(
        "||",
        new AstNumber(0, ObjTypeFunda::eBool),
        new AstReturn(new AstNumber(0, ObjTypeFunda::eBool)))),
    "");
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
    a_simple_data_obj_definition,
    transform,
    inserts_an_appropriate_symbol_table_entry_into_Env)) {

  string spec = "Example: local immutable int";
  {
    // setup
    Env env;
    ErrorHandler errorHandler;
    TestingSemanticAnalizer UUT(env, errorHandler);
    const auto objType = make_shared<ObjTypeFunda>(ObjTypeFunda::eInt);
    unique_ptr<AstNode> ast(new AstDataDef("x", objType));

    // exercise
    UUT.analyze(*ast);

    // verify
    shared_ptr<Entity> entity;
    shared_ptr<Object> object;
    env.find("x", entity);
    EXPECT_TRUE( entity.get() ) << amendAst(ast.get());
    EXPECT_NO_THROW( object = std::dynamic_pointer_cast<Object>(entity));
    EXPECT_MATCHES_FULLY( *objType, object->objType()) << amendAst(ast.get());
  }

  spec = "Example: static immutable int";
  {
    // setup
    Env env;
    ErrorHandler errorHandler;
    TestingSemanticAnalizer UUT(env, errorHandler);
    const auto objType = make_shared<ObjTypeFunda>(ObjTypeFunda::eInt);
    unique_ptr<AstNode> ast(new AstDataDef("x", objType, StorageDuration::eStatic));

    // exercise
    UUT.analyze(*ast);

    // verify
    shared_ptr<Entity> entity;
    env.find("x", entity);
    EXPECT_TRUE( entity.get() ) << amendAst(ast.get());
    const auto object = std::dynamic_pointer_cast<Object>(entity);
    EXPECT_MATCHES_FULLY( *objType, object->objType()) << amendAst(ast.get());
  }
}

TEST(SemanticAnalizerTest, MAKE_TEST_NAME(
    a_static_data_obj_definition_initialized_with_an_non_ctconst_expression,
    transform,
    reports_eCTConstRequired)) {
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstSeq(
      pe.mkFunDef("foo", ObjTypeFunda::eInt, new AstNumber(42)),
      new AstDataDef("x", make_shared<ObjTypeFunda>(ObjTypeFunda::eInt),
        StorageDuration::eStatic,
        new AstFunCall(new AstSymbol("foo")))),
    Error::eCTConstRequired, "");
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

TEST(SemanticAnalizerTest, MAKE_TEST_NAME(
    a_reference_to_an_entity_other_than_local_data_object_before_its_definition,
    transform,
    succeeds)) {

  string spec = "Example: function call before function definition";
  TEST_ASTTRAVERSAL_SUCCEEDS_WITHOUT_ERRORS(
    new AstSeq(
      new AstFunCall(new AstSymbol("foo")),
      pe.mkFunDef("foo", ObjTypeFunda::eVoid, new AstNop())),
    "");

  spec = "Example: static data - !!!! Not yet implemented, "
    "currently behaves as local data !!!!";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstSeq(
      new AstSymbol("x"),
      new AstDataDef("x",
        make_shared<ObjTypeFunda>(ObjTypeFunda::eInt), StorageDuration::eStatic)),
    Error::eUnknownName, "");
}

TEST(SemanticAnalizerTest, MAKE_TEST_NAME3(
    a_data_obj_definition_in_a_block,
    transform,
    inserts_a_symbol_table_entry_with_the_scope_of_the_block)) {

  // setup
  Env env;
  ErrorHandler errorHandler;
  TestingSemanticAnalizer UUT(env, errorHandler);
  const auto dataDef =
    new AstDataDef("x", ObjTypeFunda::eInt);
  const auto symbol = new AstSymbol("x");
  unique_ptr<AstObject> ast{
    new AstBlock(
      new AstSeq( dataDef, symbol))};

  // exercise
  UUT.analyze(*ast.get());

  // verify
  const auto dataDefObject = dataDef->object();
  EXPECT_TRUE( nullptr!=dataDefObject ) << amendAst(ast);
  EXPECT_EQ( dataDefObject, symbol->object()) << amendAst(ast);
}

TEST(SemanticAnalizerTest, MAKE_TEST_NAME4(
    a_local_data_object_definition_named_x_in_a_block_AND_a_symbol_named_x_after_the_block,
    transform,
    reports_eUnknownName,
    BECAUSE_the_scope_of_local_data_objects_declarations_is_that_of_the_enclosing_block)) {
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstSeq(
      new AstBlock(
        new AstDataDef("x", ObjTypeFunda::eInt)),
      new AstSymbol("x")),
    Error::eUnknownName, "");
}

TEST(SemanticAnalizerTest, MAKE_TEST_NAME(
    a_redefinition,
    transform,
    reports_eRedefinition)) {

  string spec = "Example: types differs: two different fundamental types";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstSeq(
      new AstDataDef("x", ObjTypeFunda::eInt),
      new AstDataDef("x", ObjTypeFunda::eBool)),
    Error::eRedefinition, spec);

  spec = "Example: types differ: first function type, then fundamental type";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstSeq(
      pe.mkFunDef("foo", ObjTypeFunda::eInt, new AstNumber(42)),
      new AstDataDef("foo", ObjTypeFunda::eInt)),
    Error::eRedefinition, spec);

  spec = "Example: types differ: First fundamental type, then function type";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstSeq(
      new AstDataDef("foo", ObjTypeFunda::eInt),
      pe.mkFunDef("foo", ObjTypeFunda::eInt, new AstNumber(42))),
    Error::eRedefinition, spec);

  spec = "Example: storage duration differs";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstSeq(
      new AstDataDef("x", make_shared<ObjTypeFunda>(ObjTypeFunda::eInt), StorageDuration::eLocal),
      new AstDataDef("x", make_shared<ObjTypeFunda>(ObjTypeFunda::eInt), StorageDuration::eStatic)),
    Error::eRedefinition, "");

  spec = "Example: two local variables in implicit main method";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstSeq(
      new AstDataDef("x", ObjTypeFunda::eInt),
      new AstDataDef("x", ObjTypeFunda::eInt)),
    Error::eRedefinition, spec);

  spec = "Example: two parameters";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    pe.mkFunDef("foo",
      AstFunDef::createArgs(
        new AstDataDef("x", ObjTypeFunda::eInt),
        new AstDataDef("x", ObjTypeFunda::eInt)),
      make_shared<ObjTypeFunda>(ObjTypeFunda::eInt),
      new AstNumber(42)),
    Error::eRedefinition, spec);

  spec = "Example: a parameter and a local variable in the function's body";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    pe.mkFunDef( "foo",
      AstFunDef::createArgs(
        new AstDataDef("x", ObjTypeFunda::eInt)),
      make_shared<ObjTypeFunda>(ObjTypeFunda::eInt),
      new AstDataDef("x", ObjTypeFunda::eInt)),
    Error::eRedefinition, spec);

  spec = "Example: two functions";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstSeq(
      pe.mkFunDef("foo", ObjTypeFunda::eInt,
        new AstNumber(42)),
      pe.mkFunDef("foo", ObjTypeFunda::eInt,
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
    pe.mkFunDef("outer", ObjTypeFunda::eInt,
      new AstSeq(
        pe.mkFunDef("inner", ObjTypeFunda::eInt,
          new AstNumber(42)),
        new AstNumber(42)));

  // exercise
  UUT.analyze(*ast);

  // verify
  EXPECT_TRUE( errorHandler.hasNoErrors() ) << amendAst(ast);

  ObjTypeFun funType{ ObjTypeFun::createArgs(), make_shared<ObjTypeFunda>(ObjTypeFunda::eInt)};

  shared_ptr<Entity> entityOuter;
  shared_ptr<Object> objectOuter;
  env.find("outer", entityOuter);
  EXPECT_TRUE( entityOuter.get() ) << amendAst(ast);
  EXPECT_NO_THROW( objectOuter = std::dynamic_pointer_cast<Object>(entityOuter));
  EXPECT_TRUE( objectOuter->objType().matchesFully(funType) );

  shared_ptr<Entity> entityInner;
  shared_ptr<Object> objectInner;
  env.find("inner", entityInner);
  EXPECT_TRUE( entityInner.get() ) << amendAst(ast);
  EXPECT_NO_THROW( objectInner = std::dynamic_pointer_cast<Object>(entityInner));
  EXPECT_TRUE( objectInner->objType().matchesFully(funType) );

  // tear down
  delete ast;
}

// aka temporary objects are immutable
TEST(SemanticAnalizerTest, MAKE_TEST_NAME2(
    GIVEN_a_block,
    THEN_its_objectType_is_that_of_the_body_however_with_an_immutable_qualifier))   {

  string spec = "Example: block ends with a temporary (thus immutable) data object";
  {
    // setup
    Env env;
    ErrorHandler errorHandler;
    TestingSemanticAnalizer UUT(env, errorHandler);
    ParserExt pe(UUT.m_env, UUT.m_errorHandler);
    unique_ptr<AstObject> ast{
      new AstBlock(
        new AstNumber(42, ObjTypeFunda::eInt))};

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_MATCHES_FULLY( ObjTypeFunda(ObjTypeFunda::eInt), ast->objType()) <<
      amendAst(ast) << amendSpec(spec);
  }

  spec = "Example: block ends with a local mutable data object";
  {
    // setup
    Env env;
    ErrorHandler errorHandler;
    TestingSemanticAnalizer UUT(env, errorHandler);
    ParserExt pe(UUT.m_env, UUT.m_errorHandler);
    unique_ptr<AstObject> ast{
      new AstBlock(
        new AstDataDef("x",
          make_shared<ObjTypeQuali>(ObjType::eMutable, make_shared<ObjTypeFunda>(ObjTypeFunda::eBool))))};

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_MATCHES_FULLY( ObjTypeFunda(ObjTypeFunda::eBool), ast->objType()) <<
      amendAst(ast) << amendSpec(spec);
  }
}

TEST(SemanticAnalizerTest, MAKE_TEST_NAME2(
    GIVEN_an_addrof_operator,
    THEN_the_access_value_of_the_AstNode_operand_is_eTakeAddr))
{
  // setup
  Env env;
  ErrorHandler errorHandler;
  TestingSemanticAnalizer UUT(env, errorHandler);
  AstObject* operand = new AstNumber(42);
  unique_ptr<AstObject> ast{new AstOperator('&', operand)};

  // exercise
  UUT.analyze(*ast.get());

  // verify
  EXPECT_EQ( eTakeAddress, operand->access()) << amendAst(ast);
}

TEST(SemanticAnalizerTest, MAKE_TEST_NAME2(
    GIVEN_an_AstObject_whichs_object_is_never_modified_nor_its_address_is_taken,
    THEN_the_target_object_remembers_that))
{
  string spec = "Example: a single AstNode, i.e. a single object";
  {
    // setup
    Env env;
    ErrorHandler errorHandler;
    TestingSemanticAnalizer UUT(env, errorHandler);
    unique_ptr<AstObject> ast{new AstNumber(42)};

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_FALSE( ast->objectIsModifiedOrRevealsAddr())
      << amendSpec(spec) << amendAst(ast);
  }
}

TEST(SemanticAnalizerTest, MAKE_TEST_NAME2(
    GIVEN_modifying_or_address_taking_AstNode,
    THEN_the_target_object_remembers_that))
{
  string spec = "Example: Address-of operator on a temporary object";
  {
    // setup
    Env env;
    ErrorHandler errorHandler;
    TestingSemanticAnalizer UUT(env, errorHandler);
    AstObject* operand = new AstNumber(42);
    unique_ptr<AstObject> ast{new AstOperator('&', operand)};

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_TRUE( operand->objectIsModifiedOrRevealsAddr())
      << amendSpec(spec) << amendAst(ast);
  }

  spec = "Example: Address-of operator on a named object, and the\n"
    "objectIsModifiedOrRevealsAddr-queried AST node refers to that named\n"
    "object";
  {
    // setup
    Env env;
    ErrorHandler errorHandler;
    errorHandler.disableReportingOf(Error::eComputedValueNotUsed);
    TestingSemanticAnalizer UUT(env, errorHandler);
    AstObject* symbol = new AstSymbol("x");
    unique_ptr<AstObject> ast{
      new AstSeq(
        new AstOperator('&',
          new AstDataDef("x", ObjTypeFunda::eInt)),
        symbol)};

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_TRUE( symbol->objectIsModifiedOrRevealsAddr())
      << amendSpec(spec) << amendAst(ast);
  }
}

TEST(SemanticAnalizerTest, MAKE_TEST_NAME2(
    GIVEN_an_access_to_a_bock,
    THEN_the_access_value_of_its_body_is_always_eRead))
{
  // setup
  Env env;
  ErrorHandler errorHandler;
  TestingSemanticAnalizer UUT(env, errorHandler);
  AstObject* body = new AstNumber(42);
  unique_ptr<AstObject> ast{
    new AstSeq( // imposes eIgnore access onto the block
      new AstBlock(body),
      new AstNumber(42))};

  // exercise
  UUT.analyze(*ast.get());

  // verify
  EXPECT_EQ( eRead, body->access()) << amendAst(ast);
}

TEST(SemanticAnalizerTest, MAKE_TEST_NAME2(
    GIVEN_an_access_to_a_sequence,
    THEN_the_access_value_of_seqs_last_operand_equals_that_outer_access_value_AND_the_access_value_of_the_leading_operands_is_eIgnore))
{
  // setup
  Env env;
  ErrorHandler errorHandler;
  TestingSemanticAnalizer UUT(env, errorHandler);
  AstObject* leadingOpAst = new AstNumber(42);
  AstObject* lastOpAst = new AstSymbol("x");
  unique_ptr<AstObject> ast{
    new AstSeq(
      new AstDataDef("x",
        make_shared<ObjTypeQuali>(ObjType::eMutable, make_shared<ObjTypeFunda>(ObjTypeFunda::eInt))),
      new AstOperator('=', // imposes write access onto the sequence
        new AstSeq( leadingOpAst, lastOpAst), // the sequence under test
        new AstNumber(77)))};

  // exercise
  UUT.analyze(*ast.get());

  // verify
  EXPECT_EQ( eIgnore, leadingOpAst->access()) << amendAst(ast);
  EXPECT_EQ( eWrite, lastOpAst->access()) << amendAst(ast);
}

TEST(SemanticAnalizerTest, MAKE_TEST_NAME2(
    GIVEN_a_sequence_WITH_the_last_operator_not_being_an_AstObject,
    THEN_it_reports_eObjectExpected))
{
  typedef AstCtList AstNotAnObject; // i.e. not derived from AstObject

  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstSeq(new AstNotAnObject()),
    Error::eObjectExpected, "");

  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstSeq(new AstNumber(42), new AstNotAnObject()),
    Error::eObjectExpected, "");
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
    unique_ptr<AstObject> ast{
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
    unique_ptr<AstObject> ast{
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
    AstObject* opAst =
      new AstOperator('+',
        new AstSymbol("x"),
        new AstSymbol("x"));
    unique_ptr<AstObject> ast{
      new AstSeq(
        new AstDataDef("x",
          make_shared<ObjTypeQuali>(ObjType::eMutable, make_shared<ObjTypeFunda>(ObjTypeFunda::eInt))),
        opAst)};

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_MATCHES_FULLY( ObjTypeFunda(ObjTypeFunda::eInt), opAst->objType()) <<
      amendAst(ast) << amendSpec(spec);
  }
}

// aka temporary objects are immutable
TEST(SemanticAnalizerTest, MAKE_TEST_NAME(
    an_sequence,
    transform,
    sets_the_objectType_of_the_seq_node_to_the_type_of_the_last_operand)) {

  // setup
  Env env;
  ErrorHandler errorHandler;
  TestingSemanticAnalizer UUT(env, errorHandler);
  ParserExt pe(UUT.m_env, UUT.m_errorHandler);
  unique_ptr<AstObject> ast{
    new AstSeq(
      new AstNumber(0, ObjTypeFunda::eInt),
      new AstNumber(0, ObjTypeFunda::eBool))};

  // exercise
  UUT.analyze(*ast.get());

  // verify
  EXPECT_MATCHES_FULLY( ObjTypeFunda(ObjTypeFunda::eBool), ast->objType()) <<
    amendAst(ast);
}

TEST(SemanticAnalizerTest, MAKE_TEST_NAME(
    an_addrOf_operator,
    transform,
    sets_the_objType_of_the_AstOperator_node_to_pointer_to_operands_type)) {
  // setup
  Env env;
  ErrorHandler errorHandler;
  TestingSemanticAnalizer UUT(env, errorHandler);
  ParserExt pe(UUT.m_env, UUT.m_errorHandler);
  unique_ptr<AstObject> ast{
    new AstOperator('&', new AstNumber(0, ObjTypeFunda::eInt))};

  // exercise
  UUT.analyze(*ast.get());

  // verify
  ObjTypePtr expectedObjType{make_shared<ObjTypeFunda>(ObjTypeFunda::eInt)};
  EXPECT_MATCHES_FULLY( expectedObjType, ast->objType()) << amendAst(ast);
}

TEST(SemanticAnalizerTest, MAKE_TEST_NAME(
    an_deref_operator,
    transform,
    sets_the_objectType_of_the_AstOperator_node_)) {
  // setup
  Env env;
  ErrorHandler errorHandler;
  TestingSemanticAnalizer UUT(env, errorHandler);
  ParserExt pe(UUT.m_env, UUT.m_errorHandler);
  auto ast = 
    new AstOperator(AstOperator::eDeref,
      new AstDataDef("x",
        make_shared<ObjTypePtr>(make_shared<ObjTypeFunda>(ObjTypeFunda::eInt))));

  // exercise
  UUT.analyze(*ast);

  // verify
  EXPECT_MATCHES_FULLY( ObjTypeFunda(ObjTypeFunda::eInt), ast->objType()) <<
    amendAst(ast);
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
  AstObject* assignmentAst =
    new AstOperator("=",
      new AstSymbol("foo"),
      new AstNumber(77));
  unique_ptr<AstObject> ast{
    new AstSeq(
      new AstDataDef("foo", make_shared<ObjTypeQuali>(ObjType::eMutable, make_shared<ObjTypeFunda>(ObjTypeFunda::eInt))),
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
  AstObject* dotAssignmentAst =
    new AstOperator(".=",
      new AstSymbol("x"),
      new AstNumber(0, ObjTypeFunda::eInt));
  unique_ptr<AstObject> ast{
    new AstSeq(
      new AstDataDef("x",
        make_shared<ObjTypeQuali>(ObjType::eMutable, make_shared<ObjTypeFunda>(ObjTypeFunda::eInt))),
      dotAssignmentAst)};

  // exercise
  UUT.analyze(*ast.get());

  // verify
  EXPECT_MATCHES_FULLY(
    ObjTypeQuali(ObjType::eMutable, make_shared<ObjTypeFunda>(ObjTypeFunda::eInt)),
    dotAssignmentAst->objType()) <<
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
  unique_ptr<AstObject> ast{
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
    unique_ptr<AstObject> ast{
      new AstSeq(
        pe.mkFunDef("foo", ObjTypeFunda::eBool,
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
    unique_ptr<AstObject> ast{
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
  AstObject* retAst = new AstReturn(new AstNumber(42, ObjTypeFunda::eInt));
  unique_ptr<AstObject> ast{
    pe.mkFunDef("foo", ObjTypeFunda::eInt,
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
      pe.mkFunDef("foo", ObjTypeFunda::eInt,
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
      pe.mkFunDef("foo", ObjTypeFunda::eInt,
        new AstSeq(
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
      pe.mkFunDef("outer", ObjTypeFunda::eInt,
        new AstSeq(

          pe.mkFunDef("inner", ObjTypeFunda::eBool,
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
    pe.mkFunDef("foo", ObjTypeFunda::eInt,
      new AstReturn(new AstNumber(42, ObjTypeFunda::eBool))),
    Error::eNoImplicitConversion, spec);

  spec = "Example: Early return, return expression in an if clause";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    pe.mkFunDef("foo", ObjTypeFunda::eInt,
      new AstSeq(
        new AstIf(
          new AstNumber(0, ObjTypeFunda::eBool),
          new AstReturn(new AstNumber(0, ObjTypeFunda::eBool))), // early return
        new AstNumber(77, ObjTypeFunda::eInt))),
    Error::eNoImplicitConversion, spec);

  spec = "Example: Nested function. Return expression in body of inner function "
    "does not match return type of its function, but would match that of the "
    "outer function. Naturally that is still an error.";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    pe.mkFunDef("outer", ObjTypeFunda::eInt,
      new AstSeq(

        pe.mkFunDef("inner", ObjTypeFunda::eBool,
          new AstReturn(new AstNumber(0, ObjTypeFunda::eInt))), // return under test

        new AstNumber(42))),
    Error::eNoImplicitConversion, spec);
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
    unique_ptr<AstObject> ast{
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
    unique_ptr<AstObject> ast{
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
    AstObject* if_ = new AstIf(
      new AstNumber(0, ObjTypeFunda::eBool),
      new AstSymbol("x"),
      new AstSymbol("x"));
    unique_ptr<AstObject> ast{
      new AstSeq(
        new AstDataDef("x",
          make_shared<ObjTypeQuali>(ObjType::eMutable, make_shared<ObjTypeFunda>(ObjTypeFunda::eBool))),
        if_)};

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_MATCHES_FULLY(
      ObjTypeQuali(ObjType::eMutable, make_shared<ObjTypeFunda>(ObjTypeFunda::eBool)),
      ast->objType()) <<
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
    AstObject* astIf =
      new AstIf(
        new AstNumber(0, ObjTypeFunda::eBool),
        new AstNumber(1, ObjTypeFunda::eBool),
        new AstReturn(new AstNumber(42, ObjTypeFunda::eInt)));
    unique_ptr<AstObject> ast{
      pe.mkFunDef("foo", ObjTypeFunda::eInt,
        new AstSeq(
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
    AstObject* astIf =
      new AstIf(
        new AstNumber(0, ObjTypeFunda::eBool),
        new AstReturn(new AstNumber(42, ObjTypeFunda::eInt)),
        new AstNumber(1, ObjTypeFunda::eBool));
    unique_ptr<AstObject> ast{
      pe.mkFunDef("foo", ObjTypeFunda::eInt,
        new AstSeq(
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
    AstObject* astIf =
      new AstIf(
        new AstNumber(0, ObjTypeFunda::eBool),
        new AstReturn(new AstNumber(42, ObjTypeFunda::eInt)),
        new AstReturn(new AstNumber(42, ObjTypeFunda::eInt)));
    unique_ptr<AstObject> ast{
      pe.mkFunDef("foo", ObjTypeFunda::eInt,
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
    unique_ptr<AstObject> ast{
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
    AstObject* astIf =
      new AstIf(
        new AstNumber(0, ObjTypeFunda::eBool),
        new AstReturn(new AstNumber(42, ObjTypeFunda::eInt)));
    unique_ptr<AstObject> ast{
      pe.mkFunDef("foo", ObjTypeFunda::eInt,
        new AstSeq(
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
    AstObject* thenClauseAst = new AstSymbol("x");
    AstObject* elseClauseAst = new AstSymbol("x");
    unique_ptr<AstObject> ast{
      new AstSeq( // imposes read access on its rhs
        new AstDataDef("x",
          make_shared<ObjTypeQuali>(ObjType::eMutable, make_shared<ObjTypeFunda>(ObjTypeFunda::eInt))),
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
    AstObject* thenClauseAst = new AstSymbol("x");
    AstObject* elseClauseAst = new AstSymbol("x");
    unique_ptr<AstObject> ast{
      new AstSeq(
        new AstDataDef("x",
          make_shared<ObjTypeQuali>(ObjType::eMutable, make_shared<ObjTypeFunda>(ObjTypeFunda::eInt))),
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
    unique_ptr<AstObject> ast{
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
    unique_ptr<AstObject> ast{
        pe.mkFunDef("foo", ObjTypeFunda::eInt,
          new AstSeq( loop, new AstNumber(42)))};

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
  AstObject* condition = new AstNumber(0, ObjTypeFunda::eBool);
  AstObject* body = new AstNumber(42);
  unique_ptr<AstObject> ast{ new AstLoop( condition, body)};

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
    new AstSeq(
      new AstDataDef("foo", ObjTypeFunda::eInt,
        new AstNumber(42)),
      new AstOperator('=',
        new AstSymbol("foo"),
        new AstNumber(77))),
    Error::eWriteToImmutable, spec);

  spec = "Example: Assignment directly to the definition of an "
    "immutable data object";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstOperator('=',
      new AstDataDef("foo", ObjTypeFunda::eInt,
        new AstNumber(42)),
      new AstNumber(77)),
    Error::eWriteToImmutable, spec);

  spec = "Example: Assignment to the temporary object resulting from a "
    "math operator";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstOperator('=',
      new AstOperator('+',
        new AstNumber(42),
        new AstNumber(77)),
      new AstNumber(88)),
    Error::eWriteToImmutable, spec);
}

TEST(SemanticAnalizerTest, MAKE_TEST_NAME(
    a_function_call_to_a_defined_function_WITH_incorrect_an_argument_count_or_incorrect_argument_type,
    transform,
    reports_an_eInvalidArguments)) {

  string spec = "Function foo expects one arg, but zero where passed on call";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstSeq(
      pe.mkFunDef("foo",
        AstFunDef::createArgs(
          new AstDataDef("x", ObjTypeFunda::eInt)),
        make_shared<ObjTypeFunda>(ObjTypeFunda::eInt),
        new AstNumber(42)),
      new AstFunCall(new AstSymbol("foo"))),
    Error::eInvalidArguments, spec);

  spec = "Function foo expects no args, but one arg was passed on call";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstSeq(
      pe.mkFunDef("foo", ObjTypeFunda::eInt,
        new AstNumber(42)),
      new AstFunCall(new AstSymbol("foo"), new AstCtList(new AstNumber(0)))),
    Error::eInvalidArguments, spec);

  spec = "Function foo expects one bool arg, but one int is passed";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstSeq(
      pe.mkFunDef("foo",
        AstFunDef::createArgs(
          new AstDataDef("x", ObjTypeFunda::eBool)),
        make_shared<ObjTypeFunda>(ObjTypeFunda::eInt),
        new AstNumber(42)),
      new AstFunCall(new AstSymbol("foo"), new AstCtList(new AstNumber(0, ObjTypeFunda::eInt)))),
    Error::eInvalidArguments, spec);

  spec = "Function foo expects one int arg, but one bool is passed";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstSeq(
      pe.mkFunDef("foo",
        AstFunDef::createArgs(
          new AstDataDef("x", ObjTypeFunda::eInt)),
        make_shared<ObjTypeFunda>(ObjTypeFunda::eInt),
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
    unique_ptr<AstObject> ast{
      new AstSeq(
        pe.mkFunDef("foo",
          AstFunDef::createArgs(
            new AstDataDef("x", ObjTypeFunda::eInt)),
          make_shared<ObjTypeFunda>(ObjTypeFunda::eInt),
          new AstNumber(42)),
        new AstDataDef("x", make_shared<ObjTypeQuali>(ObjType::eMutable, make_shared<ObjTypeFunda>(ObjTypeFunda::eInt))),
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
    unique_ptr<AstObject> ast{
      new AstSeq(
        pe.mkFunDef("foo",
          AstFunDef::createArgs(
            new AstDataDef("x", make_shared<ObjTypeQuali>(ObjType::eMutable, make_shared<ObjTypeFunda>(ObjTypeFunda::eInt)))),
          make_shared<ObjTypeFunda>(ObjTypeFunda::eInt),
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
    new AstSeq(
      pe.mkFunDef("foo", ObjTypeFunda::eInt, new AstNumber(42)),
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
    new AstSeq(
      pe.mkFunDef("foo", ObjTypeFunda::eInt, new AstNumber(42)),
      new AstOperator("==",
        new AstSymbol("foo"),
        new AstSymbol("foo"))),
    Error::eNoSuchMember, spec);

  spec = "Example: Dereference operator (*) with an non-pointer argument";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstOperator(AstOperator::eDeref, new AstNumber(0)),
    Error::eNoSuchMember, spec);
}

TEST(SemanticAnalizerTest, MAKE_TEST_NAME2(
    GIVEN_a_cast_aka_constructor,
    THEN_it_succeeds_if_newtype_has_respective_constructor_or_if_both_types_matchsaufqualifiers)) {

  struct T {
    ObjTypeFunda::EType newType;
    ObjTypeFunda::EType oldType;
    bool isValid;
  };
  vector<T> inputSpecs{
    {ObjTypeFunda::eVoid, ObjTypeFunda::eVoid, true},
    {ObjTypeFunda::eVoid, ObjTypeFunda::eNoreturn, false},
    {ObjTypeFunda::eVoid, ObjTypeFunda::eBool, false},
    {ObjTypeFunda::eVoid, ObjTypeFunda::eInt, false},
    {ObjTypeFunda::eVoid, ObjTypeFunda::eDouble, false},

    {ObjTypeFunda::eNoreturn, ObjTypeFunda::eVoid, false},
    {ObjTypeFunda::eNoreturn, ObjTypeFunda::eNoreturn, true},
    {ObjTypeFunda::eNoreturn, ObjTypeFunda::eBool, false},
    {ObjTypeFunda::eNoreturn, ObjTypeFunda::eInt, false},
    {ObjTypeFunda::eNoreturn, ObjTypeFunda::eDouble, false},

    {ObjTypeFunda::eBool, ObjTypeFunda::eVoid, false},
    {ObjTypeFunda::eBool, ObjTypeFunda::eNoreturn, false},
    {ObjTypeFunda::eBool, ObjTypeFunda::eBool, true},
    {ObjTypeFunda::eBool, ObjTypeFunda::eInt, true},
    {ObjTypeFunda::eBool, ObjTypeFunda::eDouble, true},

    {ObjTypeFunda::eInt, ObjTypeFunda::eVoid, false},
    {ObjTypeFunda::eInt, ObjTypeFunda::eNoreturn, false},
    {ObjTypeFunda::eInt, ObjTypeFunda::eBool, true},
    {ObjTypeFunda::eInt, ObjTypeFunda::eInt, true},
    {ObjTypeFunda::eInt, ObjTypeFunda::eDouble, true},

    {ObjTypeFunda::eDouble, ObjTypeFunda::eVoid, false},
    {ObjTypeFunda::eDouble, ObjTypeFunda::eNoreturn, false},
    {ObjTypeFunda::eDouble, ObjTypeFunda::eBool, true},
    {ObjTypeFunda::eDouble, ObjTypeFunda::eInt, true},
    {ObjTypeFunda::eDouble, ObjTypeFunda::eDouble, true}};

  for ( const auto& inputSpec : inputSpecs ) {

    // create child of AstCast
    AstObject* castChild = nullptr;
    switch (inputSpec.oldType) {
    case ObjTypeFunda::eVoid:
      castChild = new AstNop();
      break; 
    case ObjTypeFunda::eNoreturn:
      castChild = new AstReturn(new AstNumber(0));
      break; 
    case ObjTypeFunda::eBool: // fall through
    case ObjTypeFunda::eChar:  // fall through
    case ObjTypeFunda::eInt:  // fall through
    case ObjTypeFunda::eDouble:
      castChild = new AstNumber(0, inputSpec.oldType);
      break; 
    case ObjTypeFunda::ePointer:
      assert(false);
      break;
    }

    // AstCast needs to be wrapped into an AstFunDef since in some iterations
    // the child of AstCast is the return expression.
    AstObject* UUT = new AstCast( new ObjTypeFunda(inputSpec.newType), castChild);
    AstObject* funbody =
      inputSpec.newType == ObjTypeFunda::eNoreturn ?
      UUT :
      new AstSeq( UUT, new AstNumber(0));
    if ( inputSpec.isValid ) {
      TEST_ASTTRAVERSAL_SUCCEEDS_WITHOUT_ERRORS(
        pe.mkFunDef("foo", ObjTypeFunda::eInt, funbody),
        "");
    } else {
      TEST_ASTTRAVERSAL_REPORTS_ERROR(
        pe.mkFunDef("foo", ObjTypeFunda::eInt, funbody),
        Error::eNoSuchMember, "");
    }
  }
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
    pe.mkFunDef("foo", ObjTypeFunda::eInt,
      new AstSeq(
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
    new AstSeq(
      new AstOperator('+',
        new AstNumber(42),
        new AstNumber(77)),
      new AstNumber(11)),
    Error::eComputedValueNotUsed, spec)
}

TEST(SemanticAnalizerTest, MAKE_TEST_NAME3(
    GIVEN_a_return_type_with_mutable_qualifier,
    THEN_eRetTypeCantHaveMutQualifier_is_reported,
    BECAUSE_currently_temporary_objects_are_always_immutable)) {
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    pe.mkFunDef("foo",
      make_shared<ObjTypeQuali>(ObjType::eMutable, make_shared<ObjTypeFunda>(ObjTypeFunda::eInt)),
      new AstNumber(42)),
    Error::eRetTypeCantHaveMutQualifier, "");

}
