#include "tokenfilter.h"
using namespace yy;

TokenFilter::TokenFilter(TokenStream& input) :
  m_input(input),
  m_bIsStartOfStream(true) {
}

Parser::symbol_type TokenFilter::pop() {
  if ( m_bIsStartOfStream ) {
    while (m_input.front().token() == Parser::token::TOK_NEWLINE) {
      m_input.pop();
    }
    m_bIsStartOfStream = false;
  }
  switch (ParserApiExt::tokenClass(m_input.front().token())) {

  case ParserApiExt::TKSeparator: // fall trough
  case ParserApiExt::TKStarter: {
    auto ret = m_input.pop();
    while (m_input.front().token() == Parser::token::TOK_NEWLINE) {
      m_input.pop();
    }
    return ret;
  }

  case ParserApiExt::TKNewline: {
    auto newlineToken = m_input.pop();
    while (m_input.front().token() == Parser::token::TOK_NEWLINE) {
      m_input.pop();
    }
    switch (ParserApiExt::tokenClass(m_input.front().token())) {
    case ParserApiExt::TKStarter: // fall trough
    case ParserApiExt::TKComponentOrAmbigous: return newlineToken;
    case ParserApiExt::TKDelimiter: return m_input.pop(); // i.e. drop newline
    case ParserApiExt::TKNewline: assert(false); return newlineToken;
    case ParserApiExt::TKSeparator: {
      auto ret = m_input.pop();
      while (m_input.front().token() == Parser::token::TOK_NEWLINE) {
        m_input.pop();
      }
      return ret;
    }
    default: assert(false); return newlineToken;
    }
  }

  case ParserApiExt::TKDelimiter:
  case ParserApiExt::TKComponentOrAmbigous:
    return m_input.pop();
  }
  assert(false);
  return {};
}
