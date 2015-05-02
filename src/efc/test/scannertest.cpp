#include "test.h"
#include "../scanner.h"
#include "driverontmpfile.h"
#include <string>
#include <vector>
using namespace testing;
using namespace std;
using namespace yy;

/** Tests that the scanner produces from the given input the given expected
sequence of tokens. TOK_END_OF_FILE is implicitely appended to the given
expected token sequence for the caller's convenience. */
void testScanner(const string& input,
  const string& spec,
  vector<Parser::token_type>&& expectedTokenSequence) {
  // setup
  DriverOnTmpFile driver( input );
  Scanner& UUT = driver.scanner(); 
  expectedTokenSequence.push_back(Parser::token::TOK_END_OF_FILE);
  
  for (const auto& expectedToken : expectedTokenSequence) {
    // execute
    const auto& actualToken = UUT.pop().token();

    // verify 
    EXPECT_EQ(expectedToken, actualToken) << amendSpec(spec)
      << "Input: '" << input << "'\n";
    EXPECT_FALSE(driver.d().gotError()) << amendSpec(spec)
      << "Input: '" << input << "'\n";
    if (Parser::token::TOK_END_OF_FILE==actualToken ||
      Parser::token::TOK_END_OF_FILE==expectedToken) {
      break;
    }
  }
}

/** Analogous to testScanner, see there */
#define TEST_SCANNER(input, spec, ... )                              \
  {                                                                  \
    SCOPED_TRACE("testScanner called from here (via TEST_SCANNER)"); \
    testScanner(input, spec, {__VA_ARGS__} );                        \
  }

TEST(ScannerTest, MAKE_TEST_NAME(
    an_empty_file,
    yylex,
    returns_TOK_END_OF_FILE_AND_succeeds)) {
  TEST_SCANNER( "", "" );
}

TEST(ScannerTest, MAKE_TEST_NAME(
    a_keyword,
    yylex,
    returns_token_of_that_keyword_notably_opposed_to_token_for_id_AND_succeeds)) {
  TEST_SCANNER( "if", "", Parser::token::TOK_IF);
}

TEST(ScannerTest, MAKE_TEST_NAME(
    an_id_consisting_of_concatenated_keywords,
    yylex,
    returns_TOK_ID_AND_succeeds)) {
  TEST_SCANNER( "ifelse", "", Parser::token::TOK_ID);
}

TEST(ScannerTest, MAKE_TEST_NAME(
    keywords_separated_by_blanks,
    yylex_is_called_repeatedly,
    returns_the_keywords_tokens_AND_succeeds)) {
  TEST_SCANNER( "if else", "", Parser::token::TOK_IF, Parser::token::TOK_ELSE);
}

TEST(ScannerTest, MAKE_TEST_NAME(
    a_literal_char,
    yylex,
    returns_TOK_NUMBER_AND_the_char_s_value_and_it_s_type_as_semantic_value_AND_succeeds)) {

  DriverOnTmpFile driver( "'x'" );
  Scanner& UUT = driver.scanner(); 

  Parser::symbol_type st = UUT.pop();
  EXPECT_EQ(Parser::token::TOK_NUMBER, st.token());
  EXPECT_EQ('x', st.value.as<NumberToken>().m_value );
  ObjTypeFunda expectedType(ObjTypeFunda::eChar);
  EXPECT_EQ(ObjType::eFullMatch, st.value.as<NumberToken>().m_objType->match(expectedType));

  EXPECT_EQ(Parser::token::TOK_END_OF_FILE, UUT.pop().token() );

  EXPECT_FALSE( driver.d().gotError() );
}

TEST(ScannerTest, MAKE_TEST_NAME(
    a_literal_number,
    yylex,
    returns_TOK_NUMBER_AND_the_number_s_value_and_it_s_type_as_semantic_value_AND_succeeds)) {

  string spec = "Example: a literal integral value";
  {
    DriverOnTmpFile driver( "42" );
    Scanner& UUT = driver.scanner(); 

    Parser::symbol_type st = UUT.pop();
    EXPECT_EQ(Parser::token::TOK_NUMBER, st.token());
    EXPECT_EQ(42, st.value.as<NumberToken>().m_value );
    ObjTypeFunda expectedType(ObjTypeFunda::eInt);
    EXPECT_EQ(ObjType::eFullMatch, st.value.as<NumberToken>().m_objType->match(expectedType));

    EXPECT_EQ(Parser::token::TOK_END_OF_FILE, UUT.pop().token() );

    EXPECT_FALSE( driver.d().gotError() );
  }

  spec = "Example: a literal floating point value";
  {
    DriverOnTmpFile driver( "42.77" );
    Scanner& UUT = driver.scanner(); 

    Parser::symbol_type st = UUT.pop();
    EXPECT_EQ(Parser::token::TOK_NUMBER, st.token());
    EXPECT_EQ(42.77, st.value.as<NumberToken>().m_value );
    ObjTypeFunda expectedType(ObjTypeFunda::eDouble);
    EXPECT_EQ(ObjType::eFullMatch, st.value.as<NumberToken>().m_objType->match(expectedType));

    EXPECT_EQ(Parser::token::TOK_END_OF_FILE, UUT.pop().token() );

    EXPECT_FALSE( driver.d().gotError() );
  }
}

