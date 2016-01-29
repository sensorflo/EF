#include "test.h"
#include "../env.h"
#include <string>
using namespace testing;
using namespace std;

const FQNameProvider* dummyFQNameProvider{};

TEST(EnvTest, MAKE_TEST_NAME3(
    an_Env_with_name_x_not_yet_inserted_in_current_scope,
    insert_WITH_name_x,
    returns_non_null)) {

  auto spec = "Current scope being root scope, using insertLeaf to insert";
  {
    // setup
    Env UUT;
    // exercise
    shared_ptr<Entity>* entity = UUT.insertLeaf("x", dummyFQNameProvider);
    // verify
    EXPECT_TRUE(nullptr!=entity) << amendSpec(spec) << amend(UUT);
  }

  spec = "Current scope being root scope, using AutoScope to insert";
  {
    // setup
    Env UUT;
    // exercise
    shared_ptr<Entity>* entity;
    Env::AutoScope scope(UUT, "x", dummyFQNameProvider, entity,
      Env::AutoScope::insertScopeAndDescent);
    // verify
    EXPECT_TRUE(nullptr!=entity) << amendSpec(spec) << amend(UUT);
  }

  spec = "Current scope being _not_ root scope, using insertLeaf to insert";
  {
    // setup
    Env UUT;
    Env::AutoScope scope(UUT, "foo", Env::AutoScope::insertScopeAndDescent);
    // exercise
    shared_ptr<Entity>* entity = UUT.insertLeaf("x", dummyFQNameProvider);
    // verify
    EXPECT_TRUE(nullptr!=entity) << amendSpec(spec) << amend(UUT);
  }

  spec = "Current scope being _not_ root scope, using AutoScope to insert";
  {
    // setup
    Env UUT;
    Env::AutoScope scope1(UUT, "foo", Env::AutoScope::insertScopeAndDescent);
    // exercise
    shared_ptr<Entity>* entity;
    Env::AutoScope scope2(UUT, "x", dummyFQNameProvider, entity,
      Env::AutoScope::insertScopeAndDescent);
    // verify
    EXPECT_TRUE(nullptr!=entity) << amendSpec(spec) << amend(UUT);
  }
}

TEST(EnvTest, MAKE_TEST_NAME(
    an_Env_with_name_x_already_inserted_in_current_scope,
    insertLeaf_WITH_name_x,
    returns_null)) {
  const auto name = "x"s;

  auto spec = "Current scope being root scope, using insertLeaf to insert";
  {
    // setup
    Env UUT;
    UUT.insertLeaf(name, dummyFQNameProvider);
    // exercise
    shared_ptr<Entity>* entity = UUT.insertLeaf(name, dummyFQNameProvider);
    // verify
    EXPECT_EQ(nullptr, entity) << amendSpec(spec) << amend(UUT);
  }

  spec = "Current scope being root scope, using AutoScope to insert";
  {
    // setup
    Env UUT;
    UUT.insertLeaf(name, dummyFQNameProvider);
    // exercise
    shared_ptr<Entity>* entity;
    Env::AutoScope scope(UUT, name, dummyFQNameProvider, entity,
      Env::AutoScope::insertScopeAndDescent);
    // verify
    EXPECT_EQ(nullptr, entity) << amendSpec(spec) << amend(UUT);
  }

  spec = "Current scope being _not_ root scope, using insertLeaf to insert";
  {
    // setup
    Env UUT;
    Env::AutoScope scope(UUT, "foo", Env::AutoScope::insertScopeAndDescent);
    UUT.insertLeaf(name, dummyFQNameProvider);
    // exercise
    shared_ptr<Entity>* entity = UUT.insertLeaf(name, dummyFQNameProvider);
    // verify
    EXPECT_EQ(nullptr, entity) << amendSpec(spec) << amend(UUT);
  }

  spec = "Current scope being _not_ root scope, using AutoScope to insert";
  {
    // setup
    Env UUT;
    Env::AutoScope scope1(UUT, "foo", Env::AutoScope::insertScopeAndDescent);
    UUT.insertLeaf(name, dummyFQNameProvider);
    // exercise
    shared_ptr<Entity>* entity;
    Env::AutoScope scope2(UUT, name, dummyFQNameProvider, entity,
      Env::AutoScope::insertScopeAndDescent);
    // verify
    EXPECT_EQ(nullptr, entity) << amendSpec(spec) << amend(UUT);
  }
}

