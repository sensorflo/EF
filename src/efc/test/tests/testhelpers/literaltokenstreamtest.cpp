#include "literaltokenstream.h"
#include "tokentesthelper.h"
using namespace std;
using namespace yy;

TEST(LiteralTokenStreamTest, MAKE_TEST_NAME2(
    pop,
    it_returns_and_removes_the_front_token)) {
  LiteralTokenStream UUT(TOKIL2(TOK_ID, TOK_NUMBER));
  EXPECT_TOK_EQ(TOK_ID, UUT.pop());
  EXPECT_TOK_EQ(TOK_NUMBER, UUT.pop());
}

TEST(LiteralTokenStreamTest, MAKE_TEST_NAME2(
    front,
    it_returns_the_front_token_without_removing_it)) {
  LiteralTokenStream UUT(TOKIL2(TOK_ID, TOK_NUMBER));
  EXPECT_TOK_EQ(TOK_ID, UUT.front());
  EXPECT_TOK_EQ(TOK_ID, UUT.front());
}

TEST(LiteralTokenStreamTest, MAKE_TEST_NAME1(
    front_and_pop_are_called_in_any_order)) {

  string spec = "Example: first front, then pop";
  {
    LiteralTokenStream UUT(TOKIL2(TOK_ID, TOK_NUMBER));
    EXPECT_TOK_EQ(TOK_ID, UUT.front()) << amendSpec(spec);
    EXPECT_TOK_EQ(TOK_ID, UUT.pop()) << amendSpec(spec);
  }

  spec = "Example: first pop, then front";
  {
    LiteralTokenStream UUT(TOKIL2(TOK_ID, TOK_NUMBER));
    EXPECT_TOK_EQ(TOK_ID, UUT.pop()) << amendSpec(spec);
    EXPECT_TOK_EQ(TOK_NUMBER, UUT.front()) << amendSpec(spec);
  }
}
