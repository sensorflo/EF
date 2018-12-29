#include "test.h"
#include "../env.h"
#include <string>
using namespace testing;
using namespace std;

TEST(EnvTest, MAKE_TEST_NAME3(
    an_Env_with_name_x_not_yet_inserted_in_current_scope,
    insert_WITH_name_x,
    returns_true_for_success)) {
  auto spec = "Current scope being root scope, using insertLeaf to insert";
  {
    // setup
    EnvNode node("x");
    Env UUT;  // env shall life shorter than it's nodes

    // exercise
    auto success = UUT.insertLeaf(node);

    // verify
    EXPECT_TRUE(success) << amendSpec(spec) << amend(UUT);
  }

  spec = "Current scope being root scope, using AutoScope to insert";
  {
    // setup
    EnvNode node("x");
    Env UUT;  // env shall life shorter than it's nodes

    // exercise
    bool success;
    Env::AutoScope dummy(
      UUT, node, Env::AutoScope::insertScopeAndDescent, success);

    // verify
    EXPECT_TRUE(success) << amendSpec(spec) << amend(UUT);
  }

  spec = "Current scope being _not_ root scope, using insertLeaf to insert";
  {
    // setup
    EnvNode otherNode("otherName");
    EnvNode node("x");
    Env UUT;  // env shall life shorter than it's nodes
    Env::AutoScope dummy(UUT, otherNode, Env::AutoScope::insertScopeAndDescent);

    // exercise
    auto success = UUT.insertLeaf(node);

    // verify
    EXPECT_TRUE(success) << amendSpec(spec) << amend(UUT);
  }

  spec = "Current scope being _not_ root scope, using AutoScope to insert";
  {
    // setup
    EnvNode otherNode("otherName");
    EnvNode node("x");
    Env UUT;  // env shall life shorter than it's nodes
    Env::AutoScope dummy1(
      UUT, otherNode, Env::AutoScope::insertScopeAndDescent);

    // exercise
    bool success;
    Env::AutoScope dummy2(
      UUT, node, Env::AutoScope::insertScopeAndDescent, success);

    // verify
    EXPECT_TRUE(success) << amendSpec(spec) << amend(UUT);
  }
}

TEST(EnvTest, MAKE_TEST_NAME(
    an_Env_with_name_x_already_inserted_in_current_scope,
    insertLeaf_WITH_name_x,
    returns_false)) {
  const auto name = "x"s;

  auto spec = "Current scope being root scope, using insertLeaf to insert";
  {
    // setup
    EnvNode meanNode(name);
    EnvNode node(name);
    Env UUT;  // env shall life shorter than it's nodes
    UUT.insertLeaf(meanNode);

    // exercise
    const auto success = UUT.insertLeaf(node);

    // verify
    EXPECT_FALSE(success) << amendSpec(spec) << amend(UUT);
  }

  spec = "Current scope being root scope, using AutoScope to insert";
  {
    // setup
    EnvNode meanNode(name);
    EnvNode node(name);
    Env UUT;  // env shall life shorter than it's nodes
    UUT.insertLeaf(meanNode);

    // exercise
    bool success;
    Env::AutoScope dummy(
      UUT, node, Env::AutoScope::insertScopeAndDescent, success);

    // verify
    EXPECT_FALSE(success) << amendSpec(spec) << amend(UUT);
  }

  spec = "Current scope being _not_ root scope, using insertLeaf to insert";
  {
    // setup
    EnvNode yetOtherNode("otherName");
    EnvNode meanNode(name);
    EnvNode node(name);
    Env UUT;  // env shall life shorter than it's nodes
    Env::AutoScope dummy(
      UUT, yetOtherNode, Env::AutoScope::insertScopeAndDescent);
    UUT.insertLeaf(meanNode);

    // exercise
    auto success = UUT.insertLeaf(node);

    // verify
    EXPECT_FALSE(success) << amendSpec(spec) << amend(UUT);
  }

  spec = "Current scope being _not_ root scope, using AutoScope to insert";
  {
    // setup
    EnvNode yetOtherNode("otherName");
    EnvNode meanNode(name);
    EnvNode node(name);
    Env UUT;  // env shall life shorter than it's nodes
    Env::AutoScope dummy1(
      UUT, yetOtherNode, Env::AutoScope::insertScopeAndDescent);
    UUT.insertLeaf(meanNode);

    // exercise
    bool success;
    Env::AutoScope dummy2(
      UUT, node, Env::AutoScope::insertScopeAndDescent, success);

    // verify
    EXPECT_FALSE(success) << amendSpec(spec) << amend(UUT);
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
    EnvNode insertedNode(name);
    Env UUT;  // env shall life shorter than it's nodes
    UUT.insertLeaf(insertedNode);

    // exercise
    const auto foundNode = UUT.find(name);

    // verify
    EXPECT_EQ(&insertedNode, foundNode) << amendSpec(spec) << amend(UUT);
  }

  spec = "Using AutoScope to insert";
  {
    // setup
    EnvNode insertedNode(name);
    Env UUT;  // env shall life shorter than it's nodes
    Env::AutoScope dummy(
      UUT, insertedNode, Env::AutoScope::insertScopeAndDescent);

    // exercise
    const auto foundNode = UUT.find(name);

    // verify
    EXPECT_EQ(&insertedNode, foundNode) << amendSpec(spec) << amend(UUT);
  }
}

