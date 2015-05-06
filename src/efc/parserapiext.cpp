#include "parserapiext.h"
#include <map>

using namespace std;
using namespace yy;

vector<ParserApiExt::TokenTypeAttr> ParserApiExt::m_TokenAttrs;
vector<char> ParserApiExt::m_OneCharTokenNames(256*2); // 256 * (char+'\0')

ParserApiExt::TokenTypeAttr::TokenTypeAttr() :
  m_name{},
  m_semanticValueType{SVTInvalid} {
}

ParserApiExt::TokenTypeAttr::TokenTypeAttr(const char* name, SemanticValueType svt) :
  m_name{name},
  m_semanticValueType{svt} {
}

const char* ParserApiExt::tokenName(Parser::token_type t) {
  initTokenAttrs();
  return m_TokenAttrs.at(t).m_name;
}

/** Analogous to makeToken */
template<typename T>
Parser::symbol_type ParserApiExt::makeTokenT(Parser::token_type tt) {
  return Parser::symbol_type(tt, T{}, Parser::location_type());
}

/** Returns a new Parser::symbol_type with the semantic value member and the
location member being default initialized */
Parser::symbol_type ParserApiExt::makeToken(Parser::token_type tt) {
  switch(m_TokenAttrs.at(tt).m_semanticValueType) {
  case SVTInvalid: assert(false); // fall through
  case SVTVoid: return Parser::symbol_type(tt, Parser::location_type());
  case SVTFundamentalType: return makeTokenT<ObjTypeFunda::EType>(tt);
  case SVTString: return makeTokenT<string>(tt);
  case SVTNumberToken: return makeTokenT<NumberToken>(tt);
  }
  assert(false);
  return Parser::symbol_type(tt, Parser::location_type());
}

