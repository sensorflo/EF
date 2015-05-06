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