TEST(EnvTest, MAKE_TEST_NAME2(
    find_WITH_an_nonexisting_name,
    returns_NULL)) {
  EnvNode insertedNode("x");
  Env UUT;  // env shall life shorter than it's nodes
  const auto foundNode = UUT.find("cantfindme");
  EXPECT_EQ(nullptr, foundNode) << amend(UUT);
}

TEST(EnvTest, MAKE_TEST_NAME4(
    an_Env_with_a_leaf_at_root_named_x,
    a_new_empty_scope_is_inserted_and_descendet_to_AND_then_insertLeaf_WITH_name_x_is_called,
    it_adds_a_new_leaf,
    BECAUSE_the_new_scope_allows_the_inner_x_to_shadow_the_outer_x)) {
  // setup
  const auto name = "x"s;
  EnvNode nodeOuter(name);
  EnvNode scopesNode("foo");
  EnvNode nodeInner(name);
  Env UUT;  // env shall life shorter than it's nodes
  UUT.insertLeaf(nodeOuter);

  // exercise
  Env::AutoScope dummy(UUT, scopesNode, Env::AutoScope::insertScopeAndDescent);
  const auto success = UUT.insertLeaf(nodeInner);

  // verify
  EXPECT_TRUE(success) << amend(UUT);
}

TEST(EnvTest, MAKE_TEST_NAME4(
    an_Env_with_an_name_x_at_root,
    a_new_scope_is_inserted_and_descended_to_AND_find_with_name_x_is_called,
    find_finds_the_x_at_the_root_scope,
    because_the_new_scope_did_not_add_a_new_x_which_would_shadow_the_x_at_root)) {
  // setup
  const auto name = "x"s;
  EnvNode scopesNode("foo");
  EnvNode insertedNode(name);
  Env UUT;  // env shall life shorter than it's nodes
  UUT.insertLeaf(insertedNode);

  // exercise
  Env::AutoScope scope(UUT, scopesNode, Env::AutoScope::insertScopeAndDescent);
  const auto foundNode = UUT.find(name);

  // verify
  EXPECT_EQ(&insertedNode, foundNode) << amend(UUT);
}

TEST(EnvTest, MAKE_TEST_NAME(
    an_outer_and_inner_scope_AND_both_contain_a_name_x,
    ascentScope_is_called_AND_then_find_is_called_WITH_name_x,
    find_behaves_as_if_the_part_from_pushScope_to_popScope_did_not_happen)) {
  // setup
  const auto name = "x"s;
  EnvNode scopesNode("foo");
  EnvNode outerNode(name);
  EnvNode innerNode(name);
  Env UUT;  // env shall life shorter than it's nodes
  UUT.insertLeaf(outerNode);

  // exercise
  {
    Env::AutoScope dummy(
      UUT, scopesNode, Env::AutoScope::insertScopeAndDescent);
    UUT.insertLeaf(innerNode);
  }
  const auto foundNode = UUT.find(name);

  // verify
  EXPECT_EQ(&outerNode, foundNode) << amend(UUT);
}

