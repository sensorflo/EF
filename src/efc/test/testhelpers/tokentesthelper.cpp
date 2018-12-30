#include "tokentesthelper.h"

#include "../parserapiext.h"

#include <sstream>

using namespace std;
using namespace yy;

string amend(const vector<Parser::token_type>& inputTokens,
  const vector<Parser::token_type>& expectedTokens, int currentExpectedPos) {
  stringstream ss;
  ss << "input tokens   : " << inputTokens << "\n"
     << "expected tokens: " << expectedTokens << "\n"
     << "current pos in expected tokens (0 based): " << currentExpectedPos;
  return ss.str();
}
