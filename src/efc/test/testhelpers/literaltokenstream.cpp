#include "literaltokenstream.h"

#include "../parser.h"

using namespace std;
using namespace yy;

LiteralTokenStream::LiteralTokenStream(const vector<Parser::token_type>& tokens)
  : m_frontPos(0)
  , m_size(tokens.size())
  , m_stream(new Parser::symbol_type[m_size]) {
  for (size_t i = 0; i < tokens.size(); ++i) {
    auto tmp = Parser::makeToken(tokens[i]);
    m_stream[i].move(tmp);
  }
}

Parser::symbol_type LiteralTokenStream::pop() {
  assert(m_frontPos < m_size);
  ++m_frontPos;
  return m_stream[m_frontPos - 1];
}
