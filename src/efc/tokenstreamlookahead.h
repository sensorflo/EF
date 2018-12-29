#pragma once
#include "tokenstream.h"
#include <array>

/** Adds lookahead to a given input token stream */
class TokenStreamLookAhead : public TokenStream {
public:
  TokenStreamLookAhead(TokenStream& input);
  ~TokenStreamLookAhead() override;

  yy::Parser::symbol_type pop() override;
  /** Returns reference token at given pos without removing it; 0 is the
  front */
  virtual yy::Parser::symbol_type& lookAhead(size_t pos);
  yy::Parser::symbol_type& front() { return lookAhead(0); }

private:
  /** Circular buffer of lookahead tokens, front token at index
  m_frontRawIndex, then come m_lookAheadCnt number of tokens, the rest
  of m_buf is free. */
  std::array<yy::Parser::symbol_type, 2> m_buf;
  size_t m_lookAheadCnt;
  size_t m_frontRawIndex;
  TokenStream& m_input;
};
