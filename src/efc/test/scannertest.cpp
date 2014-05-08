#include "test.h"
#include "../gensrc/parser.hpp"
#include "driverontmpfile.h"
using namespace testing;
using namespace std;
using namespace yy;

TEST(ScannerTest, MAKE_TEST_NAME(
    an_empty_file,
    yylex,
    returns_TOK_END_OF_FILE_AND_succeeds) ) {
  DriverOnTmpFile driver( "" );
  EXPECT_EQ(Parser::token::TOK_END_OF_FILE, yylex(driver).token() );
  EXPECT_FALSE( driver.d().gotError() );
}

TEST(ScannerTest, MAKE_TEST_NAME(
    concatenated_keywords,
    yylex,
    returns_TOK_ID_AND_succeeds)) {
  DriverOnTmpFile driver( "ifelse" );
  EXPECT_EQ(Parser::token::TOK_ID, yylex(driver).token() );
  EXPECT_FALSE( driver.d().gotError() );
}

TEST(ScannerTest, MAKE_TEST_NAME(
    keywords_separated_by_blanks,
    yylex_is_called_repeatedly,
    returns_the_keyword_tokens_AND_succeeds)) {
  DriverOnTmpFile driver( "if else" );
  EXPECT_EQ(Parser::token::TOK_IF, yylex(driver).token() );
  EXPECT_EQ(Parser::token::TOK_ELSE, yylex(driver).token() );
  EXPECT_FALSE( driver.d().gotError() );
}

TEST(ScannerTest, number) {
  {
    DriverOnTmpFile driver( "42" );
    EXPECT_EQ(Parser::token::TOK_NUMBER, yylex(driver).token() );
    EXPECT_EQ(Parser::token::TOK_END_OF_FILE, yylex(driver).token() );
    EXPECT_FALSE( driver.d().gotError() );
  }

  {
    DriverOnTmpFile driver( "42if" );
    EXPECT_EQ(Parser::token::TOK_NUMBER, yylex(driver).token() );
    EXPECT_TRUE( driver.d().gotError() );
    EXPECT_EQ(Parser::token::TOK_END_OF_FILE, yylex(driver).token() );
  }

  {
    DriverOnTmpFile driver( "flo 42" );
    EXPECT_EQ(Parser::token::TOK_ID, yylex(driver).token() );
    EXPECT_EQ(Parser::token::TOK_NUMBER, yylex(driver).token() );
    EXPECT_EQ(Parser::token::TOK_END_OF_FILE, yylex(driver).token() );
    EXPECT_FALSE( driver.d().gotError() );
  }

  {
    DriverOnTmpFile driver( "42" );
    Parser::symbol_type st = yylex(driver);
    EXPECT_EQ(42, st.value.as<int>());
  }
}

TEST(ScannerTest, id) {
  {
    DriverOnTmpFile driver( "foo" );
    Parser::symbol_type st = yylex(driver);
    EXPECT_EQ(Parser::token::TOK_ID, st.token() );
    EXPECT_EQ("foo", st.value.as<string>());
    EXPECT_FALSE( driver.d().gotError() );
  }
}
