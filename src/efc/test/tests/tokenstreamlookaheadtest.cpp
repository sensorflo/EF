#include "test.h"
#include "../tokenstreamlookahead.h"
#include "literaltokenstream.h"
#include "tokentesthelper.h"

using namespace testing;
using namespace std;
using namespace yy;


TEST(TokenStreamLookAheadTest, MAKE_TEST_NAME2(
    pop,
    it_returns_and_removes_the_front_token)) {
  LiteralTokenStream input(TOKIL3(TOK_IF, TOK_ELSE, TOK_ELIF));
  TokenStreamLookAhead UUT(input);
  EXPECT_TOK_EQ(TOK_IF, UUT.pop());
  EXPECT_TOK_EQ(TOK_ELSE, UUT.pop());
  EXPECT_TOK_EQ(TOK_ELIF, UUT.pop());
}

TEST(TokenStreamLookAheadTest, MAKE_TEST_NAME2(
    lookAhead,
    it_returns_the_token_at_pos_without_removing_it)) {

  {
    LiteralTokenStream input(TOKIL3(TOK_IF, TOK_ELSE, TOK_ELIF));
    TokenStreamLookAhead UUT(input);
    EXPECT_TOK_EQ(TOK_IF, UUT.lookAhead(0)) << "test: token at pos";
    EXPECT_TOK_EQ(TOK_IF, UUT.lookAhead(0)) << "test: does not remove token";
  }

  {
    LiteralTokenStream input(TOKIL3(TOK_IF, TOK_ELSE, TOK_ELIF));
    TokenStreamLookAhead UUT(input);
    EXPECT_TOK_EQ(TOK_ELSE, UUT.lookAhead(1)) << "test: token at pos";
    EXPECT_TOK_EQ(TOK_ELSE, UUT.lookAhead(1)) << "test: does not remove token";
  }

  // test arbitrary order of calls
  {
    LiteralTokenStream input(TOKIL3(TOK_IF, TOK_ELSE, TOK_ELIF));
    TokenStreamLookAhead UUT(input);
    EXPECT_TOK_EQ(TOK_ELSE, UUT.lookAhead(1));
    EXPECT_TOK_EQ(TOK_IF, UUT.lookAhead(0));
  }
  {
    LiteralTokenStream input(TOKIL3(TOK_IF, TOK_ELSE, TOK_ELIF));
    TokenStreamLookAhead UUT(input);
    EXPECT_TOK_EQ(TOK_IF, UUT.lookAhead(0));
    EXPECT_TOK_EQ(TOK_ELSE, UUT.lookAhead(1));
  }
}

TEST(TokenStreamLookAheadTest, MAKE_TEST_NAME1(
    front_and_lookAhead_are_called_in_any_order)) {

  string spec = "Example: first lookAhead, then pop";
  {
    LiteralTokenStream input(TOKIL3(TOK_IF, TOK_ELSE, TOK_ELIF));
    TokenStreamLookAhead UUT(input);
    EXPECT_TOK_EQ(TOK_IF, UUT.lookAhead(0)) << amendSpec(spec);
    EXPECT_TOK_EQ(TOK_ELSE, UUT.lookAhead(1)) << amendSpec(spec);
    EXPECT_TOK_EQ(TOK_IF, UUT.pop()) << amendSpec(spec);
  }

  spec = "Example: first pop, then lookAhead";
  {
    LiteralTokenStream input(TOKIL3(TOK_IF, TOK_ELSE, TOK_ELIF));
    TokenStreamLookAhead UUT(input);
    EXPECT_TOK_EQ(TOK_IF, UUT.pop()) << amendSpec(spec);
    EXPECT_TOK_EQ(TOK_ELSE, UUT.lookAhead(0)) << amendSpec(spec);
    EXPECT_TOK_EQ(TOK_ELSE, UUT.lookAhead(0)) << amendSpec(spec);
  }

  spec = "Example: lookAhead-pop pairs";
  {
    LiteralTokenStream input(TOKIL3(TOK_NEWLINE, TOK_COMMA, TOK_END_OF_FILE));
    TokenStreamLookAhead UUT(input);
    EXPECT_TOK_EQ(TOK_NEWLINE, UUT.lookAhead(0)) << amendSpec(spec);
    EXPECT_TOK_EQ(TOK_NEWLINE, UUT.pop()) << amendSpec(spec);
    EXPECT_TOK_EQ(TOK_COMMA, UUT.lookAhead(0)) << amendSpec(spec);
    EXPECT_TOK_EQ(TOK_COMMA, UUT.pop()) << amendSpec(spec);
  }
}

TEST(TokenStreamLookAheadTest, MAKE_TEST_NAME1(
    ctorscheisse)) {
  LiteralTokenStream input(TOKIL3(TOK_IF, TOK_ELSE, TOK_ELIF));
  TokenStreamLookAhead UUT(input);
  UUT.lookAhead(1);
}
