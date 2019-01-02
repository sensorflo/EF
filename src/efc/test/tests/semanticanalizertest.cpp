#include "test.h"
#include "../semanticanalizer.h"
#include "../ast.h"
#include "../objtype.h"
#include "../env.h"
#include "../errorhandler.h"
#include "../astdefaultiterator.h"
#include "../genparserext.h"

#include <memory>
#include <string>
#include <sstream>

using namespace testing;
using namespace std;

/** \file
Since SemanticAnalizer and GenParserExt don't have clearly separated
responsibilities, see the comments of the two, this test test responsibilities
which can be implemented by either of the two. */

class SemanticAnalizerTest : public Test {
private:
  DisableLocationRequirement m_Dummy;
};

class TestingAstSymbol : public AstSymbol {
public:
  TestingAstSymbol(const std::string& name) : AstSymbol(name) {}
  using AstSymbol::m_referencedAstObj;
};

class TestingSemanticAnalizer : public SemanticAnalizer {
public:
  TestingSemanticAnalizer(Env& env, ErrorHandler& errorHandler)
    : SemanticAnalizer(env, errorHandler){};
  using SemanticAnalizer::m_errorHandler;
  using SemanticAnalizer::m_env;
};

// this method is mainly not inlined into TEST_ASTTRAVERSAL so we have a nicer
// stack trace with SCOPED_TRACE.
void verifyAstTraversal(TestingSemanticAnalizer& UUT, AstNode* ast,
  Error::No expectedErrorNo, const string& expectedMsgParam1,
  const string& expectedMsgParam2, const string& expectedMsgParam3,
  const string& spec, bool foreignCatches, string excptionWhat) {

  const auto details = amendSpec(spec) + amendAst(ast) +
    amend(UUT.m_errorHandler) + amend(UUT.m_env);

  SCOPED_TRACE("verifyErrorHandlerHasExpectedError called from here");
  verifyErrorHandlerHasExpectedError(UUT.m_errorHandler, details,
    foreignCatches, excptionWhat, expectedErrorNo, expectedMsgParam1,
    expectedMsgParam2, expectedMsgParam3);
}

#define TEST_ASTTRAVERSAL(ast, expectedErrorNo, expectedMsgParam1, expectedMsgParam2, expectedMsgParam3, spec) \
  ErrorHandler errorHandler;                                            \
  Env env; /* must live shorter than the AST */                         \
  GenParserExt pe(env, errorHandler);                                   \
  TestingSemanticAnalizer UUT(env, errorHandler);                       \
  bool foreignThrow = false;                                            \
  string excptionwhat;                                                  \
  AstNode* ast_ = nullptr;                                              \
  try {                                                                 \
    ast_ = (ast);                                                       \
    UUT.analyze(*ast_);                                                 \
  }                                                                     \
  catch (BuildError&)  { /* nop */  }                                   \
  catch (exception& e) { foreignThrow = true; excptionwhat = e.what(); } \
  catch (exception* e) { foreignThrow = true; if (e) { excptionwhat = e->what(); } } \
  catch (...)          { foreignThrow = true; }                         \
  SCOPED_TRACE("verifyAstTraversal called from here");                  \
  verifyAstTraversal(UUT, ast_, expectedErrorNo, expectedMsgParam1,     \
    expectedMsgParam2, expectedMsgParam3, spec, foreignThrow, excptionwhat);

/** Actually does not only test traversal of AST, but also creation of it
with ParserExt, see also file's comment. */
#define TEST_ASTTRAVERSAL_REPORTS_ERROR(ast, expectedErrorNo, spec)     \
  {                                                                     \
    TEST_ASTTRAVERSAL(ast, expectedErrorNo, "", "", "", spec)           \
  }

/** Analogous to TEST_ASTTRAVERSAL_REPORTS_ERROR. */
#define TEST_ASTTRAVERSAL_REPORTS_ERROR_1MSGPARAM(ast, expectedErrorNo, expectedMsgParam1, spec) \
  {                                                                     \
    TEST_ASTTRAVERSAL(ast, expectedErrorNo, expectedMsgParam1, "", "", spec) \
  }

/** Analogous to TEST_ASTTRAVERSAL_REPORTS_ERROR. */
#define TEST_ASTTRAVERSAL_REPORTS_ERROR_2MSGPARAM(ast, expectedErrorNo, expectedMsgParam1, expectedMsgParam2, spec) \
  {                                                                     \
    TEST_ASTTRAVERSAL(ast, expectedErrorNo, expectedMsgParam1, expectedMsgParam2, "", spec) \
  }

/** Analogous to TEST_ASTTRAVERSAL_REPORTS_ERROR */
#define TEST_ASTTRAVERSAL_SUCCEEDS_WITHOUT_ERRORS(ast, spec)            \
  {                                                                     \
    TEST_ASTTRAVERSAL(ast, Error::eNone, "", "", "", spec)              \
  }

TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME(
    an_AstObjType_node,
    transform,
    sets_the_correct_objType_on_the_node)) {

  string spec = "Example: int";
  {
    // setup
    ErrorHandler errorHandler;
    Env env;
    auto ast = make_unique<AstObjTypeSymbol>(ObjTypeFunda::eInt);
    Env::AutoLetLooseNodes dummy(env);
    TestingSemanticAnalizer UUT(env, errorHandler);

    // exercise
    UUT.analyze(*ast);

    // verify
    EXPECT_MATCHES_FULLY(ObjTypeFunda(ObjTypeFunda::eInt), ast->objType())
      << amendAst(ast.get());
  }

  spec = "Example: mut-double";
  {
    // setup
    ErrorHandler errorHandler;
    Env env;
    auto ast = make_unique<AstObjTypeQuali>(
      ObjType::eMutable, new AstObjTypeSymbol(ObjTypeFunda::eInt));
    Env::AutoLetLooseNodes dummy(env);
    TestingSemanticAnalizer UUT(env, errorHandler);

    // exercise
    UUT.analyze(*ast);

    // verify
    EXPECT_MATCHES_FULLY(
      ObjTypeQuali(ObjType::eMutable, make_shared<ObjTypeFunda>(ObjTypeFunda::eInt)),
      ast->objType())
      << amendAst(ast.get());
  }

  spec = "Example: pointer";
  {
    // setup
    ErrorHandler errorHandler;
    Env env;
    auto ast = make_unique<AstObjTypePtr>(new AstObjTypeSymbol(ObjTypeFunda::eInt));
    Env::AutoLetLooseNodes dummy(env);
    TestingSemanticAnalizer UUT(env, errorHandler);

    // exercise
    UUT.analyze(*ast);

    // verify
    EXPECT_MATCHES_FULLY(
      ObjTypePtr(make_shared<ObjTypeFunda>(ObjTypeFunda::eInt)),
      ast->objType())
      << amendAst(ast.get());
  }

  spec = "Example: class";
  {
    // setup
    ErrorHandler errorHandler;
    Env env;
    auto ast = make_unique<AstClassDef>(
      "foo",
      new AstDataDef("member1", new AstObjTypeSymbol(ObjTypeFunda::eInt),
        StorageDuration::eMember),
      new AstDataDef("member2", new AstObjTypeSymbol(ObjTypeFunda::eDouble),
        StorageDuration::eMember));
    Env::AutoLetLooseNodes dummy(env);
    TestingSemanticAnalizer UUT(env, errorHandler);

    // exercise
    UUT.analyze(*ast);

    // verify
    const auto& objType = ast->objType();
    EXPECT_EQ("foo", objType.name());
    EXPECT_EQ(2U, objType.members().size());
    EXPECT_MATCHES_FULLY(ObjTypeFunda(ObjTypeFunda::eInt), *objType.members().at(0));
    EXPECT_MATCHES_FULLY(ObjTypeFunda(ObjTypeFunda::eDouble), *objType.members().at(1));
  }
}

TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME4(
    an_node_whose_childs_need_an_implicit_type_conversion_AND_the_rhs_is_of_larger_type,
    transform,
    reports_eNoImplicitConversion,
    BECAUSE_there_are_no_narrowing_implicit_conversions)) {
  string spec = "Example: Data object definition";
  TEST_ASTTRAVERSAL_REPORTS_ERROR_2MSGPARAM(
    new AstDataDef("x",
      ObjTypeFunda::eBool,
      new AstNumber(42, ObjTypeFunda::eInt)),
    Error::eNoImplicitConversion, "int", "bool", spec);

  spec = "Example: Binary math operator";
  TEST_ASTTRAVERSAL_REPORTS_ERROR_2MSGPARAM(
    new AstOperator('+',
      new AstNumber(0, ObjTypeFunda::eBool),
      new AstNumber(77, ObjTypeFunda::eInt)),
    Error::eNoImplicitConversion, "int", "bool", spec);

  spec = "Example: If else clause (note that there is an excpetion if one type "
    "is noreturn, see other specifications)";
  TEST_ASTTRAVERSAL_REPORTS_ERROR_2MSGPARAM(
    new AstIf(new AstNumber(0, ObjTypeFunda::eBool),
      new AstNumber(0, ObjTypeFunda::eBool),
      new AstNumber(77, ObjTypeFunda::eInt)),
    Error::eNoImplicitConversion, "int", "bool", spec);

  spec = "Example: Body of a function definition must match function's return type";
  TEST_ASTTRAVERSAL_REPORTS_ERROR_2MSGPARAM(
    pe.mkFunDef("foo", ObjTypeFunda::eBool, new AstNumber(42)),
    Error::eNoImplicitConversion, "int", "bool", spec);
}

TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME4(
    an_node_whose_childs_need_an_implicit_type_conversion_AND_the_rhs_is_of_smaller_type,
    transform,
    reports_eNoImplicitConversion,
    BECAUSE_currently_there_are_no_implicit_widening_conversions)) {
  string spec = "Example: Data object definition";
  TEST_ASTTRAVERSAL_REPORTS_ERROR_2MSGPARAM(
    new AstDataDef("x", ObjTypeFunda::eInt,
      new AstNumber(0, ObjTypeFunda::eBool)),
    Error::eNoImplicitConversion, "bool", "int", spec);

  spec = "Example: Binary math operator";
  TEST_ASTTRAVERSAL_REPORTS_ERROR_2MSGPARAM(
    new AstOperator('+',
      new AstNumber(42, ObjTypeFunda::eInt),
      new AstNumber(1, ObjTypeFunda::eBool)),
    Error::eNoImplicitConversion, "bool", "int", spec);

  spec = "Example: If else clause (note that there is an excpetion if one type "
    "is noreturn, see other specifications)";
  TEST_ASTTRAVERSAL_REPORTS_ERROR_2MSGPARAM(
    new AstIf(new AstNumber(0, ObjTypeFunda::eBool),
      new AstNumber(77, ObjTypeFunda::eInt),
      new AstNumber(0, ObjTypeFunda::eBool)),
    Error::eNoImplicitConversion, "bool", "int", spec);

  spec = "Example: Body of a function definition must match function's return type";
  TEST_ASTTRAVERSAL_REPORTS_ERROR_2MSGPARAM(
    pe.mkFunDef("foo", ObjTypeFunda::eInt,
      new AstNumber(0, ObjTypeFunda::eBool)),
    Error::eNoImplicitConversion, "bool", "int", spec);
}

TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME2(
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

TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME4(
    an_flow_control_expresssion_WITH_a_condition_whose_type_is_not_bool,
    transform,
    reports_eNoImplicitConversion,
    BECAUSE_currently_there_are_no_implicit_widening_conversions)) {

  TEST_ASTTRAVERSAL_REPORTS_ERROR_2MSGPARAM(
    new AstIf(new AstNumber(0, ObjTypeFunda::eInt),
      new AstNumber(42),
      new AstNumber(77)),
    Error::eNoImplicitConversion, "int", "bool", "");

  TEST_ASTTRAVERSAL_REPORTS_ERROR_2MSGPARAM(
    new AstLoop(new AstNumber(0, ObjTypeFunda::eInt),
      new AstNop()),
    Error::eNoImplicitConversion, "int", "bool", "");
}

TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME(
    a_simple_data_obj_definition,
    transform,
    inserts_an_appropriate_symbol_table_entry_into_Env)) {

  string spec = "Example: local immutable int";
  {
    // setup
    ErrorHandler errorHandler;
    Env env;
    unique_ptr<AstNode> ast(new AstDataDef("x", ObjTypeFunda::eInt));
    Env::AutoLetLooseNodes dummy(env);
    TestingSemanticAnalizer UUT(env, errorHandler);

    // exercise
    UUT.analyze(*ast);

    // verify
    const auto node = env.find("x");
    EXPECT_TRUE(nullptr!=node) << amendAst(ast.get());
    const auto& astObject = dynamic_cast<AstObject&>(*node);
    EXPECT_MATCHES_FULLY(ObjTypeFunda(ObjTypeFunda::eInt),
      astObject.object().objType()) << amendAst(ast.get());
  }

  spec = "Example: static immutable int";
  {
    // setup
    ErrorHandler errorHandler;
    Env env;
    unique_ptr<AstNode> ast(new AstDataDef("x",
        new AstObjTypeSymbol(ObjTypeFunda::eInt), StorageDuration::eStatic));
    Env::AutoLetLooseNodes dummy(env);
    TestingSemanticAnalizer UUT(env, errorHandler);

    // exercise
    UUT.analyze(*ast);

    // verify
    const auto node = env.find("x");
    EXPECT_TRUE(nullptr!=node) << amendAst(ast.get());
    const auto& astObject = dynamic_cast<AstObject&>(*node);
    EXPECT_MATCHES_FULLY(ObjTypeFunda(ObjTypeFunda::eInt),
      astObject.object().objType()) << amendAst(ast.get());
  }

  spec = "Example: noinit initializer";
  {
    // setup
    ErrorHandler errorHandler;
    Env env;
    unique_ptr<AstNode> ast(new AstDataDef("x", ObjTypeFunda::eInt,
        AstDataDef::noInit));
    Env::AutoLetLooseNodes dummy(env);
    TestingSemanticAnalizer UUT(env, errorHandler);

    // exercise
    UUT.analyze(*ast);

    // verify
    const auto node = env.find("x");
    EXPECT_TRUE(nullptr!=node) << amendAst(ast.get());
    const auto& astObject = dynamic_cast<AstObject&>(*node);
    EXPECT_MATCHES_FULLY(ObjTypeFunda(ObjTypeFunda::eInt),
      astObject.object().objType()) << amendAst(ast.get());
  }
}

TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME4(
    a_data_obj_definition_WITH_zero_ctor_arguments,
    transform,
    inserts_an_AstNumber_with_value_zero_as_single_ctor_argument,
    BECAUSE_this_is_the_current_hack_how_default_constructors_are_implemented)) {

  const auto ast = new AstDataDef("x", new AstObjTypeSymbol(ObjTypeFunda::eInt));
  TEST_ASTTRAVERSAL_SUCCEEDS_WITHOUT_ERRORS(ast, "");

  // further verifications
  const auto& ctorArgs = ast->ctorArgs().childs();
  ASSERT_EQ(1, ctorArgs.size()) << amendAst(ast);
  const auto arg = dynamic_cast<AstNumber*>(ast->ctorArgs().childs().front());
  EXPECT_TRUE(nullptr!=arg) << amendAst(ast);
  EXPECT_EQ(0, arg->value()) << amendAst(ast);
}

TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME(
    a_data_obj_initialization_WITH_incorrect_argument_count_for_the_constructor,
    transform,
    reports_a_eInvalidArguments)) {

  // note that if the type doesn't match, the reported error is eNoImplicitConversion

  string spec = "Example: data object definition";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstDataDef("x", new AstObjTypeSymbol(ObjTypeFunda::eInt), StorageDuration::eLocal,
      new AstCtList(new AstNumber(0), new AstNumber(0))),
    Error::eInvalidArguments, spec);

  spec = "Example: return value";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    pe.mkFunDef("foo", ObjTypeFunda::eInt,
      new AstReturn(new AstCtList(new AstNumber(0), new AstNumber(0)))),
    Error::eInvalidArguments, spec);
}

TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME(
    a_static_data_obj_definition_initialized_with_an_non_ctconst_expression,
    transform,
    reports_eCTConstRequired)) {
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstSeq(
      pe.mkFunDef("foo", ObjTypeFunda::eInt, new AstNumber(42)),
      new AstDataDef("x", new AstObjTypeSymbol(ObjTypeFunda::eInt),
        StorageDuration::eStatic,
        new AstFunCall(new AstSymbol("foo")))),
    Error::eCTConstRequired, "");
}

// The semantic analizer shall handle this case independently of whether the
// grammar already rules out this case or not, so the two are, well,
// independent.
TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME(
    a_parameter_defined_with_non_local_storage_duration,
    transform,
    reports_eInvalidStorageDurationInDef)) {
  TEST_ASTTRAVERSAL_REPORTS_ERROR_1MSGPARAM(
    pe.mkFunDef("foo",
      AstFunDef::createArgs(
        new AstDataDef("x",
          new AstObjTypeSymbol(ObjTypeFunda::eInt),
          StorageDuration::eStatic)),
      new AstObjTypeSymbol(ObjTypeFunda::eInt),
      new AstNumber(42)),
    Error::eInvalidStorageDurationInDef, "static", "");
}

TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME(
    a_simple_example_of_a_reference_to_an_unknown_name,
    transform,
    reports_an_eErrUnknownName)) {
  TEST_ASTTRAVERSAL_REPORTS_ERROR_1MSGPARAM(
    new AstSymbol("x"),
    Error::eUnknownName, "x", "");

  // separate test for symbol and funcal since currently the two do sadly not
  // share a common implementation
  TEST_ASTTRAVERSAL_REPORTS_ERROR_1MSGPARAM(
    new AstFunCall(new AstSymbol("foo")),
    Error::eUnknownName, "foo", "");
}

TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME(
    a_reference_to_an_entity_other_than_local_data_object_before_its_definition,
    transform,
    succeeds)) {

  string spec = "Example: function call before function definition";
  TEST_ASTTRAVERSAL_SUCCEEDS_WITHOUT_ERRORS(
    new AstSeq(
      new AstFunCall(new AstSymbol("foo")),
      pe.mkFunDef("foo", ObjTypeFunda::eVoid, new AstNop())),
    "");

  spec = "Example: static data";
  TEST_ASTTRAVERSAL_SUCCEEDS_WITHOUT_ERRORS(
    new AstSeq(
      new AstSymbol("x"),
      new AstDataDef("x",
        new AstObjTypeSymbol(ObjTypeFunda::eInt), StorageDuration::eStatic)),
    "");
}

// Note that the error is
// eNonIgnoreAccessToLocalDataObjectBeforeItsInitialization, as opposed to
// eUnknownName, because the name _is_ known and really refers to the local
// data object, it's just not allowed to access it yet, except for
// eIgnoreValueAndAddr access.
TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME3(
    a_non_ignore_access_to_a_local_data_object_before_its_initialization,
    transform,
    reports_eNonIgnoreAccessToLocalDataObjectBeforeItsInitialization)) {

  for (const auto& access : {Access::eRead, Access::eWrite, Access::eTakeAddress}) {
    string spec = "Trivial example";
    TEST_ASTTRAVERSAL_REPORTS_ERROR_1MSGPARAM(
      new AstSeq(
        createAccessTo("x", access),
        new AstDataDef("x")),
      Error::eNonIgnoreAccessToLocalDataObjectBeforeItsInitialization,
      "x", spec);

    spec = "Within the initializer subtree the data object is not yet considered initialized";
    TEST_ASTTRAVERSAL_REPORTS_ERROR_1MSGPARAM(
      new AstDataDef("x", ObjTypeFunda::eInt, createAccessTo("x", access)),
      Error::eNonIgnoreAccessToLocalDataObjectBeforeItsInitialization,
      "x", spec);
  }
}

TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME3(
    a_ignore_access_to_a_local_data_object_before_its_initialization,
    transform,
    succeeds)) {
  string spec = "An example would be using typeof(x) before defining x further below "
    "in the same block. 'typeof' does not yet exist however. The example used here is "
    "'x ; val x$', which however is a bad example since that should trigger another "
    "warning, which however does not exist yet.";
  TEST_ASTTRAVERSAL_SUCCEEDS_WITHOUT_ERRORS(
    new AstSeq(
      new AstSymbol("x"),
      new AstDataDef("x")),
    "");

  // todo: once there is something like 'typeof(expr)', use that as example to
  // write a test for.
}

TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME4(
    an_ignore_access_to_a_local_data_object_before_its_initialization,
    transform,
    succeeds,
    BECAUSE_the_name_really_is_valid_the_data_object_just_is_not_initialized_yet)) {
  string spec = "Trivial example";
  TEST_ASTTRAVERSAL_SUCCEEDS_WITHOUT_ERRORS(
    new AstSeq(
      createAccessTo("x", Access::eIgnoreValueAndAddr),
      new AstDataDef("x")), spec);

  spec = "Within the initializer subtree the data object is not yet considered initialized";
  TEST_ASTTRAVERSAL_SUCCEEDS_WITHOUT_ERRORS(
    new AstDataDef("x", ObjTypeFunda::eInt,
      createAccessTo("x", Access::eIgnoreValueAndAddr)),
    spec);

  spec = "A later valid non-ignore access shall not have an influence";
  for (const auto& otherAccess :
    {Access::eRead, Access::eWrite, Access::eTakeAddress}) {
    TEST_ASTTRAVERSAL_SUCCEEDS_WITHOUT_ERRORS(
      new AstSeq(
        createAccessTo("x", Access::eIgnoreValueAndAddr),
        new AstDataDef("x",
          new AstObjTypeQuali(ObjType::eMutable,
            new AstObjTypeSymbol(ObjTypeFunda::eInt))),
        createAccessTo("x", otherAccess)),
      spec);
  }
}

TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME3(
    a_data_obj_definition_in_a_block,
    transform,
    a_later_symbol_of_that_name_references_the_definition)) {

  // setup
  ErrorHandler errorHandler;
  Env env;
  const auto dataDef =
    new AstDataDef("x", ObjTypeFunda::eInt);
  const auto symbol = new TestingAstSymbol("x");
  unique_ptr<AstObject> ast{
    new AstBlock(
      new AstSeq(dataDef, symbol))};
  Env::AutoLetLooseNodes dummy(env);
  TestingSemanticAnalizer UUT(env, errorHandler);

  // exercise
  UUT.analyze(*ast.get());

  // verify
  EXPECT_EQ(dataDef, symbol->m_referencedAstObj) << amendAst(ast);
}

TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME4(
    a_local_data_object_definition_named_x_in_a_block_AND_a_symbol_named_x_after_the_block,
    transform,
    reports_eUnknownName,
    BECAUSE_the_scope_of_local_data_objects_declarations_is_that_of_the_enclosing_block)) {
  TEST_ASTTRAVERSAL_REPORTS_ERROR_1MSGPARAM(
    new AstSeq(
      new AstBlock(
        new AstDataDef("x", ObjTypeFunda::eInt)),
      new AstSymbol("x")),
    Error::eUnknownName, "x", "");
}

TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME(
    a_nested_non_local_definition_with_same_name_as_another_outer_non_local_definition,
    genIr,
    succeeds)) {
  TEST_ASTTRAVERSAL_SUCCEEDS_WITHOUT_ERRORS(
    new AstSeq(
      pe.mkFunDef("foo", ObjTypeFunda::eInt, new AstNumber(42)),
      pe.mkFunDef("unrelated_name", ObjTypeFunda::eInt,
        new AstSeq(
          pe.mkFunDef("foo", ObjTypeFunda::eInt, new AstNumber(42)),
          new AstNumber(42))),
      new AstNumber(42)),
    "");
}

TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME(
    a_redefinition,
    transform,
    reports_eRedefinition)) {

  string spec = "Example: types differs: two different fundamental types";
  TEST_ASTTRAVERSAL_REPORTS_ERROR_1MSGPARAM(
    new AstSeq(
      new AstDataDef("x", ObjTypeFunda::eInt),
      new AstDataDef("x", ObjTypeFunda::eBool)),
    Error::eRedefinition, "x", spec);

  spec = "Example: types differ: first function type, then fundamental type";
  TEST_ASTTRAVERSAL_REPORTS_ERROR_1MSGPARAM(
    new AstSeq(
      pe.mkFunDef("foo", ObjTypeFunda::eInt, new AstNumber(42)),
      new AstDataDef("foo", ObjTypeFunda::eInt)),
    Error::eRedefinition, "foo", spec);

  spec = "Example: types differ: First fundamental type, then function type";
  TEST_ASTTRAVERSAL_REPORTS_ERROR_1MSGPARAM(
    new AstSeq(
      new AstDataDef("foo", ObjTypeFunda::eInt),
      pe.mkFunDef("foo", ObjTypeFunda::eInt, new AstNumber(42))),
    Error::eRedefinition, "foo", spec);

  spec = "Example: storage duration differs";
  TEST_ASTTRAVERSAL_REPORTS_ERROR_1MSGPARAM(
    new AstSeq(
      new AstDataDef("x", new AstObjTypeSymbol(ObjTypeFunda::eInt), StorageDuration::eLocal),
      new AstDataDef("x", new AstObjTypeSymbol(ObjTypeFunda::eInt), StorageDuration::eStatic)),
    Error::eRedefinition, "x", "");

  spec = "Example: two local variables in implicit main method";
  TEST_ASTTRAVERSAL_REPORTS_ERROR_1MSGPARAM(
    new AstSeq(
      new AstDataDef("x", ObjTypeFunda::eInt),
      new AstDataDef("x", ObjTypeFunda::eInt)),
    Error::eRedefinition, "x", spec);

  spec = "Example: two parameters";
  TEST_ASTTRAVERSAL_REPORTS_ERROR_1MSGPARAM(
    pe.mkFunDef("foo",
      AstFunDef::createArgs(
        new AstDataDef("x", ObjTypeFunda::eInt),
        new AstDataDef("x", ObjTypeFunda::eInt)),
      new AstObjTypeSymbol(ObjTypeFunda::eInt),
      new AstNumber(42)),
    Error::eRedefinition, "x", spec);

  spec = "Example: a parameter and a local variable in the function's body";
  TEST_ASTTRAVERSAL_REPORTS_ERROR_1MSGPARAM(
    pe.mkFunDef("foo",
      AstFunDef::createArgs(
        new AstDataDef("x", ObjTypeFunda::eInt)),
      new AstObjTypeSymbol(ObjTypeFunda::eInt),
      new AstDataDef("x", ObjTypeFunda::eInt)),
    Error::eRedefinition, "x", spec);

  spec = "Example: two functions";
  TEST_ASTTRAVERSAL_REPORTS_ERROR_1MSGPARAM(
    new AstSeq(
      pe.mkFunDef("foo", ObjTypeFunda::eInt,
        new AstNumber(42)),
      pe.mkFunDef("foo", ObjTypeFunda::eInt,
        new AstNumber(42))),
    Error::eRedefinition, "foo", spec);
}

TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME(
    a_nested_function_definition,
    transform,
    succeeds_AND_inserts_both_appropriately_into_env)) {

  // setup
  ErrorHandler errorHandler;
  Env env;
  GenParserExt pe(env, errorHandler);
  const auto innerAstFunDef =
    pe.mkFunDef("inner", ObjTypeFunda::eInt,
      new AstNumber(42));
  unique_ptr<AstFunDef> outerAstFunDef(
    pe.mkFunDef("outer", ObjTypeFunda::eInt,
      new AstSeq(
        innerAstFunDef,
        new AstNumber(42))));
  Env::AutoLetLooseNodes dummy(env);
  TestingSemanticAnalizer UUT(env, errorHandler);

  // exercise
  UUT.analyze(*outerAstFunDef);

  // verify
  EXPECT_FALSE(errorHandler.hasErrors()) << amendAst(outerAstFunDef.get());

  const auto outerFoundNode = env.find("outer");
  EXPECT_EQ(outerAstFunDef.get(), outerFoundNode) << amendAst(outerAstFunDef.get());

  {
    Env::AutoScope scope(env, *outerAstFunDef, Env::AutoScope::descentScope);
    const auto innerFoundNode = env.find("inner");
    EXPECT_EQ(innerAstFunDef, innerFoundNode) << amendAst(outerAstFunDef.get());
  }
}

// aka temporary objects are immutable
TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME2(
    GIVEN_a_block,
    THEN_its_objectType_is_that_of_the_body_however_with_an_immutable_qualifier))   {

  string spec = "Example: block ends with a temporary (thus immutable) data object";
  {
    // setup
    ErrorHandler errorHandler;
    Env env;
    unique_ptr<AstObject> ast{
      new AstBlock(
        new AstNumber(42, ObjTypeFunda::eInt))};
    Env::AutoLetLooseNodes dummy(env);
    TestingSemanticAnalizer UUT(env, errorHandler);

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_MATCHES_FULLY(ObjTypeFunda(ObjTypeFunda::eInt), ast->object().objType()) <<
      amendAst(ast) << amendSpec(spec);
  }

  spec = "Example: block ends with a local mutable data object";
  {
    // setup
    ErrorHandler errorHandler;
    Env env;
    unique_ptr<AstObject> ast{
      new AstBlock(
        new AstDataDef("x",
          new AstObjTypeQuali(ObjType::eMutable, new AstObjTypeSymbol(ObjTypeFunda::eBool))))};
    Env::AutoLetLooseNodes dummy(env);
    TestingSemanticAnalizer UUT(env, errorHandler);

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_MATCHES_FULLY(ObjTypeFunda(ObjTypeFunda::eBool), ast->object().objType()) <<
      amendAst(ast) << amendSpec(spec);
  }
}

TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME2(
    GIVEN_an_addrof_operator,
    THEN_the_operands_accessFromAstParent_is_eTakeAddr))
{
  // setup
  ErrorHandler errorHandler;
  Env env;
  AstObject* operand = new AstNumber(42);
  unique_ptr<AstObject> ast{new AstOperator('&', operand)};
  Env::AutoLetLooseNodes dummy(env);
  TestingSemanticAnalizer UUT(env, errorHandler);

  // exercise
  UUT.analyze(*ast.get());

  // verify
  EXPECT_EQ(Access::eTakeAddress, operand->accessFromAstParent()) << amendAst(ast);
}

TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME2(
    GIVEN_an_AstObject_whichs_object_is_never_modified_nor_its_address_is_taken,
    THEN_the_target_object_remembers_that))
{
  string spec = "Example: a single AstNode, i.e. a single object";
  {
    // setup
    ErrorHandler errorHandler;
    Env env;
    unique_ptr<AstObject> ast{new AstNumber(42)};
    Env::AutoLetLooseNodes dummy(env);
    TestingSemanticAnalizer UUT(env, errorHandler);

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_FALSE(ast->object().isModifiedOrRevealsAddr())
      << amendSpec(spec) << amendAst(ast);
  }
}

TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME2(
    GIVEN_modifying_or_address_taking_AstNode,
    THEN_the_target_object_remembers_that))
{
  string spec = "Example: Address-of operator on a temporary object";
  {
    // setup
    ErrorHandler errorHandler;
    Env env;
    AstObject* operand = new AstNumber(42);
    unique_ptr<AstObject> ast{new AstOperator('&', operand)};
    Env::AutoLetLooseNodes dummy(env);
    TestingSemanticAnalizer UUT(env, errorHandler);

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_TRUE(operand->object().isModifiedOrRevealsAddr())
      << amendSpec(spec) << amendAst(ast);
  }

  spec = "Example: Address-of operator on a named object, and the\n"
    "objectIsModifiedOrRevealsAddr-queried AST node refers to that named\n"
    "object";
  {
    // setup
    ErrorHandler errorHandler;
    Env env;
    AstObject* symbol = new AstSymbol("x");
    unique_ptr<AstObject> ast{
      new AstSeq(
        new AstOperator('&',
          new AstDataDef("x", ObjTypeFunda::eInt)),
        symbol)};
    Env::AutoLetLooseNodes dummy(env);
    TestingSemanticAnalizer UUT(env, errorHandler);

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_TRUE(symbol->object().isModifiedOrRevealsAddr())
      << amendSpec(spec) << amendAst(ast);
  }
}

TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME2(
    GIVEN_any_access_to_a_bock,
    THEN_the_bodys_accessFromAstParent_is_still_always_eRead))
{
  // setup
  ErrorHandler errorHandler;
  Env env;
  AstObject* body = new AstNumber(42);
  unique_ptr<AstObject> ast{
    new AstSeq(// imposes eIgnoreValueAndAddr access onto the block
      new AstBlock(body),
      new AstNumber(42))};
  Env::AutoLetLooseNodes dummy(env);
  TestingSemanticAnalizer UUT(env, errorHandler);

  // exercise
  UUT.analyze(*ast.get());

  // verify
  EXPECT_EQ(Access::eRead, body->accessFromAstParent()) << amendAst(ast);
}

TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME2(
    GIVEN_an_access_to_a_sequence,
    THEN_accessFromAstParent_to_seqs_last_operand_equals_that_outer_access_value_AND_accessFromAstParent_to_the_leading_operands_is_eIgnoreValueAndAddr))
{
  // setup
  ErrorHandler errorHandler;
  Env env;
  AstObject* leadingOpAst = new AstNumber(42);
  AstObject* lastOpAst = new AstSymbol("x");
  unique_ptr<AstObject> ast{
    new AstSeq(
      new AstDataDef("x",
        new AstObjTypeQuali(ObjType::eMutable, new AstObjTypeSymbol(ObjTypeFunda::eInt))),
      new AstOperator('=', // imposes write access onto the sequence
        new AstSeq(leadingOpAst, lastOpAst), // the sequence under test
        new AstNumber(77)))};
  Env::AutoLetLooseNodes dummy(env);
  TestingSemanticAnalizer UUT(env, errorHandler);

  // exercise
  UUT.analyze(*ast.get());

  // verify
  EXPECT_EQ(Access::eIgnoreValueAndAddr,
    leadingOpAst->accessFromAstParent()) << amendAst(ast);
  EXPECT_EQ(Access::eWrite,
    lastOpAst->accessFromAstParent()) << amendAst(ast);
}

TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME2(
    GIVEN_a_sequence_WITH_the_last_operator_not_being_an_AstObject,
    THEN_it_reports_eObjectExpected))
{
  // an AstNode not derived from AstObject
  const auto makeANoneAstObjectNode = [](){
    return new AstObjTypeSymbol(ObjTypeFunda::eInt); };

  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstSeq(makeANoneAstObjectNode()),
    Error::eObjectExpected, "");

  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstSeq(new AstNumber(42), makeANoneAstObjectNode()),
    Error::eObjectExpected, "");
}