/** Is not required to be called explicitely. But it helps to dedect errors
earlier. */
void ParserApiExt::initTokenAttrs() {
  if (!m_TokenAttrs.empty()) {
    return;
  }
  m_TokenAttrs.resize(Parser::token::TOK_TOKENLISTEND);
  for (int i=0; i<256; ++i) {
    m_OneCharTokenNames.at(i*2  ) = char(i);
    m_OneCharTokenNames.at(i*2+1) = '\0';
    m_TokenAttrs.at(i).m_name = &m_OneCharTokenNames.at(i*2);
    m_TokenAttrs.at(i).m_semanticValueType = SVTVoid;
  }
  const map<Parser::token::yytokentype, TokenTypeAttr> m {
    { Parser::token::TOK_END_OF_FILE, {"END_OF_FILE", SVTVoid}},
    { Parser::token::TOK_END, {"END", SVTVoid}},
    { Parser::token::TOK_DECL, {"DECL", SVTVoid}},
    { Parser::token::TOK_IF, {"IF", SVTVoid}},
    { Parser::token::TOK_ELIF, {"ELIF", SVTVoid}},
    { Parser::token::TOK_ELSE, {"ELSE", SVTVoid}},
    { Parser::token::TOK_WHILE, {"WHILE", SVTVoid}},
    { Parser::token::TOK_FUN, {"FUN", SVTVoid}},
    { Parser::token::TOK_VAL, {"VAL", SVTVoid}},
    { Parser::token::TOK_VAR, {"VAR", SVTVoid}},
    { Parser::token::TOK_RAW_NEW, {"RAW_NEW", SVTVoid}},
    { Parser::token::TOK_RAW_DELETE, {"RAW_DELETE", SVTVoid}},
    { Parser::token::TOK_NOP, {"NOP", SVTVoid}},
    { Parser::token::TOK_RETURN, {"RETURN", SVTVoid}},
    { Parser::token::TOK_EQUAL, {"EQUAL", SVTVoid}},
    { Parser::token::TOK_DOT_EQUAL, {"DOT_EQUAL", SVTVoid}},
    { Parser::token::TOK_COLON_EQUAL, {"COLON_EQUAL", SVTVoid}},
    { Parser::token::TOK_COMMA, {"COMMA", SVTVoid}},
    { Parser::token::TOK_SEMICOLON, {"SEMICOLON", SVTVoid}},
    { Parser::token::TOK_NEWLINE, {"NEWLINE", SVTVoid}},
    { Parser::token::TOK_DOLLAR, {"DOLLAR", SVTVoid}},
    { Parser::token::TOK_COLON, {"COLON", SVTVoid}},
    { Parser::token::TOK_PLUS, {"PLUS", SVTVoid}},
    { Parser::token::TOK_MINUS, {"MINUS", SVTVoid}},
    { Parser::token::TOK_STAR, {"STAR", SVTVoid}},
    { Parser::token::TOK_CARET, {"CARET", SVTVoid}},
    { Parser::token::TOK_AMPER, {"AMPER", SVTVoid}},
    { Parser::token::TOK_NOT, {"NOT", SVTVoid}},
    { Parser::token::TOK_EXCL, {"EXCL", SVTVoid}},
    { Parser::token::TOK_AND, {"AND", SVTVoid}},
    { Parser::token::TOK_AMPER_AMPER, {"AMPER_AMPER", SVTVoid}},
    { Parser::token::TOK_OR, {"OR", SVTVoid}},
    { Parser::token::TOK_PIPE_PIPE, {"PIPE_PIPE", SVTVoid}},
    { Parser::token::TOK_EQUAL_EQUAL, {"EQUAL_EQUAL", SVTVoid}},
    { Parser::token::TOK_SLASH, {"SLASH", SVTVoid}},
    { Parser::token::TOK_G_LPAREN, {"G_LPAREN", SVTVoid}},
    { Parser::token::TOK_LPAREN, {"LPAREN", SVTVoid}},
    { Parser::token::TOK_RPAREN, {"RPAREN", SVTVoid}},
    { Parser::token::TOK_LBRACE, {"LBRACE", SVTVoid}},
    { Parser::token::TOK_RBRACE, {"RBRACE", SVTVoid}},
    { Parser::token::TOK_ARROW, {"ARROW", SVTVoid}},
    { Parser::token::TOK_FUNDAMENTAL_TYPE, {"FUNDAMENTAL_TYPE", SVTFundamentalType}},
    { Parser::token::TOK_OP_NAME, {"OP_NAME", SVTString}},
    { Parser::token::TOK_ID, {"ID", SVTString}},
    { Parser::token::TOK_NUMBER, {"NUMBER", SVTNumberToken}},
    { Parser::token::TOK_ASSIGNEMENT, {"ASSIGNEMENT", SVTVoid}}};
  for (const auto kv: m) {
    m_TokenAttrs.at(kv.first) = kv.second;
  }
  for (int i=256; i<=Parser::token::TOK_TOKENLISTSTART; ++i) {
    m_TokenAttrs.at(i) = TokenTypeAttr{"<UNUSED_BY_BISON>", SVTInvalid};
  }
  for (int i=0; i<Parser::token::TOK_TOKENLISTEND; ++i) {
    if ( !m_TokenAttrs.at(i).m_name ) {
      cerr << "token " << i << " has no been given a name. Update the "
           << "redundant above map with what yy::Parser::token::yytokentype "
           << "defines" << endl;
      assert(false);
    }
  }
}

basic_ostream<char>& yy::operator<<(basic_ostream<char>& os,
  Parser::token_type t) {
  return os << ParserApiExt::tokenName(t);
}

basic_ostream<char>& std::operator<<(basic_ostream<char>& os,
  const vector<Parser::token_type>& tokens) {
  os << "{";
  for (auto i = tokens.begin(); i!=tokens.end(); ++i) {
    if (i!=tokens.begin()) {
      os << ", ";
    }
    os << *i;
  }
  return os << "}";
}
