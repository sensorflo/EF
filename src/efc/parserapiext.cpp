#include "parserapiext.h"
#include <map>

using namespace std;
using namespace yy;

vector<const char*> ParserApiExt::m_TokenNames;
vector<char> ParserApiExt::m_OneCharTokenNames(256*2); // 256 * (char+'\0')

const char* ParserApiExt::tokenName(Parser::token_type t) {
  initTokenNames();
  return m_TokenNames.at(t);
}

/** Is not required to be called explicitely. But it helps to dedect errors
earlier. */
void ParserApiExt::initTokenNames() {
  if (!m_TokenNames.empty()) {
    return;
  }
  m_TokenNames.resize(Parser::token::TOK_TOKENLISTEND, nullptr);
  for (int i=0; i<256; ++i) {
    m_OneCharTokenNames.at(i*2  ) = char(i);
    m_OneCharTokenNames.at(i*2+1) = '\0';
    m_TokenNames.at(i) = &m_OneCharTokenNames.at(i*2);
  }
  const map<Parser::token::yytokentype, const char* const> m {
    { Parser::token::TOK_END_OF_FILE, "END_OF_FILE"},
    { Parser::token::TOK_END, "END"},
    { Parser::token::TOK_DECL, "DECL"},
    { Parser::token::TOK_IF, "IF"},
    { Parser::token::TOK_ELIF, "ELIF"},
    { Parser::token::TOK_ELSE, "ELSE"},
    { Parser::token::TOK_WHILE, "WHILE"},
    { Parser::token::TOK_FUN, "FUN"},
    { Parser::token::TOK_VAL, "VAL"},
    { Parser::token::TOK_VAR, "VAR"},
    { Parser::token::TOK_RAW_NEW, "RAW_NEW"},
    { Parser::token::TOK_RAW_DELETE, "RAW_DELETE"},
    { Parser::token::TOK_NOP, "NOP"},
    { Parser::token::TOK_RETURN, "RETURN"},
    { Parser::token::TOK_EQUAL, "EQUAL"},
    { Parser::token::TOK_DOT_EQUAL, "DOT_EQUAL"},
    { Parser::token::TOK_COLON_EQUAL, "COLON_EQUAL"},
    { Parser::token::TOK_COMMA, "COMMA"},
    { Parser::token::TOK_SEMICOLON, "SEMICOLON"},
    { Parser::token::TOK_DOLLAR, "DOLLAR"},
    { Parser::token::TOK_COLON, "COLON"},
    { Parser::token::TOK_PLUS, "PLUS"},
    { Parser::token::TOK_MINUS, "MINUS"},
    { Parser::token::TOK_STAR, "STAR"},
    { Parser::token::TOK_CARET, "CARET"},
    { Parser::token::TOK_AMPER, "AMPER"},
    { Parser::token::TOK_NOT, "NOT"},
    { Parser::token::TOK_EXCL, "EXCL"},
    { Parser::token::TOK_AND, "AND"},
    { Parser::token::TOK_AMPER_AMPER, "AMPER_AMPER"},
    { Parser::token::TOK_OR, "OR"},
    { Parser::token::TOK_PIPE_PIPE, "PIPE_PIPE"},
    { Parser::token::TOK_EQUAL_EQUAL, "EQUAL_EQUAL"},
    { Parser::token::TOK_SLASH, "SLASH"},
    { Parser::token::TOK_G_LPAREN, "G_LPAREN"},
    { Parser::token::TOK_LPAREN, "LPAREN"},
    { Parser::token::TOK_RPAREN, "RPAREN"},
    { Parser::token::TOK_LBRACE, "LBRACE"},
    { Parser::token::TOK_RBRACE, "RBRACE"},
    { Parser::token::TOK_ARROW, "ARROW"},
    { Parser::token::TOK_FUNDAMENTAL_TYPE, "FUNDAMENTAL_TYPE"},
    { Parser::token::TOK_OP_NAME, "OP_NAME"},
    { Parser::token::TOK_ID, "ID"},
    { Parser::token::TOK_NUMBER, "NUMBER"},
    { Parser::token::TOK_ASSIGNEMENT, "ASSIGNEMENT"}};
  for (const auto kv: m) {
    m_TokenNames.at(kv.first) = kv.second;
  }
  for (int i=256; i<=Parser::token::TOK_TOKENLISTSTART; ++i) {
    m_TokenNames.at(i) = "<UNUSED_BY_BISON>";
  }
  for (int i=0; i<Parser::token::TOK_TOKENLISTEND; ++i) {
    if ( !m_TokenNames.at(i) ) {
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