// aka temporary objects are immutable
TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME(
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
    ErrorHandler errorHandler;
    Env env;
    unique_ptr<AstObject> ast{
      new AstOperator('+',
        new AstNumber(0, ObjTypeFunda::eInt),
        new AstNumber(0, ObjTypeFunda::eInt))};
    Env::AutoLetLooseNodes dummy(env);
    TestingSemanticAnalizer UUT(env, errorHandler);

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_MATCHES_FULLY(ObjTypeFunda(ObjTypeFunda::eInt), ast->object().objType()) <<
      amendAst(ast) << amendSpec(spec);
  }

  spec = "Example: Logical operator (&&) with operands of type bool -> "
    "result temporary object must be of type bool";
  {
    // setup
    ErrorHandler errorHandler;
    Env env;
    unique_ptr<AstObject> ast{
      new AstOperator("&&",
        new AstNumber(0, ObjTypeFunda::eBool),
        new AstNumber(0, ObjTypeFunda::eBool))};
    TestingSemanticAnalizer UUT(env, errorHandler);

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_MATCHES_FULLY(ObjTypeFunda(ObjTypeFunda::eBool), ast->object().objType()) <<
      amendAst(ast) << amendSpec(spec);
  }

  spec = "Example: two muttable operands -> result temporary object is still immutable";
  {
    // setup
    ErrorHandler errorHandler;
    Env env;
    AstObject* opAst =
      new AstOperator('+',
        new AstSymbol("x"),
        new AstSymbol("x"));
    unique_ptr<AstObject> ast{
      new AstSeq(
        new AstDataDef("x",
          new AstObjTypeQuali(ObjType::eMutable, new AstObjTypeSymbol(ObjTypeFunda::eInt))),
        opAst)};
    Env::AutoLetLooseNodes dummy(env);
    TestingSemanticAnalizer UUT(env, errorHandler);

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_MATCHES_FULLY(ObjTypeFunda(ObjTypeFunda::eInt), opAst->object().objType()) <<
      amendAst(ast) << amendSpec(spec);
  }
}

// aka temporary objects are immutable
TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME(
    an_sequence,
    transform,
    sets_the_objectType_of_the_seq_node_to_the_type_of_the_last_operand)) {

  // setup
  ErrorHandler errorHandler;
  Env env;
  unique_ptr<AstObject> ast{
    new AstSeq(
      new AstNumber(0, ObjTypeFunda::eInt),
      new AstNumber(0, ObjTypeFunda::eBool))};
  Env::AutoLetLooseNodes dummy(env);
  TestingSemanticAnalizer UUT(env, errorHandler);

  // exercise
  UUT.analyze(*ast.get());

  // verify
  EXPECT_MATCHES_FULLY(ObjTypeFunda(ObjTypeFunda::eBool), ast->object().objType()) <<
    amendAst(ast);
}

TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME(
    an_addrOf_operator,
    transform,
    sets_the_objType_of_the_AstOperator_node_to_pointer_to_operands_type)) {
  // setup
  ErrorHandler errorHandler;
  Env env;
  unique_ptr<AstObject> ast{
    new AstOperator('&', new AstNumber(0, ObjTypeFunda::eInt))};
  Env::AutoLetLooseNodes dummy(env);
  TestingSemanticAnalizer UUT(env, errorHandler);

  // exercise
  UUT.analyze(*ast.get());

  // verify
  ObjTypePtr expectedObjType{make_shared<ObjTypeFunda>(ObjTypeFunda::eInt)};
  EXPECT_MATCHES_FULLY(expectedObjType, ast->object().objType()) << amendAst(ast);
}

TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME(
    an_deref_operator,
    transform,
    sets_the_objectType_of_the_AstOperator_node_)) {
  // setup
  ErrorHandler errorHandler;
  Env env;
  auto ast =
    new AstOperator(AstOperator::eDeref,
      new AstDataDef("x",
        new AstObjTypePtr(new AstObjTypeSymbol(ObjTypeFunda::eInt)),
        new AstCast(
          new AstObjTypePtr(new AstObjTypeSymbol(ObjTypeFunda::eInt)),
          new AstNumber(0, ObjTypeFunda::eNullptr))));
  Env::AutoLetLooseNodes dummy(env);
  TestingSemanticAnalizer UUT(env, errorHandler);

  // exercise
  UUT.analyze(*ast);

  // verify
  EXPECT_MATCHES_FULLY(ObjTypeFunda(ObjTypeFunda::eInt), ast->object().objType()) <<
    amendAst(ast);
}

// aka temporary objects are immutable
TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME(
    a_non_dot_assignment_operator,
    transform,
    sets_the_objectType_of_the_AstOperator_node_to_void)) {
  // setup
  ErrorHandler errorHandler;
  Env env;
  AstObject* assignmentAst =
    new AstOperator("=",
      new AstSymbol("foo"),
      new AstNumber(77));
  unique_ptr<AstObject> ast{
    new AstSeq(
      new AstDataDef("foo", new AstObjTypeQuali(ObjType::eMutable, new AstObjTypeSymbol(ObjTypeFunda::eInt))),
      assignmentAst)};
  Env::AutoLetLooseNodes dummy(env);
  TestingSemanticAnalizer UUT(env, errorHandler);

  // exercise
  UUT.analyze(*ast.get());

  // verify
  EXPECT_MATCHES_FULLY(ObjTypeFunda(ObjTypeFunda::eVoid), assignmentAst->object().objType()) <<
    amendAst(ast);
}

TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME(
    a_dot_assignment_operator,
    transform,
    sets_the_objectType_of_the_AstOperator_node_to_lhs_which_includes_being_mutable)) {

  // setup
  ErrorHandler errorHandler;
  Env env;
  AstObject* dotAssignmentAst =
    new AstOperator("=<",
      new AstSymbol("x"),
      new AstNumber(0, ObjTypeFunda::eInt));
  unique_ptr<AstObject> ast{
    new AstSeq(
      new AstDataDef("x",
        new AstObjTypeQuali(ObjType::eMutable, new AstObjTypeSymbol(ObjTypeFunda::eInt))),
      dotAssignmentAst)};
  Env::AutoLetLooseNodes dummy(env);
  TestingSemanticAnalizer UUT(env, errorHandler);

  // exercise
  UUT.analyze(*ast.get());

  // verify
  EXPECT_MATCHES_FULLY(
    ObjTypeQuali(ObjType::eMutable, make_shared<ObjTypeFunda>(ObjTypeFunda::eInt)),
    dotAssignmentAst->object().objType()) <<
    amendAst(ast);
}

// aka temporary objects are immutable
TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME(
    an_comparision_operator,
    transform,
    sets_the_objectType_of_the_AstOperator_node_to_immutable_bool)) {
  // setup
  ErrorHandler errorHandler;
  Env env;
  unique_ptr<AstObject> ast{
    new AstOperator("==",
      new AstNumber(42),
      new AstNumber(77))};
  Env::AutoLetLooseNodes dummy(env);
  TestingSemanticAnalizer UUT(env, errorHandler);

  // exercise
  UUT.analyze(*ast.get());

  // verify
  EXPECT_MATCHES_FULLY(ObjTypeFunda(ObjTypeFunda::eBool), ast->object().objType()) <<
    amendAst(ast);
}

// aka temporary objects are immutable
TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME(
    a_function_call,
    transform,
    sets_the_objectType_of_the_AstFunCall_node_to_the_return_type_of_the_called_function)) {

  string spec = "Example: function with no args returning bool";
  {
    // setup
    ErrorHandler errorHandler;
    Env env;
    GenParserExt pe(env, errorHandler);
    AstFunCall* funCall = new AstFunCall(new AstSymbol("foo"));
    unique_ptr<AstObject> ast{
      new AstSeq(
        pe.mkFunDef("foo", ObjTypeFunda::eBool,
          new AstNumber(0, ObjTypeFunda::eBool)),
        funCall)};
    Env::AutoLetLooseNodes dummy(env);
    TestingSemanticAnalizer UUT(env, errorHandler);

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_MATCHES_FULLY(ObjTypeFunda(ObjTypeFunda::eBool), funCall->objType()) <<
      amendAst(ast) << amendSpec(spec);
  }
}

TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME(
    a_cast,
    transform,
    sets_the_objectType_of_the_AstCast_node_to_the_type_of_the_cast)) {

  {
    // setup
    ErrorHandler errorHandler;
    Env env;
    GenParserExt pe(env, errorHandler);
    unique_ptr<AstObject> ast{
      new AstCast(
        new AstObjTypeSymbol(ObjTypeFunda::eInt),
        new AstNumber(0, ObjTypeFunda::eBool))};
    Env::AutoLetLooseNodes dummy(env);
    TestingSemanticAnalizer UUT(env, errorHandler);

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_MATCHES_FULLY(ObjTypeFunda(ObjTypeFunda::eInt), ast->object().objType()) <<
      amendAst(ast);
  }
}

TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME2(
    GIVEN_a_return_expression,
    THEN_the_objType_is_noreturn)) {

  // setup
  ErrorHandler errorHandler;
  Env env;
  GenParserExt pe(env, errorHandler);
  AstObject* retAst = new AstReturn(new AstNumber(42, ObjTypeFunda::eInt));
  unique_ptr<AstObject> ast{
    pe.mkFunDef("foo", ObjTypeFunda::eInt,
      retAst)};
  Env::AutoLetLooseNodes dummy(env);
  TestingSemanticAnalizer UUT(env, errorHandler);

  // exercise
  UUT.analyze(*ast.get());

  // verify
  EXPECT_MATCHES_FULLY(ObjTypeFunda(ObjTypeFunda::eNoreturn), retAst->object().objType()) <<
    amendAst(ast);
}

TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME2(
    GIVEN_a_return_expression_whose_retVal_ObjType_does_match_functions_return_ObjType,
    THEN_it_succeeds)) {

  string spec = "Example: return expression as last (and only) expression in fun's body";
  {
    // setup
    ErrorHandler errorHandler;
    Env env;
    GenParserExt pe(env, errorHandler);
    AstNode* ast =
      pe.mkFunDef("foo", ObjTypeFunda::eInt,
        new AstReturn(new AstNumber(42, ObjTypeFunda::eInt)));
    Env::AutoLetLooseNodes dummy(env);
    TestingSemanticAnalizer UUT(env, errorHandler);

    // exercise
    UUT.analyze(*ast);

    // verify
    EXPECT_FALSE(errorHandler.hasErrors()) << amendAst(ast) << amendSpec(spec);
  }

  spec = "Example: Early return, return expression in an if clause";
  {
    // setup
    ErrorHandler errorHandler;
    Env env;
    GenParserExt pe(env, errorHandler);
    AstNode* ast =
      pe.mkFunDef("foo", ObjTypeFunda::eInt,
        new AstSeq(
          new AstIf(
            new AstNumber(0, ObjTypeFunda::eBool),
            new AstReturn(new AstNumber(42, ObjTypeFunda::eInt))), // early return
          new AstNumber(77, ObjTypeFunda::eInt)));
    Env::AutoLetLooseNodes dummy(env);
    TestingSemanticAnalizer UUT(env, errorHandler);

    // exercise
    UUT.analyze(*ast);

    // verify
    EXPECT_FALSE(errorHandler.hasErrors()) << amendAst(ast);
  }

  spec = "Example: Nested functions with different return types.";
  {
    // setup
    ErrorHandler errorHandler;
    Env env;
    GenParserExt pe(env, errorHandler);
    AstNode* ast =
      pe.mkFunDef("outer", ObjTypeFunda::eInt,
        new AstSeq(

          pe.mkFunDef("inner", ObjTypeFunda::eBool,
            new AstReturn(new AstNumber(0, ObjTypeFunda::eBool))),

          new AstReturn(new AstNumber(42, ObjTypeFunda::eInt))));
    Env::AutoLetLooseNodes dummy(env);
    TestingSemanticAnalizer UUT(env, errorHandler);

    // exercise
    UUT.analyze(*ast);

    // verify
    EXPECT_FALSE(errorHandler.hasErrors()) << amendAst(ast);
  }
}

TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME2(
    GIVEN_a_return_expression_whose_retVal_ObjType_does_not_match_functions_return_ObjType,
    THEN_it_reports_eNoImplicitConversion)) {

  string spec = "Example: return expression as last (and only) expression in fun's body";
  TEST_ASTTRAVERSAL_REPORTS_ERROR_2MSGPARAM(
    pe.mkFunDef("foo", ObjTypeFunda::eInt,
      new AstReturn(new AstNumber(0, ObjTypeFunda::eBool))),
    Error::eNoImplicitConversion, "bool", "int", spec);

  spec = "Example: Early return, return expression in an if clause";
  TEST_ASTTRAVERSAL_REPORTS_ERROR_2MSGPARAM(
    pe.mkFunDef("foo", ObjTypeFunda::eInt,
      new AstSeq(
        new AstIf(
          new AstNumber(0, ObjTypeFunda::eBool),
          new AstReturn(new AstNumber(0, ObjTypeFunda::eBool))), // early return
        new AstNumber(77, ObjTypeFunda::eInt))),
    Error::eNoImplicitConversion, "bool", "int", spec);

  spec = "Example: Nested function. Return expression in body of inner function "
    "does not match return type of its function, but would match that of the "
    "outer function. Naturally that is still an error.";
  TEST_ASTTRAVERSAL_REPORTS_ERROR_2MSGPARAM(
    pe.mkFunDef("outer", ObjTypeFunda::eInt,
      new AstSeq(

        pe.mkFunDef("inner", ObjTypeFunda::eBool,
          new AstReturn(new AstNumber(0, ObjTypeFunda::eInt))), // return under test

        new AstNumber(42))),
    Error::eNoImplicitConversion, "int", "bool", spec);
}

// i.e. the if expression is just like an operator, which is just like an
// call: they return temporaries, i.e. new objects on the stack
TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME2(
    GIVEN_an_if_expression_with_else_clause_AND_none_of_the_two_clauses_is_of_obj_type_noreturn,
    THEN_the_if_expressions_obj_type_is_a_temporay)) {

  // this specification only looks at operands with identical types. Other
  // specifications specify how to introduce implicit conversions when the
  // type of the operands don't match

  string spec = "Example: both clauses of type bool -> whole if is of type bool";
  {
    // setup
    ErrorHandler errorHandler;
    Env env;
    unique_ptr<AstObject> ast{
      new AstIf(
        new AstNumber(0, ObjTypeFunda::eBool),
        new AstNumber(1, ObjTypeFunda::eBool),
        new AstNumber(0, ObjTypeFunda::eBool))};
    Env::AutoLetLooseNodes dummy(env);
    TestingSemanticAnalizer UUT(env, errorHandler);

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_MATCHES_FULLY(ObjTypeFunda(ObjTypeFunda::eBool), ast->object().objType()) <<
      amendAst(ast);
  }

  spec = "Example: both clauses of type void -> whole if is of type void";
  {
    // setup
    ErrorHandler errorHandler;
    Env env;
    unique_ptr<AstObject> ast{
      new AstIf(
        new AstNumber(0, ObjTypeFunda::eBool),
        new AstNop(),
        new AstNop())};
    Env::AutoLetLooseNodes dummy(env);
    TestingSemanticAnalizer UUT(env, errorHandler);

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_MATCHES_FULLY(ObjTypeFunda(ObjTypeFunda::eVoid), ast->object().objType()) <<
      amendAst(ast);
  }

  spec = "Example: both clauses are of type mutable-bool -> whole if is of type (immutable) bool";
  {
    // setup
    ErrorHandler errorHandler;
    Env env;
    AstObject* if_ = new AstIf(
      new AstNumber(0, ObjTypeFunda::eBool),
      new AstSymbol("x"),
      new AstSymbol("x"));
    unique_ptr<AstObject> ast{
      new AstSeq(
        new AstDataDef("x",
          new AstObjTypeQuali(ObjType::eMutable, new AstObjTypeSymbol(ObjTypeFunda::eBool))),
        if_)};
    Env::AutoLetLooseNodes dummy(env);
    TestingSemanticAnalizer UUT(env, errorHandler);

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_MATCHES_FULLY(
      ObjTypeFunda(ObjTypeFunda::eBool),
      if_->object().objType()) <<
      amendAst(ast);
  }
}

TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME2(
    GIVEN_an_if_expression_with_else_clause_AND_either_of_the_two_clauses_is_of_obj_type_noreturn,
    THEN_the_if_expressions_obj_type_is_exactly_that_of_the_other_clause)) {

  string spec = "Example: then clause of type bool, else clause of type noreturn -> "
    "the if expression's type is bool.";
  {
    // setup
    Env env;
    ErrorHandler errorHandler;
    GenParserExt pe(env, errorHandler);
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
    Env::AutoLetLooseNodes dummy(env);
    TestingSemanticAnalizer UUT(env, errorHandler);

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_MATCHES_FULLY(ObjTypeFunda(ObjTypeFunda::eBool), astIf->object().objType()) <<
      amendAst(ast);
  }

  spec = "Example: then clause of type noreturn, else clause of type bool -> "
    "the if expression's type is bool.";
  {
    // setup
    ErrorHandler errorHandler;
    Env env;
    GenParserExt pe(env, errorHandler);
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
    Env::AutoLetLooseNodes dummy(env);
    TestingSemanticAnalizer UUT(env, errorHandler);

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_MATCHES_FULLY(ObjTypeFunda(ObjTypeFunda::eBool), astIf->object().objType()) <<
      amendAst(ast);
  }

  spec = "Example: both clauses of type noreturn -> the if expression's type is noreturn.";
  {
    // setup
    ErrorHandler errorHandler;
    Env env;
    GenParserExt pe(env, errorHandler);
    AstObject* astIf =
      new AstIf(
        new AstNumber(0, ObjTypeFunda::eBool),
        new AstReturn(new AstNumber(42, ObjTypeFunda::eInt)),
        new AstReturn(new AstNumber(42, ObjTypeFunda::eInt)));
    unique_ptr<AstObject> ast{
      pe.mkFunDef("foo", ObjTypeFunda::eInt,
        astIf)};
    Env::AutoLetLooseNodes dummy(env);
    TestingSemanticAnalizer UUT(env, errorHandler);

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_MATCHES_FULLY(ObjTypeFunda(ObjTypeFunda::eNoreturn), astIf->object().objType()) <<
      amendAst(ast);
  }
}

TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME3(
    GIVEN_an_if_expression_with_no_else_clause,
    THEN_the_if_expressions_obj_type_is_void,
    BECAUSE_the_type_of_the_inexistent_else_clause_is_void)) {

  string spec = "Example: then clause is of type bool -> if expression is of type void.";
  {
    // setup
    ErrorHandler errorHandler;
    Env env;
    unique_ptr<AstObject> ast{
      new AstIf(
        new AstNumber(0, ObjTypeFunda::eBool),
        new AstNumber(1, ObjTypeFunda::eInt))};
    Env::AutoLetLooseNodes dummy(env);
    TestingSemanticAnalizer UUT(env, errorHandler);

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_MATCHES_FULLY(ObjTypeFunda(ObjTypeFunda::eVoid), ast->object().objType()) <<
      amendAst(ast);
  }

  spec = "Example: then clause is of type noreturn -> if expression is of type void.";
  {
    // setup
    ErrorHandler errorHandler;
    Env env;
    GenParserExt pe(env, errorHandler);
    AstObject* astIf =
      new AstIf(
        new AstNumber(0, ObjTypeFunda::eBool),
        new AstReturn(new AstNumber(42, ObjTypeFunda::eInt)));
    unique_ptr<AstObject> ast{
      pe.mkFunDef("foo", ObjTypeFunda::eInt,
        new AstSeq(
          astIf,
          new AstNumber(42)))};
    Env::AutoLetLooseNodes dummy(env);
    TestingSemanticAnalizer UUT(env, errorHandler);

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_MATCHES_FULLY(ObjTypeFunda(ObjTypeFunda::eVoid), astIf->object().objType()) <<
      amendAst(ast);
  }
}

// i.e. an if expression is just as an operator which in turn is just as an
// call
TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME2(
    GIVEN_any_access_to_an_if_expression,
    THEN_accessFromAstParent_to_both_clauses_is_still_always_eRead))
{
  string spec = "Example: eIgnoreValueAndAddr access";
  {
    // setup
    ErrorHandler errorHandler;
    Env env;
    AstObject* thenClauseAst = new AstNumber(42);
    AstObject* elseClauseAst = new AstNumber(42);
    unique_ptr<AstObject> ast{
      new AstSeq( // imposes eIgnoreValueAndAddr access upon the if expression
        new AstIf( // the if expression under test
          new AstNumber(0, ObjTypeFunda::eBool),
          thenClauseAst, elseClauseAst),
        new AstNumber(42))};
    Env::AutoLetLooseNodes dummy(env);
    TestingSemanticAnalizer UUT(env, errorHandler);

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_EQ(Access::eRead, thenClauseAst->accessFromAstParent()) << amendAst(ast);
    EXPECT_EQ(Access::eRead, thenClauseAst->accessFromAstParent()) << amendAst(ast);
  }
}


TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME3(
    GIVEN_an_loop_expression,
    THEN_the_loop_expressions_obj_type_is_void,
    BEAUSE_the_condition_might_prevent_the_block_from_ever_being_executed)) {

  string spec = "Example: obj type of body is int -> obj type of loop is still and always void";
  {
    // setup
    ErrorHandler errorHandler;
    Env env;
    unique_ptr<AstObject> ast{
      new AstLoop(
        new AstNumber(0, ObjTypeFunda::eBool),
        new AstNumber(42))};
    Env::AutoLetLooseNodes dummy(env);
    TestingSemanticAnalizer UUT(env, errorHandler);

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_MATCHES_FULLY(ObjTypeFunda(ObjTypeFunda::eVoid), ast->object().objType()) <<
      amendAst(ast);
  }

  spec = "Example: obj type of body is noret -> obj type of loop is still and always void";
  {
    // setup
    ErrorHandler errorHandler;
    Env env;
    GenParserExt pe(env, errorHandler);
    auto loop =
      new AstLoop(
        new AstNumber(0, ObjTypeFunda::eBool),
        new AstReturn(new AstNumber(42)));
    unique_ptr<AstObject> ast{
      pe.mkFunDef("foo", ObjTypeFunda::eInt,
        new AstSeq(loop, new AstNumber(42)))};
    Env::AutoLetLooseNodes dummy(env);
    TestingSemanticAnalizer UUT(env, errorHandler);

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_MATCHES_FULLY(ObjTypeFunda(ObjTypeFunda::eVoid), loop->objType()) <<
      amendAst(ast);
  }
}

TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME2(
    GIVEN_an_loop_expression,
    THEN_accessFromAstParent_to_the_condition_is_eRead_AND_accessFromAstParent_to_body_is_eIgnoreValueAndAddr))
{
  // setup
  ErrorHandler errorHandler;
  Env env;
  AstObject* condition = new AstNumber(0, ObjTypeFunda::eBool);
  AstObject* body = new AstNumber(42);
  unique_ptr<AstObject> ast{new AstLoop(condition, body)};
  Env::AutoLetLooseNodes dummy(env);
  TestingSemanticAnalizer UUT(env, errorHandler);

  // exercise
  UUT.analyze(*ast.get());

  // verify
  EXPECT_EQ(Access::eRead, condition->accessFromAstParent()) << amendAst(ast);
  EXPECT_EQ(Access::eIgnoreValueAndAddr, body->accessFromAstParent()) << amendAst(ast);
}

TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME(
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

TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME(
    a_function_call_to_a_defined_function_WITH_incorrect_argument_count,
    transform,
    reports_a_eInvalidArguments)) {

  string spec = "Function foo expects one arg, but zero where passed on call";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstSeq(
      pe.mkFunDef("foo",
        AstFunDef::createArgs(
          new AstDataDef("x", ObjTypeFunda::eInt)),
        new AstObjTypeSymbol(ObjTypeFunda::eInt),
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
}

TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME(
    a_function_call_to_a_defined_function_WITH_correct_argument_count_but_non_matching_types,
    transform,
    reports_eNoImplicitConversion)) {

  string spec = "Function foo expects one bool arg, but one int is passed";
  TEST_ASTTRAVERSAL_REPORTS_ERROR_2MSGPARAM(
    new AstSeq(
      pe.mkFunDef("foo",
        AstFunDef::createArgs(
          new AstDataDef("x", ObjTypeFunda::eBool)),
        new AstObjTypeSymbol(ObjTypeFunda::eInt),
        new AstNumber(42)),
      new AstFunCall(new AstSymbol("foo"), new AstCtList(new AstNumber(0, ObjTypeFunda::eInt)))),
    Error::eNoImplicitConversion,
    "int",
    "bool",
    spec);

  spec = "Function foo expects one int arg, but one bool is passed";
  TEST_ASTTRAVERSAL_REPORTS_ERROR_2MSGPARAM(
    new AstSeq(
      pe.mkFunDef("foo",
        AstFunDef::createArgs(
          new AstDataDef("x", ObjTypeFunda::eInt)),
        new AstObjTypeSymbol(ObjTypeFunda::eInt),
        new AstNumber(42)),
      new AstFunCall(new AstSymbol("foo"), new AstCtList(new AstNumber(0, ObjTypeFunda::eBool)))),
    Error::eNoImplicitConversion,
    "bool",
    "int",
    spec);
}

TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME(
    a_function_call_to_an_defined_function_WITH_correct_number_of_arguments_and_types,
    transform,
    succeeds)) {

  string spec = "Parameter is immutable, argument is mutable";
  {
    // setup
    ErrorHandler errorHandler;
    Env env;
    GenParserExt pe(env, errorHandler);
    unique_ptr<AstObject> ast{
      new AstSeq(
        pe.mkFunDef("foo",
          AstFunDef::createArgs(
            new AstDataDef("x", ObjTypeFunda::eInt)),
          new AstObjTypeSymbol(ObjTypeFunda::eInt),
          new AstNumber(42)),
        new AstDataDef("x", new AstObjTypeQuali(ObjType::eMutable, new AstObjTypeSymbol(ObjTypeFunda::eInt))),
        new AstFunCall(new AstSymbol("foo"),
          new AstCtList(new AstSymbol("x"))))};
    Env::AutoLetLooseNodes dummy(env);
    TestingSemanticAnalizer UUT(env, errorHandler);

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_FALSE(errorHandler.hasErrors()) << amendSpec(spec) << amendAst(ast);
  }

  spec = "Parameter is mutable, argument is immutable";
  {
    // setup
    ErrorHandler errorHandler;
    Env env;
    GenParserExt pe(env, errorHandler);
    unique_ptr<AstObject> ast{
      new AstSeq(
        pe.mkFunDef("foo",
          AstFunDef::createArgs(
            new AstDataDef("x", new AstObjTypeQuali(ObjType::eMutable, new AstObjTypeSymbol(ObjTypeFunda::eInt)))),
          new AstObjTypeSymbol(ObjTypeFunda::eInt),
          new AstNumber(42)),
        new AstFunCall(new AstSymbol("foo"),
          new AstCtList(new AstNumber(0, ObjTypeFunda::eInt))))};
    Env::AutoLetLooseNodes dummy(env);
    TestingSemanticAnalizer UUT(env, errorHandler);

    // exercise
    UUT.analyze(*ast.get());

    // verify
    EXPECT_FALSE(errorHandler.hasErrors()) << amendSpec(spec) << amendAst(ast);
  }
}

TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME(
    an_operator_WITH_an_argument_whose_obj_type_does_not_have_that_operator_as_member,
    transform,
    reports_eNoSuchMemberFun)) {
  // Note that after introducing implicit casts, the objec types of the
  // operands always match sauf qualifiers. That is specified by other
  // tests.

  // Also note that operators are semantically always really member method
  // calls, also for builtin types.

  string spec = "Example: Arithemtic operator (+) with argument not being of arithmetic class (here bool)";
  TEST_ASTTRAVERSAL_REPORTS_ERROR_2MSGPARAM(
    new AstOperator('+',
      new AstNumber(0, ObjTypeFunda::eBool),
      new AstNumber(0, ObjTypeFunda::eBool)),
    Error::eNoSuchMemberFun, "bool", "operator+", spec);

  spec = "Example: Arithemtic operator (+) with argument not being of arithmetic class (here function)";
  TEST_ASTTRAVERSAL_REPORTS_ERROR_2MSGPARAM(
    new AstSeq(
      pe.mkFunDef("foo", ObjTypeFunda::eInt, new AstNumber(42)),
      new AstOperator('+',
        new AstSymbol("foo"),
        new AstSymbol("foo"))),
    Error::eNoSuchMemberFun, "<dont_verify>", "operator+", spec);

  spec = "Example: Logic operator (&&) with argument not being bool (here int)";
  TEST_ASTTRAVERSAL_REPORTS_ERROR_2MSGPARAM(
    new AstOperator("&&",
      new AstNumber(0, ObjTypeFunda::eInt),
      new AstNumber(0, ObjTypeFunda::eInt)),
    Error::eNoSuchMemberFun, "int", "operator_and", spec);

  spec = "Example: Comparision operator (==) with argument not being of scalar class (here function)";
  TEST_ASTTRAVERSAL_REPORTS_ERROR_2MSGPARAM(
    new AstSeq(
      pe.mkFunDef("foo", ObjTypeFunda::eInt, new AstNumber(42)),
      new AstOperator("==",
        new AstSymbol("foo"),
        new AstSymbol("foo"))),
    Error::eNoSuchMemberFun, "<dont_verify>", "operator==", spec);

  spec = "Example: Dereference operator (*) with an non-pointer argument";
  TEST_ASTTRAVERSAL_REPORTS_ERROR_2MSGPARAM(
    new AstOperator(AstOperator::eDeref, new AstNumber(0)),
    Error::eNoSuchMemberFun, "int", "operator*", spec);
}

TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME2(
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
    {ObjTypeFunda::eVoid, ObjTypeFunda::eNullptr, false},

    {ObjTypeFunda::eNoreturn, ObjTypeFunda::eVoid, false},
    {ObjTypeFunda::eNoreturn, ObjTypeFunda::eNoreturn, true},
    {ObjTypeFunda::eNoreturn, ObjTypeFunda::eBool, false},
    {ObjTypeFunda::eNoreturn, ObjTypeFunda::eInt, false},
    {ObjTypeFunda::eNoreturn, ObjTypeFunda::eDouble, false},
    {ObjTypeFunda::eNoreturn, ObjTypeFunda::eNullptr, false},

    {ObjTypeFunda::eBool, ObjTypeFunda::eVoid, false},
    {ObjTypeFunda::eBool, ObjTypeFunda::eNoreturn, false},
    {ObjTypeFunda::eBool, ObjTypeFunda::eBool, true},
    {ObjTypeFunda::eBool, ObjTypeFunda::eInt, true},
    {ObjTypeFunda::eBool, ObjTypeFunda::eDouble, true},
    {ObjTypeFunda::eBool, ObjTypeFunda::eNullptr, false},

    {ObjTypeFunda::eInt, ObjTypeFunda::eVoid, false},
    {ObjTypeFunda::eInt, ObjTypeFunda::eNoreturn, false},
    {ObjTypeFunda::eInt, ObjTypeFunda::eBool, true},
    {ObjTypeFunda::eInt, ObjTypeFunda::eInt, true},
    {ObjTypeFunda::eInt, ObjTypeFunda::eNullptr, false},

    {ObjTypeFunda::eDouble, ObjTypeFunda::eVoid, false},
    {ObjTypeFunda::eDouble, ObjTypeFunda::eNoreturn, false},
    {ObjTypeFunda::eDouble, ObjTypeFunda::eBool, true},
    {ObjTypeFunda::eDouble, ObjTypeFunda::eInt, true},
    {ObjTypeFunda::eDouble, ObjTypeFunda::eNullptr, false},

    {ObjTypeFunda::eNullptr, ObjTypeFunda::eVoid, false},
    {ObjTypeFunda::eNullptr, ObjTypeFunda::eNoreturn, false},
    {ObjTypeFunda::eNullptr, ObjTypeFunda::eBool, false},
    {ObjTypeFunda::eNullptr, ObjTypeFunda::eInt, false},
    {ObjTypeFunda::eNullptr, ObjTypeFunda::eNullptr, true}};

  for (const auto& inputSpec : inputSpecs) {

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
    case ObjTypeFunda::eDouble:  // fall through
    case ObjTypeFunda::eNullptr:
      castChild = new AstNumber(0, inputSpec.oldType);
      break; 
    case ObjTypeFunda::ePointer:
      assert(false);
      break;
    case ObjTypeFunda::eInfer: // fall through
    case ObjTypeFunda::eTypeCnt: assert(false);
    }

    // AstCast needs to be wrapped into an AstFunDef since in some iterations
    // the child of AstCast is the return expression.
    const auto newAstObjTypeSymbol = new AstObjTypeSymbol(inputSpec.newType);
    AstObject* UUT = new AstCast(newAstObjTypeSymbol, castChild);
    AstObject* funbody =
      inputSpec.newType == ObjTypeFunda::eNoreturn ?
      UUT :
      new AstSeq(UUT, new AstNumber(0));
    if (inputSpec.isValid) {
      TEST_ASTTRAVERSAL_SUCCEEDS_WITHOUT_ERRORS(
        pe.mkFunDef("foo", ObjTypeFunda::eInt, funbody),
        "");
    } else {
      TEST_ASTTRAVERSAL_REPORTS_ERROR_2MSGPARAM(
        pe.mkFunDef("foo", ObjTypeFunda::eInt, funbody),
        Error::eNoSuchCtor,
        ObjTypeFunda(inputSpec.oldType).completeName(),
        ObjTypeFunda(inputSpec.newType).completeName(),
        "");
    }
  }
}

TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME3(
    a_cast_WITH_an_invalid_number_of_arguments,
    transform,
    reports_eInvalidArguments)) {

  string spec = "Zero instead exactly one argument";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstCast(
      new AstObjTypeSymbol(ObjTypeFunda::eInt),
      new AstCtList()),
    Error::eInvalidArguments, spec);

  spec = "More than one argument";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstCast(
      new AstObjTypeSymbol(ObjTypeFunda::eInt),
      new AstCtList(new AstNumber(42), new AstNumber(77))),
    Error::eInvalidArguments, spec);
}

TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME3(
    a_return_expression_outside_a_function_body,
    transform,
    reports_an_eNotInFunBodyContext)) {
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstReturn(),
    Error::eNotInFunBodyContext, "");
}

TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME4(
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

TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME3(
    GIVEN_a_return_type_with_mutable_qualifier,
    THEN_eRetTypeCantHaveMutQualifier_is_reported,
    BECAUSE_currently_temporary_objects_are_always_immutable)) {
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    pe.mkFunDef("foo",
      new AstObjTypeQuali(ObjType::eMutable, new AstObjTypeSymbol(ObjTypeFunda::eInt)),
      new AstNumber(42)),
    Error::eRetTypeCantHaveMutQualifier, "");

}

TEST_F(SemanticAnalizerTest, MAKE_TEST_NAME3(
    an_occurence_of_type_inference,
    transform,
    reports_a_eTypeInferenceIsNotYetSupported)) {
  string spec = "example: data def with type 'infer'";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstDataDef("x",
      new AstObjTypeSymbol(ObjTypeFunda::eInfer),
      StorageDuration::eLocal,
      new AstNumber(42)),
    Error::eTypeInferenceIsNotYetSupported, spec);

  spec = "example: fun def with type 'infer'";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    pe.mkFunDef("foo", ObjTypeFunda::eInfer, new AstNumber(42)),
    Error::eTypeInferenceIsNotYetSupported, spec);

  spec = "example: data def with type 'mut infer'";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstDataDef("x",
      new AstObjTypeQuali(ObjType::eMutable, new AstObjTypeSymbol(ObjTypeFunda::eInfer)),
      StorageDuration::eLocal,
      new AstNumber(42)),
    Error::eTypeInferenceIsNotYetSupported, spec);

  spec = "example: data def with type 'raw*infer'";
  TEST_ASTTRAVERSAL_REPORTS_ERROR(
    new AstDataDef("x",
      new AstObjTypePtr(new AstObjTypeSymbol(ObjTypeFunda::eInfer)),
      StorageDuration::eLocal,
      new AstNumber(42)),
    Error::eTypeInferenceIsNotYetSupported, spec);
}
