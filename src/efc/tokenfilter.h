#pragma once
#include "tokenstream.h"
#include "tokenstreamlookahead.h"
#include <array>

class TokenFilter : public TokenStream {
public:
  TokenFilter(TokenStream& input);

  yy::Parser::symbol_type pop() override;

private:
  TokenStreamLookAhead m_input;
  bool m_bIsStartOfStream;
};
