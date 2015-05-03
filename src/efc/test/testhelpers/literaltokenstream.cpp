#include "literaltokenstream.h"
using namespace std;
using namespace yy;

LiteralTokenStream::LiteralTokenStream(const vector<Parser::token_type>& tokens) :
  m_front(0),
  m_size(tokens.size()),
  m_stream(new Parser::symbol_type[m_size]) {
  for (int i=0; i<tokens.size(); ++i) {
    auto tmp = ParserApiExt::makeToken(tokens[i]);
    m_stream[i].move(tmp);
  }
};

Parser::symbol_type& LiteralTokenStream::front() {
  return m_stream[m_front];
}

Parser::symbol_type LiteralTokenStream::pop() {
  assert(m_front<m_size);
  ++m_front;
  return m_stream[m_front-1];
}
