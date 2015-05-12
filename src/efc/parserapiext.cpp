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

ParserApiExt::TokenTypeAttr::TokenTypeAttr(const char* name,
  SemanticValueType svt, TokenClass tc) :
  m_name{name},
  m_semanticValueType{svt},
  m_tokenClass{tc} {
}

const char* ParserApiExt::tokenName(Parser::token_type t) {
  initTokenAttrs();
  return m_TokenAttrs.at(t).m_name;
}

ParserApiExt::TokenClass ParserApiExt::tokenClass(Parser::token_type t) {
  initTokenAttrs();
  return m_TokenAttrs.at(t).m_tokenClass;
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
    { Parser::token::TOK_END_OF_FILE, {"END_OF_FILE", SVTVoid, TKDelimiter}},
    { Parser::token::TOK_END, {"END", SVTVoid, TKDelimiter}},
    { Parser::token::TOK_DECL, {"DECL", SVTVoid, TKStarter}},
    { Parser::token::TOK_IF, {"IF", SVTVoid, TKStarter}},
    { Parser::token::TOK_ELIF, {"ELIF", SVTVoid, TKSeparator}},
    { Parser::token::TOK_ELSE, {"ELSE", SVTVoid, TKSeparator}},
    { Parser::token::TOK_WHILE, {"WHILE", SVTVoid, TKStarter}},
    { Parser::token::TOK_FUN, {"FUN", SVTVoid, TKStarter}},
    { Parser::token::TOK_VAL, {"VAL", SVTVoid, TKStarter}},
    { Parser::token::TOK_VAR, {"VAR", SVTVoid, TKStarter}},
    { Parser::token::TOK_RAW_NEW, {"RAW_NEW", SVTVoid, TKStarter}},
    { Parser::token::TOK_RAW_DELETE, {"RAW_DELETE", SVTVoid, TKStarter}},
    { Parser::token::TOK_NOP, {"NOP", SVTVoid, TKComponentOrAmbigous}},
    { Parser::token::TOK_RETURN, {"RETURN", SVTVoid, TKStarter}},
    { Parser::token::TOK_EQUAL, {"EQUAL", SVTVoid, TKComponentOrAmbigous}},
    { Parser::token::TOK_DOT_EQUAL, {"DOT_EQUAL", SVTVoid, TKComponentOrAmbigous}},
    { Parser::token::TOK_COLON_EQUAL, {"COLON_EQUAL", SVTVoid, TKComponentOrAmbigous}},
    { Parser::token::TOK_COMMA, {"COMMA", SVTVoid, TKSeparator}},
    { Parser::token::TOK_SEMICOLON, {"SEMICOLON", SVTVoid, TKSeparator}},
    { Parser::token::TOK_NEWLINE, {"NEWLINE", SVTVoid, TKNewline}},
    { Parser::token::TOK_DOLLAR, {"DOLLAR", SVTVoid, TKDelimiter}},
    { Parser::token::TOK_COLON, {"COLON", SVTVoid, TKSeparator}},
    { Parser::token::TOK_PLUS, {"PLUS", SVTVoid, TKComponentOrAmbigous}},
    { Parser::token::TOK_MINUS, {"MINUS", SVTVoid, TKComponentOrAmbigous}},
    { Parser::token::TOK_STAR, {"STAR", SVTVoid, TKComponentOrAmbigous}},
    { Parser::token::TOK_CARET, {"CARET", SVTVoid, TKComponentOrAmbigous}},
    { Parser::token::TOK_AMPER, {"AMPER", SVTVoid, TKComponentOrAmbigous}},
    { Parser::token::TOK_NOT, {"NOT", SVTVoid, TKComponentOrAmbigous}},
    { Parser::token::TOK_EXCL, {"EXCL", SVTVoid, TKComponentOrAmbigous}},
    { Parser::token::TOK_AND, {"AND", SVTVoid, TKComponentOrAmbigous}},
    { Parser::token::TOK_AMPER_AMPER, {"AMPER_AMPER", SVTVoid, TKComponentOrAmbigous}},
    { Parser::token::TOK_OR, {"OR", SVTVoid, TKComponentOrAmbigous}},
    { Parser::token::TOK_PIPE_PIPE, {"PIPE_PIPE", SVTVoid, TKComponentOrAmbigous}},
    { Parser::token::TOK_EQUAL_EQUAL, {"EQUAL_EQUAL", SVTVoid, TKComponentOrAmbigous}},
    { Parser::token::TOK_COMMA_EQUAL, {"COMMA_EQUAL", SVTVoid, TKSeparator}},
    { Parser::token::TOK_LPAREN_EQUAL, {"LPAREN_LPAREN", SVTVoid, TKSeparator}},
    { Parser::token::TOK_SLASH, {"SLASH", SVTVoid, TKComponentOrAmbigous}},
    { Parser::token::TOK_G_LPAREN, {"G_LPAREN", SVTVoid, TKStarter}},
    { Parser::token::TOK_LPAREN, {"LPAREN", SVTVoid, TKStarter}},
    { Parser::token::TOK_RPAREN, {"RPAREN", SVTVoid, TKDelimiter}},
    { Parser::token::TOK_LBRACE, {"LBRACE", SVTVoid, TKStarter}},
    { Parser::token::TOK_RBRACE, {"RBRACE", SVTVoid, TKDelimiter}},
    { Parser::token::TOK_ARROW, {"ARROW", SVTVoid, TKComponentOrAmbigous}},
    { Parser::token::TOK_FUNDAMENTAL_TYPE, {"FUNDAMENTAL_TYPE", SVTFundamentalType, TKComponentOrAmbigous}},
    { Parser::token::TOK_OP_NAME, {"OP_NAME", SVTString, TKStarter}},
    { Parser::token::TOK_ID, {"ID", SVTString, TKComponentOrAmbigous}},
    { Parser::token::TOK_NUMBER, {"NUMBER", SVTNumberToken, TKComponentOrAmbigous}}};
  for (const auto kv: m) {
    m_TokenAttrs.at(kv.first) = kv.second;
  }
  for (int i=256; i<=Parser::token::TOK_TOKENLISTSTART; ++i) {
    m_TokenAttrs.at(i) = TokenTypeAttr{"<UNUSED_BY_BISON>", SVTInvalid,
                                       TKComponentOrAmbigous};
  }
  // not a 'real' token, only used for %precedence
  m_TokenAttrs.at(Parser::token::TOK_ASSIGNEMENT) =
    TokenTypeAttr{"<UNUSED_BY_BISON>", SVTInvalid, TKComponentOrAmbigous};
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