TEST(EnvTest, MAKE_TEST_NAME3(
    an_env_with_name_x_already_inserted,
    find_WITH_name_x,
    returns_the_same_ptr_that_insert_already_returned)) {
  const auto name = "x"s;
  auto spec = "Using insertLeaf to insert";
  {
    // setup
    Env UUT;
    shared_ptr<Entity>* insertedEntity = UUT.insertLeaf(name, dummyFQNameProvider);

    // exercise
    shared_ptr<Entity>* foundEntity = UUT.find(name);

    // verify
    EXPECT_EQ(insertedEntity, foundEntity) << amendSpec(spec) << amend(UUT);
  }

  spec = "Using AutoScope to insert";
  {
    // setup
    Env UUT;
    shared_ptr<Entity>* insertedEntity;
    Env::AutoScope scope(UUT, name, dummyFQNameProvider, insertedEntity,
      Env::AutoScope::insertScopeAndDescent);

    // exercise
    shared_ptr<Entity>* foundEntity = UUT.find(name);

    // verify
    EXPECT_EQ(insertedEntity, foundEntity) << amendSpec(spec) << amend(UUT);
  }
}

TEST(EnvTest, MAKE_TEST_NAME2(
    find_WITH_an_nonexisting_name,
    returns_NULL)) {
  Env UUT;
  shared_ptr<Entity>* foundEntity = UUT.find("x");
  EXPECT_EQ(nullptr, foundEntity) << amend(UUT);
}

TEST(EnvTest, MAKE_TEST_NAME4(
    an_Env_with_a_leaf_at_root_named_x,
    a_new_empty_scope_is_inserted_and_descendet_to_AND_then_insertLeaf_WITH_name_x_is_called,
    it_adds_a_new_leaf,
    BECAUSE_the_new_scope_allows_the_inner_x_to_shadow_the_outer_x)) {

  // setup
  Env UUT;
  const auto name = "x"s;
  const auto entityOuter = UUT.insertLeaf(name, dummyFQNameProvider);

  // exercise
  Env::AutoScope scope(UUT, "foo", Env::AutoScope::insertScopeAndDescent);
  const auto entityInner = UUT.insertLeaf(name, dummyFQNameProvider);

  // verify
  EXPECT_TRUE(nullptr!=entityInner) << amend(UUT);
  EXPECT_NE(entityOuter, entityInner) << amend(UUT);
}

TEST(EnvTest, MAKE_TEST_NAME4(
    an_Env_with_an_name_x_at_root,
    a_new_scope_is_inserted_and_descended_to_AND_find_with_name_x_is_called,
    find_finds_the_x_at_the_root_scope,
    because_the_new_scope_did_not_add_a_new_x_which_would_shadow_the_x_at_root)) {
  // setup
  Env UUT;
  const auto name = "x"s;
  const auto insertedEntity = UUT.insertLeaf(name, dummyFQNameProvider);

  // exercise
  Env::AutoScope scope(UUT, "foo", Env::AutoScope::insertScopeAndDescent);
  const auto foundEntiy = UUT.find(name);

  // verify
  EXPECT_EQ(insertedEntity, foundEntiy) << amend(UUT);
}

TEST(EnvTest, MAKE_TEST_NAME(
    an_outer_and_inner_scope_AND_both_contain_a_name_x,
    ascentScope_is_called_AND_then_find_is_called_WITH_name_x,
    find_behaves_as_if_the_part_from_pushScope_to_popScope_did_not_happen)) {

  // setup
  Env UUT;
  const auto name = "x"s;
  const auto outerEntity = UUT.insertLeaf(name, dummyFQNameProvider);
  shared_ptr<Entity>* innerEntity{};

  // exercise
  {
    Env::AutoScope scope(UUT, "foo", Env::AutoScope::insertScopeAndDescent);
    innerEntity = UUT.insertLeaf(name, dummyFQNameProvider);
  }
  const auto foundEntity = UUT.find(name);

  // verify
  EXPECT_EQ(foundEntity, outerEntity) << amend(UUT) <<
    "\ninnerEntity=" << (void*)innerEntity;
}

