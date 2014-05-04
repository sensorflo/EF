#include "test.h"
#include "driverontmpfile.h"
#include "../ast.h"
#include "../gensrc/parser.hpp"
using namespace testing;
using namespace std;
using namespace yy;

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    literal_number,
    parse,
    returns_an_AST_with_one_seq_node_and_one_number_node_as_child) ) {
  // setup
  AstNode* astRoot;
  DriverOnTmpFile driver( "42" );

  // exercise
  driver.d().parse(astRoot);

  // verify
  ASSERT_TRUE( NULL != astRoot );
  EXPECT_EQ( "seq(42)", astRoot->toStr() );

  // tear down
  if (astRoot) { delete astRoot; }
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    literal_number_sequence,
    parse,
    returns_an_AST_with_one_sequence_node_and_the_numbers_as_childs) ) {
  // setup
  AstNode* astRoot;
  DriverOnTmpFile driver( "42, 64, 77 " );

  // exercise
  driver.d().parse(astRoot);

  // verify
  ASSERT_TRUE( NULL != astRoot );
  EXPECT_EQ( "seq(42,64,77)", astRoot->toStr() );

  // tear down
  if (astRoot) { delete astRoot; }
}
