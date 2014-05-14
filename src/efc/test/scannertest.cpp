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
    a_keyword,
    yylex,
    returns_token_of_that_keyword_notably_opposed_to_token_for_id_AND_succeeds)) {
  DriverOnTmpFile driver( "if" );
  EXPECT_EQ(Parser::token::TOK_IF, yylex(driver).token() );
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
    returns_the_keywords_tokens_AND_succeeds)) {
  DriverOnTmpFile driver( "if else" );
  EXPECT_EQ(Parser::token::TOK_IF, yylex(driver).token() );
  EXPECT_EQ(Parser::token::TOK_ELSE, yylex(driver).token() );
  EXPECT_FALSE( driver.d().gotError() );
}

TEST(ScannerTest, MAKE_TEST_NAME(
    a_literal_number,
    yylex,
    returns_TOK_NUMBER_AND_the_numbers_value_as_semantic_value_AND_succeeds)) {
  DriverOnTmpFile driver( "42" );

  Parser::symbol_type st = yylex(driver);
  EXPECT_EQ(Parser::token::TOK_NUMBER, st.token());
  EXPECT_EQ(42, st.value.as<int>());

  EXPECT_EQ(Parser::token::TOK_END_OF_FILE, yylex(driver).token() );

  EXPECT_FALSE( driver.d().gotError() );
}

TEST(ScannerTest, MAKE_TEST_NAME(
    a_literal_number_WITH_an_unknown_suffix,
    yylex,
    returns_TOK_NUMBER_AND_fails)) {
  DriverOnTmpFile driver( "42if" );
  EXPECT_EQ(Parser::token::TOK_NUMBER, yylex(driver).token() );
  EXPECT_TRUE( driver.d().gotError() );
  EXPECT_EQ(Parser::token::TOK_END_OF_FILE, yylex(driver).token() );
}

TEST(ScannerTest, MAKE_TEST_NAME(
    an_id,
    yylex,
    returns_TOK_ID_AND_the_ids_name_as_semantic_value_AND_succeeds)) {
  DriverOnTmpFile driver( "foo" );
  Parser::symbol_type st = yylex(driver);
  EXPECT_EQ(Parser::token::TOK_ID, st.token() );
  EXPECT_EQ("foo", st.value.as<string>());
  EXPECT_FALSE( driver.d().gotError() );
}

TEST(ScannerTest, MAKE_TEST_NAME(
    a_colon_followed_by_an_equal_without_blanks_inbetween,
    yylex_is_called,
    returns_the_single_token_COLON_EQUAL_opposed_to_the_two_separate_tokens_COLON_and_EQUAL)) {
  DriverOnTmpFile driver( ":=" );
  EXPECT_EQ(Parser::token::TOK_COLON_EQUAL, yylex(driver).token() );
  EXPECT_EQ(Parser::token::TOK_END_OF_FILE, yylex(driver).token() );
  EXPECT_FALSE( driver.d().gotError() );
}

TEST(ScannerTest, MAKE_TEST_NAME(
    a_colon_followed_by_blanks_followed_by_an_equal,
    yylex_is_called_repeatedly,
    returns_two_separate_tokens_COLON_and_EQUAL_opposed_to_the_single_token_COLON_EQUAL)) {
  DriverOnTmpFile driver( ": =" );
  EXPECT_EQ(Parser::token::TOK_COLON, yylex(driver).token() );
  EXPECT_EQ(Parser::token::TOK_EQUAL, yylex(driver).token() );
  EXPECT_EQ(Parser::token::TOK_END_OF_FILE, yylex(driver).token() );
  EXPECT_FALSE( driver.d().gotError() );
}

TEST(ScannerTest, MAKE_TEST_NAME(
    a_comment_given_by_a_sharp_sign_upto_newline,
    yylex_is_called_repeatedly,
    returns_a_token_sequence_which_ignores_the_comment)) {
  DriverOnTmpFile driver( "x #foo\n y" );
  EXPECT_EQ(Parser::token::TOK_ID, yylex(driver).token() );
  EXPECT_EQ(Parser::token::TOK_ID, yylex(driver).token() );
  EXPECT_EQ(Parser::token::TOK_END_OF_FILE, yylex(driver).token() );
  EXPECT_FALSE( driver.d().gotError() );
}

TEST(ScannerTest, MAKE_TEST_NAME(
    a_comment_on_last_line_of_file,
    yylex_is_called_repeatedly,
    returns_a_token_sequence_which_ignores_the_comment)) {
  DriverOnTmpFile driver( "x #foo" );
  EXPECT_EQ(Parser::token::TOK_ID, yylex(driver).token() );
  EXPECT_EQ(Parser::token::TOK_END_OF_FILE, yylex(driver).token() );
  EXPECT_FALSE( driver.d().gotError() );
}
