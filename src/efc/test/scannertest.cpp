#include "../gensrc/parser.hpp"
#include "../driver.h"
#include "gtest/gtest.h"
using namespace testing;
using namespace std;
using namespace yy;

TEST(ScannerTest, foo) {
  // setup
  Driver driver;
  driver.m_fileName = "test/test.ef";
  driver.beginScan();

  // exercise & verify
  // const location loc;
  EXPECT_EQ(Parser::token::TOK_END_OF_FILE, yylex(driver).token() );

  // tear down
  driver.beginScan();
}
