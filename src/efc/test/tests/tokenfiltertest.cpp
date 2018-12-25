#include "test.h"
#include "../tokenfilter.h"
#include "literaltokenstream.h"
#include "tokentesthelper.h"
using namespace testing;
using namespace std;
using namespace yy;

// Not the the beginning of the tokenstream is special, thus the first token
// must be one to bring the state machine to its default state
#define BOUNDED_TOKIL1(t1) TOKIL3(TOK_RBRACE, t1, TOK_LBRACE)
#define BOUNDED_TOKIL2(t1, t2) TOKIL4(TOK_RBRACE, t1, t2, TOK_LBRACE)
#define BOUNDED_TOKIL3(t1, t2, t3) TOKIL5(TOK_RBRACE, t1, t2, t3, TOK_LBRACE)

#define A_SEPARATOR_TOK TOK_COMMA
#define A_STARTER_TOK TOK_LPAREN
#define A_DELIMITER_TOK TOK_RPAREN
#define A_COMPONENT_TOK TOK_ID

/** Tests that the scanner produces from the given input the given expected
sequence of tokens. TOK_END_OF_FILE is implicitely appended to the given
expected token sequence for the caller's convenience. */
void testTokenFilter(
  vector<Parser::token_type>&& inputTokens,
  vector<Parser::token_type>&& expectedTokens,
  const string& spec) {
  // setup
  inputTokens.push_back(Parser::token::TOK_END_OF_FILE);
  expectedTokens.push_back(Parser::token::TOK_END_OF_FILE);
  LiteralTokenStream literalTokenStream(inputTokens);
  TokenFilter UUT(literalTokenStream); 
  
  int i = 0;
  for (const auto& expectedToken : expectedTokens) {
    // execute
    const auto& actualToken = UUT.pop().token();

    // verify 
    EXPECT_EQ(expectedToken, actualToken) << amendSpec(spec)
      << amend(inputTokens, expectedTokens, i);
    if (Parser::token::TOK_END_OF_FILE==actualToken ||
      Parser::token::TOK_END_OF_FILE==expectedToken) {
      break;
    }

    ++i;
  }
}

/** Analogous to testTokenFilter, see there */
#define TEST_TOKENFILTER(inputTokens, expectedTokens, spec )          \
  {                                                                   \
    SCOPED_TRACE("testTokenFilter called from here (via TEST_SCANNER)"); \
    testTokenFilter(inputTokens, expectedTokens, spec);                 \
  }

TEST(TokenFilterTest, MAKE_TEST_NAME2(
    GIVEN_a_token_stream_witout_any_NEWLINE_tokens,
    THEN_the_same_stream_of_tokens_is_returned) ) {
  TEST_TOKENFILTER( TOKIL0(), TOKIL0(), "");
  TEST_TOKENFILTER( TOKIL1(TOK_ID), TOKIL1(TOK_ID), "");
  TEST_TOKENFILTER( TOKIL2(TOK_NUMBER, TOK_IF), TOKIL2(TOK_NUMBER, TOK_IF), "");
}

TEST(TokenFilterTest, MAKE_TEST_NAME2(
    GIVEN_the_beginning_of_a_stream_of_tokens,
    THEN_then_any_leading_NEWLINE_tokens_are_droped) ) {
  // the A_STARTER_TOK is to prevent the implicitely added TOK_END_OF_FILE to
  // eat the newlines, so the newlines have to be eaten by the start-of-stream
  // rule
  TEST_TOKENFILTER(
    TOKIL2(TOK_NEWLINE, A_STARTER_TOK),
    TOKIL1(A_STARTER_TOK), "");
  TEST_TOKENFILTER(
    TOKIL3(TOK_NEWLINE, TOK_NEWLINE, A_STARTER_TOK),
    TOKIL1(A_STARTER_TOK), "");
}

TEST(TokenFilterTest, MAKE_TEST_NAME2(
    GIVEN_an_token_of_class_starter,
    THEN_then_any_trailing_NEWLINE_tokens_are_droped) ) {
  string spec = "Example: token of class starter with trailing newlines";
  TEST_TOKENFILTER(
    BOUNDED_TOKIL2(A_STARTER_TOK, TOK_NEWLINE),
    BOUNDED_TOKIL1(A_STARTER_TOK), spec);
  TEST_TOKENFILTER(
    BOUNDED_TOKIL3(A_STARTER_TOK, TOK_NEWLINE, TOK_NEWLINE),
    BOUNDED_TOKIL1(A_STARTER_TOK), spec);

  spec = "border case: starter token with no trailing NEWLINEs";
  TEST_TOKENFILTER(
    BOUNDED_TOKIL1(A_STARTER_TOK),
    BOUNDED_TOKIL1(A_STARTER_TOK), spec);
}

