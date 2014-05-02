#include "../gensrc/parser.hpp"
#include "../driver.h"
#include "gtest/gtest.h"
using namespace testing;
using namespace std;
using namespace yy;



TEST(ScannerTest, empty) {
  // setup
  Driver driver;
  driver.m_fileName = "test/test_empty.ef";
  driver.beginScan();

  // exercise & verify
  EXPECT_EQ(Parser::token::TOK_END_OF_FILE, yylex(driver).token() );

  // tear down
  driver.beginScan();
}

TEST(ScannerTest, id) {
  // setup
  Driver driver;
  driver.m_fileName = "test/test_id.ef";
  driver.beginScan();

  // exercise & verify
  EXPECT_EQ(Parser::token::TOK_ID, yylex(driver).token() );
  EXPECT_EQ(Parser::token::TOK_END_OF_FILE, yylex(driver).token() );

  // tear down
  driver.beginScan();
}

TEST(ScannerTest, if_else) {
  // setup
  Driver driver;
  driver.m_fileName = "test/test_if_else.ef";
  driver.beginScan();

  // exercise & verify
  EXPECT_EQ(Parser::token::TOK_IF, yylex(driver).token() );
  EXPECT_EQ(Parser::token::TOK_ELSE, yylex(driver).token() );
  EXPECT_EQ(Parser::token::TOK_END_OF_FILE, yylex(driver).token() );

  // tear down
  driver.beginScan();
}
