#include "tokenfilter.h"

#include "parser.h"

using namespace yy;

TokenFilter::TokenFilter(TokenStream& input)
  : m_input{input}, m_bIsStartOfStream{true} {
}

Parser::symbol_type TokenFilter::pop() {
  if (m_bIsStartOfStream) {
    while (m_input.front().token() == Parser::token::TOK_NEWLINE) {
      m_input.pop();
    }
    m_bIsStartOfStream = false;
  }
  switch (Parser::tokenClass(m_input.front().token())) {
  case Parser::TKSeparator: // fall trough
  case Parser::TKStarter: {
    auto ret = m_input.pop();
    while (m_input.front().token() == Parser::token::TOK_NEWLINE) {
      m_input.pop();
    }
    return ret;
  }

  case Parser::TKNewline: {
    auto newlineToken = m_input.pop();
    while (m_input.front().token() == Parser::token::TOK_NEWLINE) {
      m_input.pop();
    }
    switch (Parser::tokenClass(m_input.front().token())) {
    case Parser::TKStarter: // fall trough
    case Parser::TKComponentOrAmbigous: return newlineToken;
    case Parser::TKDelimiter: return m_input.pop(); // i.e. drop newline
    case Parser::TKNewline: assert(false); return newlineToken;
    case Parser::TKSeparator: {
      auto ret = m_input.pop();
      while (m_input.front().token() == Parser::token::TOK_NEWLINE) {
        m_input.pop();
      }
      return ret;
    }
    default: assert(false); return newlineToken;
    }
  }

  case Parser::TKDelimiter:
  case Parser::TKComponentOrAmbigous: return m_input.pop();
  }
  assert(false);
  return {};
}
