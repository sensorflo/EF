#include "tokenstreamlookahead.h"

TokenStreamLookAhead::TokenStreamLookAhead(TokenStream& input)
  : m_lookAheadCnt{0}, m_frontRawIndex{0}, m_input{input} {
}

TokenStreamLookAhead::~TokenStreamLookAhead() {
  // pop() is final, so we can call it in dtor
  while (m_lookAheadCnt > 0) { pop(); }
}

Parser::symbol_type TokenStreamLookAhead::pop() {
  if (0 == m_lookAheadCnt) { return m_input.pop(); }
  auto frontRawIndexOld = m_frontRawIndex;
  m_frontRawIndex = (m_frontRawIndex + 1) % m_buf.size();
  --m_lookAheadCnt;
  return m_buf.at(frontRawIndexOld);
}

Parser::symbol_type& TokenStreamLookAhead::lookAhead(size_t pos) {
  assert(pos < m_buf.size());
  size_t backRawIndex = (m_frontRawIndex + m_lookAheadCnt) % m_buf.size();
  while (pos >= m_lookAheadCnt) {
    Parser::symbol_type tmp{m_input.pop()};
    m_buf.at(backRawIndex).~basic_symbol();
    m_buf.at(backRawIndex).move(tmp);
    backRawIndex = (backRawIndex + 1) % m_buf.size();
    ++m_lookAheadCnt;
  }
  return m_buf.at((m_frontRawIndex + pos) % m_buf.size());
}