TEST(ScannerTest, MAKE_TEST_NAME(
    the_literal_false,
    yylex,
    returns_TOK_NUMBER_WITH_the_semantic_value_0_as_value_and_bool_as_type_AND_succeeds)) {
  DriverOnTmpFile driver( "false" );
  Scanner& UUT = driver.scanner(); 

  Parser::symbol_type st = UUT.pop();
  EXPECT_EQ(Parser::token::TOK_NUMBER, st.token());
  EXPECT_EQ(0, st.value.as<NumberToken>().m_value );
  ObjTypeFunda expectedType(ObjTypeFunda::eBool);
  EXPECT_EQ(ObjType::eFullMatch, st.value.as<NumberToken>().m_objType->match(expectedType));

  EXPECT_EQ(Parser::token::TOK_END_OF_FILE, UUT.pop().token() );

  EXPECT_FALSE( driver.d().gotError() );
}

TEST(ScannerTest, MAKE_TEST_NAME(
    the_literal_true,
    yylex,
    returns_TOK_NUMBER_WITH_the_semantic_value_1_as_value_and_bool_as_type_AND_succeeds)) {
  DriverOnTmpFile driver( "true" );
  Scanner& UUT = driver.scanner(); 

  Parser::symbol_type st = UUT.pop();
  EXPECT_EQ(Parser::token::TOK_NUMBER, st.token());
  EXPECT_EQ(1, st.value.as<NumberToken>().m_value );
  ObjTypeFunda expectedType(ObjTypeFunda::eBool);
  EXPECT_EQ(ObjType::eFullMatch, st.value.as<NumberToken>().m_objType->match(expectedType));

  EXPECT_EQ(Parser::token::TOK_END_OF_FILE, UUT.pop().token() );

  EXPECT_FALSE( driver.d().gotError() );
}

TEST(ScannerTest, MAKE_TEST_NAME(
    a_literal_number_WITH_an_unknown_suffix,
    yylex,
    returns_TOK_NUMBER_AND_fails)) {
  stringstream errorStream;
  DriverOnTmpFile driver( "42if", &errorStream);
  Scanner& UUT = driver.scanner(); 
  EXPECT_EQ(Parser::token::TOK_NUMBER, UUT.pop().token() );
  EXPECT_TRUE( driver.d().gotError() );
  EXPECT_EQ(Parser::token::TOK_END_OF_FILE, UUT.pop().token() );
}

TEST(ScannerTest, MAKE_TEST_NAME(
    an_id,
    yylex,
    returns_TOK_ID_AND_the_ids_name_as_semantic_value_AND_succeeds)) {
  DriverOnTmpFile driver( "foo" );
  Scanner& UUT = driver.scanner(); 
  Parser::symbol_type st = UUT.pop();
  EXPECT_EQ(Parser::token::TOK_ID, st.token() );
  EXPECT_EQ("foo", st.value.as<string>());
  EXPECT_FALSE( driver.d().gotError() );
}

TEST(ScannerTest, MAKE_TEST_NAME(
    a_colon_followed_by_an_equal_without_blanks_inbetween,
    yylex_is_called,
    returns_the_single_token_COLON_EQUAL_opposed_to_the_two_separate_tokens_COLON_and_EQUAL)) {
  TEST_SCANNER( ":=", "", Parser::token::TOK_COLON_EQUAL);
}

TEST(ScannerTest, MAKE_TEST_NAME(
    an_operator_in_call_syntax,
    yylex_is_called,
    returns_the_single_token_OP_NAME_AND_the_oparators_char_as_semantic_value)) {

  string spec = "trivial example";
  {
    DriverOnTmpFile driver( "op*" );
    Scanner& UUT = driver.scanner(); 
    Parser::symbol_type st = UUT.pop();
    EXPECT_EQ(Parser::token::TOK_OP_NAME, st.token() ) << amendSpec(spec);
    EXPECT_EQ("*", st.value.as<string>()) << amendSpec(spec);
    EXPECT_EQ(Parser::token::TOK_END_OF_FILE, UUT.pop().token() ) << amendSpec(spec);
    EXPECT_FALSE( driver.d().gotError() ) << amendSpec(spec);
  }

  spec = "an operator with multiple chars (punctuation)";
  {
    DriverOnTmpFile driver( "op&&" );
    Scanner& UUT = driver.scanner(); 
    Parser::symbol_type st = UUT.pop();
    EXPECT_EQ(Parser::token::TOK_OP_NAME, st.token() ) << amendSpec(spec);
    EXPECT_EQ("&&", st.value.as<string>()) << amendSpec(spec);
    EXPECT_EQ(Parser::token::TOK_END_OF_FILE, UUT.pop().token() ) << amendSpec(spec);
    EXPECT_FALSE( driver.d().gotError() ) << amendSpec(spec);
  }

  spec = "an operator with multiple chars (alphanumeric, thus preceded by an underscore)";
  {
    DriverOnTmpFile driver( "op_and" );
    Scanner& UUT = driver.scanner(); 
    Parser::symbol_type st = UUT.pop();
    EXPECT_EQ(Parser::token::TOK_OP_NAME, st.token() ) << amendSpec(spec);
    EXPECT_EQ("and", st.value.as<string>()) << amendSpec(spec);
    EXPECT_EQ(Parser::token::TOK_END_OF_FILE, UUT.pop().token() ) << amendSpec(spec);
    EXPECT_FALSE( driver.d().gotError() ) << amendSpec(spec);
  }
}