TEST(EnvTest, MAKE_TEST_NAME1(
    descentScope)) {
  Env UUT;
  shared_ptr<Entity>* insertedEntity{};
  shared_ptr<Entity>* foundEntity{};
  {
    Env::AutoScope scopeFoo(UUT, "foo", Env::AutoScope::insertScopeAndDescent);
    insertedEntity = UUT.insertLeaf("x", dummyFQNameProvider);
  }
  {
    Env::AutoScope scopeFoo(UUT, "foo", Env::AutoScope::descentScope);
    foundEntity = UUT.find("x");
  }
  EXPECT_EQ(insertedEntity, foundEntity) << amend(UUT);
}

TEST(EnvTest, MAKE_TEST_NAME2(
    insertLeaf_or_AutoScope_constructor_is_called,
    it_returns_the_fully_qualified_name_of_the_just_inserted_name)) {
  std::shared_ptr<Entity>* dummyEntity;

  string spec = "insertLeaf called at root";
  {
    // setup
    Env UUT;
    const FQNameProvider* fqNameProvider;

    // exercise
    UUT.insertLeaf("foo", fqNameProvider);

    // verify
    EXPECT_EQ(".foo", fqNameProvider->fqName()) << amend(UUT) << amendSpec(spec);
  }

  spec = "AutoScope at root";
  {
    // setup
    Env UUT;
    const FQNameProvider* fqNameProvider;

    // exercise
    Env::AutoScope scopeFoo(UUT, "foo", fqNameProvider, dummyEntity,
      Env::AutoScope::insertScopeAndDescent);

    // verify
    EXPECT_EQ(".foo", fqNameProvider->fqName()) << amend(UUT) << amendSpec(spec);
  }

  spec = "insertLeaf called at a descendant of root";
  {
    // setup
    Env UUT;
    const FQNameProvider* fqNameProvider;
    Env::AutoScope scopeFoo(UUT, "othername", Env::AutoScope::insertScopeAndDescent);

    // exercise
    UUT.insertLeaf("foo", fqNameProvider);

    // verify
    EXPECT_EQ(".othername.foo", fqNameProvider->fqName()) << amend(UUT) << amendSpec(spec);
  }

  spec = "AutoScope at a descendant of root";
  {
    // setup
    Env UUT;
    const FQNameProvider* fqNameProvider;
    Env::AutoScope scopeOther(UUT, "othername", Env::AutoScope::insertScopeAndDescent);

    // exercise
    Env::AutoScope scopeFoo(UUT, "foo", fqNameProvider, dummyEntity,
      Env::AutoScope::insertScopeAndDescent);

    // verify
    EXPECT_EQ(".othername.foo", fqNameProvider->fqName()) << amend(UUT) << amendSpec(spec);
  }
}

TEST(EnvTest, MAKE_TEST_NAME1(
    ostream_operator)) {

  auto spec = "Example: empty environment";
  {
    // setup
    Env UUT;
    stringstream ss;

    // exercise
    ss << UUT;

    // verify
    EXPECT_EQ(
      "{"
        "currentScope=$root, "
        "rootScope={"
          "name=$root, "
          "m_entity=null"
        "}"
      "}",
      ss.str()) << amendSpec(spec);
  }

  spec = "Example with a few scopes and a few leafs";
  {
    // setup
    Env UUT;
    UUT.insertLeaf("1", dummyFQNameProvider);
    {
      Env::AutoScope scope(UUT, "2_", Env::AutoScope::insertScopeAndDescent);
      UUT.insertLeaf("2_1", dummyFQNameProvider);
      UUT.insertLeaf("2_2", dummyFQNameProvider);
    }
    UUT.insertLeaf("3", dummyFQNameProvider);
    {
      Env::AutoScope scope(UUT, "4_", Env::AutoScope::insertScopeAndDescent);
      UUT.insertLeaf("4_1", dummyFQNameProvider);
    }

    stringstream ss;

    // exercise
    ss << UUT;

    // verify
    EXPECT_EQ(
      "{currentScope=$root, "
      "rootScope="
      "{name=$root, m_entity=null, children={"
      "{name=1, m_entity=null}, "
      "{name=2_, m_entity=null, children={"
      "{name=2_1, m_entity=null}, "
      "{name=2_2, m_entity=null}"
      "}}, "
      "{name=3, m_entity=null}, "
      "{name=4_, m_entity=null, children={"
      "{name=4_1, m_entity=null}"
      "}}"
      "}}"
      "}",
      ss.str());
  }
}
