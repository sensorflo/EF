#include "test.h"
#include "../env.h"
#include <string>
using namespace testing;
using namespace std;

SymbolTableEntry* createASymbolTableEntry() {
  return new SymbolTableEntry(NULL, new ObjType);
}

TEST(EnvTest, MAKE_TEST_NAME2(
    insert_WITH_name_x,
    returns_a_pair_with_the_first_being_an_iterator_to_the_inserted_pair_and_the_second_being_true)) {

  SymbolTable::KeyValue xPair = make_pair("x", createASymbolTableEntry());
  Env UUT;
  Env::InsertRet ret = UUT.insert(xPair.first, xPair.second);
  EXPECT_EQ( xPair, *ret.first );
  EXPECT_TRUE( ret.second );
}

TEST(EnvTest, MAKE_TEST_NAME(
    an_Env_with_name_x_already_inserted,
    insert_WITH_name_x,
    returns_a_pair_with_the_first_being_an_iterator_to_the_allready_inserted_pair_and_the_second_being_false)) {
  // setup
  SymbolTable::KeyValue allreadyInsertedPair = make_pair("x", createASymbolTableEntry());
  Env UUT;
  UUT.insert(allreadyInsertedPair.first, allreadyInsertedPair.second);
  SymbolTableEntry* anotherStEntry = createASymbolTableEntry();

  // exercise
  Env::InsertRet ret = UUT.insert("x", anotherStEntry);

  // verify
  EXPECT_EQ( allreadyInsertedPair, *ret.first );
  EXPECT_FALSE( ret.second );

  // teardown
  delete anotherStEntry;
}

TEST(EnvTest, MAKE_TEST_NAME(
    a_name_and_a_symbol_table_entry_pair,
    insert_is_called_WITH_the_pair_as_argument_AND_then_find_is_called_with_the_name_as_argument,
    returns_a_pointer_to_the_symbol_table_entry)) {
  // setup
  string name = "x";
  SymbolTableEntry* stEntry = createASymbolTableEntry();
  Env UUT;

  // exercise
  UUT.insert(name, stEntry);
  SymbolTableEntry* returnedStEntry = UUT.find(name);

  // verify
  EXPECT_TRUE( NULL!=returnedStEntry );
  EXPECT_EQ( stEntry, returnedStEntry );
}

TEST(EnvTest, MAKE_TEST_NAME2(
    find_WITH_an_nonexisting_name,
    returns_NULL)) {
  Env UUT;
  EXPECT_TRUE( NULL==UUT.find("x") );
}

TEST(EnvTest, MAKE_TEST_NAME4(
    an_Env_with_name_x_already_inserted,
    pushScope_is_called_AND_then_insert_is_called_with_name_x,
    insert_behaves_as_when_name_x_was_not_inserted_before,
    because_the_new_scope_allows_the_inner_x_to_shadow_the_outer_x)) {
  // setup
  Env UUT;
  UUT.insert("x", createASymbolTableEntry());

  // exercise
  UUT.pushScope();
  SymbolTable::KeyValue innerx = make_pair("x", createASymbolTableEntry());
  Env::InsertRet ret = UUT.insert(innerx);

  // verify
  EXPECT_EQ( innerx, *ret.first );
  EXPECT_TRUE( ret.second );
}

TEST(EnvTest, MAKE_TEST_NAME4(
    an_Env_with_name_x_already_inserted,
    pushScope_is_called_AND_then_find_is_called_with_name_x,
    find_behaves_as_when_pushScope_had_not_been_called,
    because_the_new_scope_did_not_add_a_new_x_which_would_shadow_the_old_x)) {
  // setup
  string name = "x";
  SymbolTableEntry* stEntry = createASymbolTableEntry();
  Env UUT;
  UUT.insert(name, stEntry);

  // exercise
  UUT.pushScope();
  SymbolTableEntry* returnedStEntry = UUT.find(name);

  // verify
  EXPECT_TRUE( NULL!=returnedStEntry );
  EXPECT_EQ( stEntry, returnedStEntry );
}

TEST(EnvTest, MAKE_TEST_NAME(
    an_inner_and_outer_scope_AND_both_contain_a_name_x,
    popScope_is_called_AND_then_find_is_called_WITH_name_x,
    find_behaves_as_if_the_part_from_pushScope_to_popScope_did_not_happen)) {

  // setup
  Env UUT;
  SymbolTableEntry* stEntryOuter = createASymbolTableEntry();
  UUT.insert("x", stEntryOuter);
  UUT.pushScope();
  UUT.insert("x", createASymbolTableEntry()); // inner symbol table entry

  // exercise
  UUT.popScope();
  SymbolTableEntry* returnedStEntry = UUT.find("x");

  // verify
  EXPECT_TRUE( NULL!=returnedStEntry );
  EXPECT_EQ( stEntryOuter, returnedStEntry );
}

