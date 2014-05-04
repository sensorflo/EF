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
    succeeds_AND_returns_an_AST_with_one_seq_node_and_one_number_node_as_child) ) {
  // setup
  DriverOnTmpFile driver( "42" );

  // exercise
  AstNode* astRoot = NULL;
  int res = driver.d().parse(astRoot);

  // verify
  EXPECT_EQ( 0, res);
  ASSERT_TRUE( NULL != astRoot );
  EXPECT_EQ( "seq(42)", astRoot->toStr() );

  // tear down
  if (astRoot) { delete astRoot; }
}

TEST(ScannerAndParserTest, MAKE_TEST_NAME(
    literal_number_sequence,
    parse,
    succeeds_AND_returns_an_AST_with_one_sequence_node_and_the_numbers_as_childs) ) {
  // setup
  DriverOnTmpFile driver( "42, 64, 77 " );

  // exercise
  AstNode* astRoot = NULL;
  int res = driver.d().parse(astRoot);

  // verify
  EXPECT_EQ( 0, res);
  ASSERT_TRUE( NULL != astRoot );
  EXPECT_EQ( "seq(42,64,77)", astRoot->toStr() );

  // tear down
  if (astRoot) { delete astRoot; }
}
