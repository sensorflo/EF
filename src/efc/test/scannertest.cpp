#include "../gensrc/parser.hpp"
#include "../driver.h"
#include "gtest/gtest.h"
#include <string>
#include <fstream>
using namespace testing;
using namespace std;
using namespace yy;

/** Creates a temporary file with the given content. The file is removed in
the destructor. Only one instance, i.e. only one temp file, can exist at a
time. This is good enough and enables having a static buffer for the file name
which enables being allowed to call tmpnam only once. */
class TmpFile {
public:
  TmpFile(const string& content) {
    if (m_instanceCount++) { throw "more than one TmpFile instance"; } 
    static bool isFileNameInit = false;
    if (!isFileNameInit) { tmpnam(m_fileName); };
    ofstream tmpFile(m_fileName);
    tmpFile << content;
  }
  ~TmpFile() { remove( m_fileName ); --m_instanceCount; }
  char const*  fileName() { return m_fileName; }
private:
  static char m_fileName[L_tmpnam];
  static int m_instanceCount;
};

char TmpFile::m_fileName[L_tmpnam];
int TmpFile::m_instanceCount = 0;

/** Creates a driver operaring on a temporary file with the content given in
the constructor. */
class DriverOnTmpFile {
public:
  DriverOnTmpFile( const string& content ) :
    m_tmpFile(content),
    m_driver(m_tmpFile.fileName()) {};
  operator Driver&() { return m_driver; }
private:
  TmpFile m_tmpFile;
  Driver m_driver;
};

TEST(ScannerTest, empty) {
  DriverOnTmpFile driver( "" );
  EXPECT_EQ(Parser::token::TOK_END_OF_FILE, yylex(driver).token() );
}

TEST(ScannerTest, id) {
  DriverOnTmpFile driver( "ifelse" );
  EXPECT_EQ(Parser::token::TOK_ID, yylex(driver).token() );
  EXPECT_EQ(Parser::token::TOK_END_OF_FILE, yylex(driver).token() );
}

TEST(ScannerTest, if_else) {
  // ef.l currently cannot handle keywords at the very end of a file, thus the
  // trailing blank
  DriverOnTmpFile driver( "if else " );
  EXPECT_EQ(Parser::token::TOK_IF, yylex(driver).token() );
  EXPECT_EQ(Parser::token::TOK_ELSE, yylex(driver).token() );
  EXPECT_EQ(Parser::token::TOK_END_OF_FILE, yylex(driver).token() );
}

TEST(ScannerTest, number) {
  {
    // ef.l currently cannot handle numbers at the very end of a file, thus
    // the trailing blank
    DriverOnTmpFile driver( "42 " );
    EXPECT_EQ(Parser::token::TOK_NUMBER, yylex(driver).token() );
    EXPECT_EQ(Parser::token::TOK_END_OF_FILE, yylex(driver).token() );
  }

  {
    DriverOnTmpFile driver( "42ll " );
    EXPECT_EQ(Parser::token::TOK_NUMBER, yylex(driver).token() );
    EXPECT_EQ(Parser::token::TOK_END_OF_FILE, yylex(driver).token() );
  }

  {
    DriverOnTmpFile driver( "42if " );
    EXPECT_EQ(Parser::token::TOK_NUMBER, yylex(driver).token() );
    EXPECT_EQ(Parser::token::TOK_END_OF_FILE, yylex(driver).token() );
  }

  {
    DriverOnTmpFile driver( "42" );
    Parser::symbol_type st = yylex(driver);
    EXPECT_EQ(42, st.value.as<int>());
  }
}
