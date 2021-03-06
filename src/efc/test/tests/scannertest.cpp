#include "test.h"
#include "../errorhandler.h"
#include "../scanner.h"
#include "driverontmpfile.h"
#include "tokentesthelper.h"

#include <string>
#include <sstream>
#include <vector>

using namespace testing;
using namespace std;
using namespace yy;

/** Tests that the scanner produces from the given input the given expected
sequence of tokens. TOK_END_OF_FILE is implicitely appended to the given
expected token sequence for the caller's convenience. */
void testScanner(const string& input, const string& spec,
  vector<Parser::token_type>&& expectedTokenSequence) {
  // setup
  DriverOnTmpFile driver(input);
  Scanner& UUT = driver.scanner();
  expectedTokenSequence.push_back(Parser::token::TOK_END_OF_FILE);

  for (const auto& expectedToken : expectedTokenSequence) {
    // execute
    const auto& actualToken = UUT.pop().token();

    // verify
    EXPECT_EQ(expectedToken, actualToken)
      << amendSpec(spec) << "Input: '" << input << "'\n";
    if (Parser::token::TOK_END_OF_FILE == actualToken ||
      Parser::token::TOK_END_OF_FILE == expectedToken) {
      break;
    }
  }
}

/** Analogous to testScanner, see there */
#define TEST_SCANNER(input, spec, expectedTokens)                    \
  {                                                                  \
    SCOPED_TRACE("testScanner called from here (via TEST_SCANNER)"); \
    testScanner(input, spec, expectedTokens);                        \
  }

void testScannerReportsError(const string& input, Error::No expectedErrorNo,
  const string& expectedMsgParam1, const string& expectedMsgParam2,
  const string& expectedMsgParam3, const string& spec) {
  // setup
  DriverOnTmpFile driver(input);
  Scanner& UUT = driver.scanner();
  stringstream tokenSs;

  // execute
  const auto body = [&](){
    bool isFirstIter = true;
    while (true) {
      const auto& token = UUT.pop().token();
      if (!isFirstIter) {
        tokenSs << ", ";
      }
      isFirstIter = false;
      tokenSs << token;
      if (Parser::token::TOK_END_OF_FILE == token) { break; }
    }
  };
  const auto res = tryCatch(body);

  // verify
  const auto details = amendSpec(spec) + "Input: '" + input + "'\n" +
    "returned tokens: {" + tokenSs.str() + "}";
  SCOPED_TRACE("verifyErrorHandlerHasExpectedError called from here");
  verifyErrorHandlerHasExpectedError(driver.errorHandler(), details,
    res.m_foreignCatches, res.m_excptionWhat, expectedErrorNo, expectedMsgParam1,
    expectedMsgParam2, expectedMsgParam3);
}

/** Analogous to testScannerReportsError, see there */
#define TEST_SCANNER_REPORTS_ERROR(input, expectedErrorNo, spec)     \
  {                                                                  \
    SCOPED_TRACE("testScannerReportsError called from here"); \
    testScannerReportsError(input, expectedErrorNo, "", "", "", spec);   \
  }

#define TEST_SCANNER_REPORTS_ERROR_1MSGPARAM(input, expectedErrorNo, expectedMsgParam1, spec) \
  {                                                                  \
    SCOPED_TRACE("testScannerReportsError called from here"); \
    testScannerReportsError(input, expectedErrorNo, expectedMsgParam1, "", "", spec); \
  }

#define TEST_SCANNER_REPORTS_ERROR_2MSGPARAMS(input, expectedErrorNo, expectedMsgParam1, expectedMsgParam2, spec) \
  {                                                                  \
    SCOPED_TRACE("testScannerReportsError called from here"); \
    testScannerReportsError(input, expectedErrorNo, expectedMsgParam1, expectedMsgParam2, "", spec); \
  }

TEST(ScannerTest, MAKE_TEST_NAME(
    an_invalid_file_name,
    create,
    reports_eCantOpenFileForReading)) {
  // setup
  ErrorHandler errorHandler;
  const string invalidFileName = "invalid-file-name.ef";

  // exercise
  const auto res = tryCatch(
    [&](){Scanner::create(invalidFileName, errorHandler);});

  // verify
  SCOPED_TRACE("called from here");
  verifyErrorHandlerHasExpectedError(errorHandler, "",
    res.m_foreignCatches, res.m_excptionWhat, Error::eCantOpenFileForReading,
    invalidFileName, "<dont_verify>");
}

TEST(ScannerTest, MAKE_TEST_NAME2(
    GIVEN_horizontal_white_spaces,
    THEN_they_are_discarded)) {
  TEST_SCANNER(" \t", "", TOKIL0());
}