TEST(ScannerTest, MAKE_TEST_NAME(
    a_colon_followed_by_blanks_followed_by_an_equal,
    yylex_is_called_repeatedly,
    returns_two_separate_tokens_COLON_and_EQUAL_opposed_to_the_single_token_COLON_EQUAL)) {
  TEST_SCANNER( ": =", "", Parser::token::TOK_COLON, Parser::token::TOK_EQUAL);
}

TEST(ScannerTest, MAKE_TEST_NAME(
    a_program_containing_a_single_line_comment_which_is_defined_by_two_slashes_upto_and_inclusive_next_newline_or_end_of_file,
    yylex_is_called_repeatedly,
    returns_a_token_sequence_which_ignores_the_comment)) {

  string spec = "Trivial example";
  TEST_SCANNER( "//foo\n", spec );

  spec = "Simple example";
  TEST_SCANNER( "if //foo\n if", spec,
    Parser::token::TOK_IF, Parser::token::TOK_IF );

  spec = "Border case example: // are the last two chars before the newline";
  TEST_SCANNER( "if //\n if", spec,
    Parser::token::TOK_IF, Parser::token::TOK_IF );

  spec = "Border case example: the single line comment is on the last line"
    "of the file which is not delimited by newline";
  TEST_SCANNER( "if //foo", spec, Parser::token::TOK_IF);

  spec = "Border case example: // are the last two chars in the file";
  TEST_SCANNER( "if //", spec, Parser::token::TOK_IF);
}

TEST(ScannerTest, MAKE_TEST_NAME(
    a_program_containing_a_single_line_comment_which_is_defined_by_a_hash_followed_by_exlamation_mark_upto_and_inclusive_next_newline_or_end_of_file,
    yylex_is_called_repeatedly,
    returns_a_token_sequence_which_ignores_the_comment)) {

  string spec = "Trivial example";
  TEST_SCANNER( "#!foo\n", spec);

  spec = "Simple example";
  TEST_SCANNER( "if #!foo\n if", spec,
    Parser::token::TOK_IF, Parser::token::TOK_IF);

  spec = "Border case example: #! are the last two chars before the newline";
  TEST_SCANNER( "if #!\n if", spec,
    Parser::token::TOK_IF, Parser::token::TOK_IF);

  spec = "Border case example: the single line comment is on the last line"
    "of the file which is not delimited by newline";
  TEST_SCANNER( "if #!foo", spec,  Parser::token::TOK_IF);

  spec = "Border case example: #! are the last two chars in the file";
  TEST_SCANNER( "if #!", spec,  Parser::token::TOK_IF);
}

TEST(ScannerTest, MAKE_TEST_NAME(
    a_program_containing_a_multi_line_comment_which_is_defined_as_everything_between_sharp_star_and_star_sharp,
    yylex_is_called_repeatedly,
    returns_a_token_sequence_which_ignores_the_comment)) {

  string spec = "trivial example";
  TEST_SCANNER( "/*foo*/", spec);

  spec = "simple example";
  TEST_SCANNER( "x /*foo*/ y", spec,
    Parser::token::TOK_ID, Parser::token::TOK_ID);

  spec = "example: comment contains newlines";
  TEST_SCANNER( "x /*foo\nbar*/ y", spec,
    Parser::token::TOK_ID, Parser::token::TOK_ID);

  spec = "example: comment contains *";
  TEST_SCANNER( "x /* foo*bar */ y", spec,
    Parser::token::TOK_ID, Parser::token::TOK_ID);

  spec = "border case example: empty comment";
  TEST_SCANNER( "/**/", spec);

  spec = "border case example: only stars inside comment";
  TEST_SCANNER( "/***/", spec);

  spec = "border case example: stars inside comment";
  TEST_SCANNER( "/* foo**bar */", spec);
}