TEST(TokenFilterTest, MAKE_TEST_NAME2(
    GIVEN_an_token_of_class_delimiter,
    THEN_then_any_leading_NEWLINE_tokens_are_droped) ) {
  TEST_TOKENFILTER(
    BOUNDED_TOKIL2(TOK_NEWLINE, A_DELIMITER_TOK),
    BOUNDED_TOKIL1(A_DELIMITER_TOK), "");
  TEST_TOKENFILTER(
    BOUNDED_TOKIL3(TOK_NEWLINE, TOK_NEWLINE, A_DELIMITER_TOK),
    BOUNDED_TOKIL1(A_DELIMITER_TOK), "");

  string spec = "border case: no leading NEWLINEs";
  TEST_TOKENFILTER(
    BOUNDED_TOKIL1(A_DELIMITER_TOK),
    BOUNDED_TOKIL1(A_DELIMITER_TOK), spec);
}

TEST(TokenFilterTest, MAKE_TEST_NAME2(
    GIVEN_an_token_of_class_separator,
    THEN_then_any_surrounding_NEWLINE_tokens_are_droped) ) {

  string spec = "Example: trailing newlines";
  TEST_TOKENFILTER(
    BOUNDED_TOKIL2(A_SEPARATOR_TOK, TOK_NEWLINE),
    BOUNDED_TOKIL1(A_SEPARATOR_TOK), spec);
  TEST_TOKENFILTER(
    BOUNDED_TOKIL3(A_SEPARATOR_TOK, TOK_NEWLINE, TOK_NEWLINE),
    BOUNDED_TOKIL1(A_SEPARATOR_TOK), spec);

  spec = "Example: leading newlines";
  TEST_TOKENFILTER(
    BOUNDED_TOKIL2(TOK_NEWLINE, A_SEPARATOR_TOK),
    BOUNDED_TOKIL1(A_SEPARATOR_TOK), "");
  TEST_TOKENFILTER(
    BOUNDED_TOKIL3(TOK_NEWLINE, TOK_NEWLINE, A_SEPARATOR_TOK),
    BOUNDED_TOKIL1(A_SEPARATOR_TOK), "");

  spec = "Example: leading and trailing newlines";
  TEST_TOKENFILTER(
    BOUNDED_TOKIL3(TOK_NEWLINE, A_SEPARATOR_TOK, TOK_NEWLINE),
    BOUNDED_TOKIL1(A_SEPARATOR_TOK), "");

  spec = "border case: no surrounding NEWLINEs";
  TEST_TOKENFILTER(
    BOUNDED_TOKIL1(A_SEPARATOR_TOK),
    BOUNDED_TOKIL1(A_SEPARATOR_TOK), spec);
}

TEST(TokenFilterTest, MAKE_TEST_NAME2(
    GIVEN_that_after_the_prevoius_rules_a_sequence_of_NEWLINEs_remain,
    THEN_all_but_the_fist_NEWLINE_tokens_are_droped)) {

  string spec = "Example: trailing newlines after token class compenent";
  TEST_TOKENFILTER(
    BOUNDED_TOKIL2(A_COMPONENT_TOK, TOK_NEWLINE),
    BOUNDED_TOKIL2(A_COMPONENT_TOK, TOK_NEWLINE), spec);
  TEST_TOKENFILTER(
    BOUNDED_TOKIL3(A_COMPONENT_TOK, TOK_NEWLINE, TOK_NEWLINE),
    BOUNDED_TOKIL2(A_COMPONENT_TOK, TOK_NEWLINE), spec);

  spec = "Example: trailing newlines after token class delimiter";
  TEST_TOKENFILTER(
    BOUNDED_TOKIL2(A_DELIMITER_TOK, TOK_NEWLINE),
    BOUNDED_TOKIL2(A_DELIMITER_TOK, TOK_NEWLINE), spec);
  TEST_TOKENFILTER(
    BOUNDED_TOKIL3(A_DELIMITER_TOK, TOK_NEWLINE, TOK_NEWLINE),
    BOUNDED_TOKIL2(A_DELIMITER_TOK, TOK_NEWLINE), spec);

  spec = "Example: leading newlines bevore token class component";
  TEST_TOKENFILTER(
    BOUNDED_TOKIL2(TOK_NEWLINE, A_COMPONENT_TOK),
    BOUNDED_TOKIL2(TOK_NEWLINE, A_COMPONENT_TOK), spec);
  TEST_TOKENFILTER(
    BOUNDED_TOKIL3(TOK_NEWLINE, TOK_NEWLINE, A_COMPONENT_TOK),
    BOUNDED_TOKIL2(TOK_NEWLINE, A_COMPONENT_TOK), spec);

  spec = "Example: leading newlines bevore token class starter";
  TEST_TOKENFILTER(
    BOUNDED_TOKIL2(TOK_NEWLINE, A_STARTER_TOK),
    BOUNDED_TOKIL2(TOK_NEWLINE, A_STARTER_TOK), spec);
  TEST_TOKENFILTER(
    BOUNDED_TOKIL3(TOK_NEWLINE, TOK_NEWLINE, A_STARTER_TOK),
    BOUNDED_TOKIL2(TOK_NEWLINE, A_STARTER_TOK), spec);

  spec = "Example: leading and trailing newlines around token class component";
  TEST_TOKENFILTER(
    BOUNDED_TOKIL3(TOK_NEWLINE, A_COMPONENT_TOK, TOK_NEWLINE),
    BOUNDED_TOKIL3(TOK_NEWLINE, A_COMPONENT_TOK, TOK_NEWLINE), spec);
}