// Note that the TokenFilter which sits between Scanner and Parser will filter
// out some NEWLINE tokens.
TEST(ScannerTest, MAKE_TEST_NAME(
    an_unescaped_newline,
    pop,
    returns_TOK_NEWLINE)) {
  TEST_SCANNER("\n", "", TOKIL1(TOK_NEWLINE));
}

TEST(ScannerTest, MAKE_TEST_NAME(
    a_backslash_escaped_newline,
    pop,
    discards_the_backslash_and_the_newline_and_any_horizontal_whites_and_any_one_line_comment_inbetween)) {
  TEST_SCANNER("\\\n", "", TOKIL0());
  TEST_SCANNER("\\ \t \n", "", TOKIL0());
  TEST_SCANNER("\\ \t //foo\n", "", TOKIL0());
  TEST_SCANNER("\\ \t #!foo\n", "", TOKIL0());
  // see also the test for multiline comments
}

TEST(ScannerTest, MAKE_TEST_NAME(
    a_keyword,
    pop,
    returns_the_token_associated_with_that_keyword)) {
  // just a few examples, don't test every known keyword
  TEST_SCANNER("if", "", TOKIL1(TOK_IF));
  TEST_SCANNER("elif", "", TOKIL1(TOK_ELIF));
}

TEST(ScannerTest, MAKE_TEST_NAME(
    an_id,
    pop,
    returns_TOK_ID_AND_the_ids_name_as_semantic_value_AND_succeeds)) {
  {
    DriverOnTmpFile driver("foo");
    Scanner& UUT = driver.scanner(); 
    Parser::symbol_type st = UUT.pop();
    EXPECT_TOK_EQ(TOK_ID, st);
    EXPECT_EQ("foo", st.value.as<string>());
  }

  string spec = "Example: An identifier composed of parts which each for"
    "itself is a keyword";
  TEST_SCANNER("ifelse", spec, TOKIL1(TOK_ID));
}

TEST(ScannerTest, MAKE_TEST_NAME(
    lexemes_separated_by_white_space_characters,
    pop_is_called_repeatedly,
    returns_the_sequence_of_tokens_associated_with_each_lexeme)) {
  TEST_SCANNER("if else", "", TOKIL2(TOK_IF, TOK_ELSE));
  TEST_SCANNER("foo bar", "", TOKIL2(TOK_ID, TOK_ID));
  TEST_SCANNER("if foo", "", TOKIL2(TOK_IF, TOK_ID));
}

TEST(ScannerTest, MAKE_TEST_NAME(
    a_literal_char,
    pop,
    returns_TOK_NUMBER_AND_the_char_s_value_and_it_s_type_as_semantic_value)) {
  DriverOnTmpFile driver("'x'");
  Scanner& UUT = driver.scanner(); 

  Parser::symbol_type st = UUT.pop();
  EXPECT_TOK_EQ(TOK_NUMBER, st);
  EXPECT_EQ('x', st.value.as<NumberToken>().m_value);
  EXPECT_EQ(ObjTypeFunda::eChar, st.value.as<NumberToken>().m_objType);

  EXPECT_TOK_EQ(TOK_END_OF_FILE, UUT.pop());
}

TEST(ScannerTest, MAKE_TEST_NAME(
    a_literal_number,
    pop,
    returns_TOK_NUMBER_AND_the_number_s_value_and_it_s_type_as_semantic_value)) {
  string spec = "Example: a literal integral value";
  {
    DriverOnTmpFile driver("42");
    Scanner& UUT = driver.scanner(); 

    Parser::symbol_type st = UUT.pop();
    EXPECT_TOK_EQ(TOK_NUMBER, st);
    EXPECT_EQ(42, st.value.as<NumberToken>().m_value);
    EXPECT_EQ(ObjTypeFunda::eInt, st.value.as<NumberToken>().m_objType);

    EXPECT_TOK_EQ(TOK_END_OF_FILE, UUT.pop());
  }

  spec = "Example: a literal floating point value";
  {
    DriverOnTmpFile driver("42.77");
    Scanner& UUT = driver.scanner();

    Parser::symbol_type st = UUT.pop();
    EXPECT_TOK_EQ(TOK_NUMBER, st);
    EXPECT_EQ(42.77, st.value.as<NumberToken>().m_value);
    EXPECT_EQ(ObjTypeFunda::eDouble, st.value.as<NumberToken>().m_objType);

    EXPECT_TOK_EQ(TOK_END_OF_FILE, UUT.pop());
  }
}

TEST(ScannerTest, MAKE_TEST_NAME(
    the_literal_false,
    pop,
    returns_TOK_NUMBER_WITH_the_semantic_value_0_as_value_and_bool_as_type)) {
  DriverOnTmpFile driver("false");
  Scanner& UUT = driver.scanner();

  Parser::symbol_type st = UUT.pop();
  EXPECT_TOK_EQ(TOK_NUMBER, st);
  EXPECT_EQ(0, st.value.as<NumberToken>().m_value);
  EXPECT_EQ(ObjTypeFunda::eBool, st.value.as<NumberToken>().m_objType);

  EXPECT_TOK_EQ(TOK_END_OF_FILE, UUT.pop());
}

