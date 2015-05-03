#include "tokenfilter.h"
using namespace yy;

TokenFilterX::TokenFilterX(TokenStream& input) :
  m_input(input) {
}

Parser::symbol_type& TokenFilterX::A() {
  return m_input.front();
}

Parser::symbol_type TokenFilterX::B() {
  // while (m_input.front().token()==Parser::token::TOK_ELSE) {
  //   m_input.pop();
  // }
  return m_input.pop();
}

// Parser::symbol_type& TokenFilter::front() {
//   assert(false);
//   return m_input.front();
// }

// Parser::symbol_type TokenFilter::pop() {
//   assert(false);
//   while (m_input.front().token()==Parser::token::TOK_ELSE) {
//     m_input.pop();
//   }
//   return m_input.pop();
// }

