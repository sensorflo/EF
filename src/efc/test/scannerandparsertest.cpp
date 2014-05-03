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
    returns_an_AST_with_one_number_node) ) {
  // setup
  AstNode* astRoot;
  DriverOnTmpFile driver( "42" );

  // exercise
  driver.d().parse(astRoot);

  // verify
  ASSERT_TRUE( NULL != astRoot );
  EXPECT_EQ( "42", astRoot->toStr() );

  // tear down
  if (astRoot) { delete astRoot; }
}