TEST(ScannerTest, MAKE_TEST_NAME(
    the_literal_true,
    pop,
    returns_TOK_NUMBER_WITH_the_semantic_value_1_as_value_and_bool_as_type)) {
  DriverOnTmpFile driver("true");
  Scanner& UUT = driver.scanner();

  Parser::symbol_type st = UUT.pop();
  EXPECT_TOK_EQ(TOK_NUMBER, st);
  EXPECT_EQ(1, st.value.as<NumberToken>().m_value);
  EXPECT_EQ(ObjTypeFunda::eBool, st.value.as<NumberToken>().m_objType);

  EXPECT_TOK_EQ(TOK_END_OF_FILE, UUT.pop());
}

TEST(ScannerTest, MAKE_TEST_NAME(
    a_literal_number_WITH_an_unknown_suffix,
    pop,
    reports_eUnknownIntegralLiteralSuffix)) {
  // setup
  stringstream errorStream;
  DriverOnTmpFile driver("42if", &errorStream);
  Scanner& UUT = driver.scanner();

  // exercise
  try {
    UUT.pop();
  }

  // verify
  catch (...) {
    const auto& errors = driver.errorHandler().errors();
    ASSERT_EQ(1u, errors.size());
    EXPECT_EQ(Error::eUnknownIntegralLiteralSuffix, errors.front()->no());
    return;
  }
  FAIL();
}

TEST(ScannerTest, MAKE_TEST_NAME(
    a_literal_integral_number_which_is_not_representable_by_type_int,
    pop,
    reports_eLiteralOutOfValidRange)) {
  TEST_SCANNER_REPORTS_ERROR_2MSGPARAMS("3141592653589793238462643383",
    Error::eLiteralOutOfValidRange,
    "-2147483648",
    "2147483647",
    "");
}

TEST(ScannerTest, MAKE_TEST_NAME(
    a_literal_float_number_which_is_not_representable_by_type_double,
    pop,
    reports_eLiteralOutOfValidRange)) {
  TEST_SCANNER_REPORTS_ERROR_2MSGPARAMS("3.14E14142135623",
    Error::eLiteralOutOfValidRange,
    "0.000000",
    "1.797693E+308",
    "");
}

TEST(ScannerTest, MAKE_TEST_NAME(
    an_operator_in_call_syntax,
    pop_is_called,
    returns_the_single_token_OP_NAME_AND_the_oparators_char_as_semantic_value)) {
  string spec = "trivial example";
  {
    DriverOnTmpFile driver("op*");
    Scanner& UUT = driver.scanner();
    Parser::symbol_type st = UUT.pop();
    EXPECT_TOK_EQ(TOK_OP_NAME, st) << amendSpec(spec);
    EXPECT_EQ("*", st.value.as<string>()) << amendSpec(spec);
    EXPECT_TOK_EQ(TOK_END_OF_FILE, UUT.pop()) << amendSpec(spec);
  }

  spec = "an operator with multiple chars (punctuation)";
  {
    DriverOnTmpFile driver("op&&");
    Scanner& UUT = driver.scanner(); 
    Parser::symbol_type st = UUT.pop();
    EXPECT_TOK_EQ(TOK_OP_NAME, st) << amendSpec(spec);
    EXPECT_EQ("&&", st.value.as<string>()) << amendSpec(spec);
    EXPECT_TOK_EQ(TOK_END_OF_FILE, UUT.pop()) << amendSpec(spec);
  }

  spec = "an operator with multiple chars (alphanumeric, thus preceded by an underscore)";
  {
    DriverOnTmpFile driver("op_and");
    Scanner& UUT = driver.scanner();
    Parser::symbol_type st = UUT.pop();
    EXPECT_TOK_EQ(TOK_OP_NAME, st) << amendSpec(spec);
    EXPECT_EQ("and", st.value.as<string>()) << amendSpec(spec);
    EXPECT_TOK_EQ(TOK_END_OF_FILE, UUT.pop()) << amendSpec(spec);
  }
}

TEST(ScannerTest, MAKE_TEST_NAME(
    a_colon_followed_by_an_equal_without_blanks_inbetween,
    pop_is_called,
    returns_the_single_token_COLON_EQUAL_opposed_to_the_two_separate_tokens_COLON_and_EQUAL)) {
  TEST_SCANNER(":=", "", TOKIL1(TOK_COLON_EQUAL));
}

