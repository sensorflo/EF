#include "ast.h"
#include <sstream>
using namespace std;

string AstNode::toStr() {
  ostringstream ss;
  printTo(ss);
  return ss.str();
}

basic_ostream<char>& AstNumber::printTo(basic_ostream<char>& os) {
  return os << m_number;
}