TEST(EnvTest, MAKE_TEST_NAME1(
    descentScope)) {
  EnvNode insertedNode("x");
  EnvNode scopesNode("foo");
  Env UUT;  // env shall life shorter than it's nodes
  {
    Env::AutoScope dummy(
      UUT, scopesNode, Env::AutoScope::insertScopeAndDescent);
    UUT.insertLeaf(insertedNode);
  }
  EnvNode* foundNode{};
  {
    Env::AutoScope dummy(UUT, scopesNode, Env::AutoScope::descentScope);
    foundNode = UUT.find("x");
  }
  EXPECT_EQ(&insertedNode, foundNode) << amend(UUT);
}

TEST(EnvTest, MAKE_TEST_NAME2(
    insertLeaf_or_AutoScope_constructor_is_called,
    it_returns_the_fully_qualified_name_of_the_just_inserted_name)) {
  string spec = "insertLeaf called at root";
  {
    // setup
    EnvNode node("foo");
    Env UUT;  // env shall life shorter than it's nodes

    // exercise
    UUT.insertLeaf(node);

    // verify
    EXPECT_EQ(".foo", node.fqName()) << amend(UUT) << amendSpec(spec);
  }

  spec = "AutoScope at root";
  {
    // setup
    EnvNode node("foo");
    Env UUT;  // env shall life shorter than it's nodes

    // exercise
    Env::AutoScope dummy(UUT, node, Env::AutoScope::insertScopeAndDescent);

    // verify
    EXPECT_EQ(".foo", node.fqName()) << amend(UUT) << amendSpec(spec);
  }

  spec = "insertLeaf called at a descendant of root";
  {
    // setup
    EnvNode scopesNode("othername");
    EnvNode node("foo");
    Env UUT;  // env shall life shorter than it's nodes
    Env::AutoScope dummy(
      UUT, scopesNode, Env::AutoScope::insertScopeAndDescent);

    // exercise
    UUT.insertLeaf(node);

    // verify
    EXPECT_EQ(".othername.foo", node.fqName()) << amend(UUT) << amendSpec(spec);
  }

  spec = "AutoScope at a descendant of root";
  {
    // setup
    EnvNode scopesNode("othername");
    EnvNode node("foo");
    Env UUT;  // env shall life shorter than it's nodes
    Env::AutoScope dummy1(
      UUT, scopesNode, Env::AutoScope::insertScopeAndDescent);

    // exercise
    Env::AutoScope dummy2(UUT, node, Env::AutoScope::insertScopeAndDescent);

    // verify
    EXPECT_EQ(".othername.foo", node.fqName()) << amend(UUT) << amendSpec(spec);
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
      "$root"
      "}"
      "}",
      ss.str())
      << amendSpec(spec);
  }

  spec = "Example with a few scopes and a few leafs";
  {
    // setup
    EnvNode node1("1");
    EnvNode node2("2_");
    EnvNode node21("2_1");
    EnvNode node22("2_2");
    EnvNode node3("3");
    EnvNode node4("4_");
    EnvNode node41("4_1");
    Env UUT;  // env shall life shorter than it's nodes

    UUT.insertLeaf(node1);
    {
      Env::AutoScope dummy(UUT, node2, Env::AutoScope::insertScopeAndDescent);
      UUT.insertLeaf(node21);
      UUT.insertLeaf(node22);
    }
    UUT.insertLeaf(node3);
    {
      Env::AutoScope dummy(UUT, node4, Env::AutoScope::insertScopeAndDescent);
      UUT.insertLeaf(node41);
    }

    stringstream ss;

    // exercise
    ss << UUT;

    // verify
    EXPECT_EQ(
      // clang-format off
      "{currentScope=$root, "
        "rootScope="
        "{$root, children={"
          "{1}, "
          "{2_, children={"
            "{2_1}, "
            "{2_2}"
          "}}, "
          "{3}, "
          "{4_, children={"
            "{4_1}"
          "}}"
        "}}"
      "}",
      // clang-format on
      ss.str())
      << amendSpec(spec);
  }
}