TEST(ScannerTest, MAKE_TEST_NAME(
    a_colon_followed_by_blanks_followed_by_an_equal,
    pop_is_called_repeatedly,
    returns_two_separate_tokens_COLON_and_EQUAL_opposed_to_the_single_token_COLON_EQUAL)) {
  TEST_SCANNER(": =", "", TOKIL2(TOK_COLON, TOK_EQUAL));
}

TEST(ScannerTest, MAKE_TEST_NAME(
    a_program_containing_a_single_line_comment_which_is_defined_by_two_slashes_upto_and_inclusive_next_newline_or_end_of_file,
    pop_is_called_repeatedly,
    returns_a_token_sequence_which_ignores_the_comment_BUT_still_generates_NEWLINE_tokens)) {
  string spec = "Trivial example";
  TEST_SCANNER("//foo\n", spec, TOKIL1(TOK_NEWLINE));

  spec = "Simple example";
  TEST_SCANNER("if //foo\n if", spec, TOKIL3(TOK_IF, TOK_NEWLINE, TOK_IF));

  spec = "Border case example: // are the last two chars before the newline";
  TEST_SCANNER("if //\n if", spec, TOKIL3(TOK_IF, TOK_NEWLINE, TOK_IF));

  spec = "Border case example: the single line comment is on the last line"
    "of the file which is not delimited by newline";
  TEST_SCANNER("if //foo", spec, TOKIL1(TOK_IF));

  spec = "Border case example: // are the last two chars in the file";
  TEST_SCANNER("if //", spec, TOKIL1(TOK_IF));
}

TEST(ScannerTest, MAKE_TEST_NAME(
    a_program_containing_a_single_line_comment_which_is_defined_by_a_hash_followed_by_exlamation_mark_upto_and_inclusive_next_newline_or_end_of_file,
    pop_is_called_repeatedly,
    returns_a_token_sequence_which_ignores_the_comment_BUT_still_generates_NEWLINE_tokens)) {
  string spec = "Trivial example";
  TEST_SCANNER("#!foo\n", spec, TOKIL1(TOK_NEWLINE));

  spec = "Simple example";
  TEST_SCANNER("if #!foo\n if", spec, TOKIL3(TOK_IF, TOK_NEWLINE, TOK_IF));

  spec = "Border case example: #! are the last two chars before the newline";
  TEST_SCANNER("if #!\n if", spec, TOKIL3(TOK_IF, TOK_NEWLINE, TOK_IF));

  spec = "Border case example: the single line comment is on the last line"
    "of the file which is not delimited by newline";
  TEST_SCANNER("if #!foo", spec, TOKIL1(TOK_IF));

  spec = "Border case example: #! are the last two chars in the file";
  TEST_SCANNER("if #!", spec, TOKIL1(TOK_IF));
}

TEST(ScannerTest, MAKE_TEST_NAME(
    a_program_containing_a_multi_line_comment_which_is_defined_as_everything_between_sharp_star_and_star_sharp,
    pop_is_called_repeatedly,
    returns_a_token_sequence_which_ignores_the_comment)) {
  string spec = "trivial example";
  TEST_SCANNER("/*foo*/", spec, TOKIL0());

  spec = "simple example";
  TEST_SCANNER("x /*foo*/ y", spec, TOKIL2(TOK_ID, TOK_ID));

  spec = "example: comment contains newlines. A NEWLINE token is _not_ "
    "generated.";
  TEST_SCANNER("x /*foo\nbar*/ y", spec, TOKIL2(TOK_ID, TOK_ID));

  spec = "example: comment contains *";
  TEST_SCANNER("x /* foo*bar */ y", spec, TOKIL2(TOK_ID, TOK_ID));

  spec = "border case example: empty comment";
  TEST_SCANNER("/**/", spec, TOKIL0());

  spec = "border case example: only stars inside comment";
  TEST_SCANNER("/***/", spec, TOKIL0());

  spec = "border case example: stars inside comment";
  TEST_SCANNER("/* foo**bar */", spec, TOKIL0());
}

TEST(ScannerTest, MAKE_TEST_NAME2(
    GIVEN_a_multiline_comment,
    THEN_the_following_token_has_the_correct_location_info)) {
  DriverOnTmpFile driver("/*\n*/\n42");
  Scanner& UUT = driver.scanner();
  const auto& actualPosition = UUT.pop().location.begin;
  EXPECT_EQ(
    Position(actualPosition.m_fileName, 3, 1),
    actualPosition);
}

TEST(ScannerTest, MAKE_TEST_NAME(
    an_invalid_character,
    pop,
    reports_eUnexpectedCharacter)) {
  TEST_SCANNER_REPORTS_ERROR_1MSGPARAM("~",
    Error::eUnexpectedCharacter, "~", "");
  TEST_SCANNER_REPORTS_ERROR_1MSGPARAM("42@",
    Error::eUnexpectedCharacter, "@", "");
}
