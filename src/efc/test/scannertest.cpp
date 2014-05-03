#include "test.h"
#include "../gensrc/parser.hpp"
#include "../driver.h"
#include <string>
#include <fstream>
#include <errno.h>
#include <stdlib.h>
using namespace testing;
using namespace std;
using namespace yy;

/** Creates a temporary file with the given content. The file is removed in
the destructor. */
class TmpFile {
public:
  TmpFile(const string& content) {
    memset(m_fileName, 'X', sizeof(m_fileName)-1);
    m_fileName[sizeof(m_fileName)-1] = '\0';
    errno = 0;
    int fd = mkstemp(m_fileName);
    if (!errno) { write(fd, content.c_str(), content.length()); }
    if (!errno) { close(fd); }
    if (errno) {
      cerr << __FUNCTION__ << ": Either of creating / writing-to / closing "
        "a temporary file failed" << strerror(errno);
      exit(1);
    }
  }
  ~TmpFile() { remove( m_fileName ); }
  char const*  fileName() { return m_fileName; }
private:
  char m_fileName[10]; // at least 7 (6 needed by mkstemp + 1 for '\0')
};


/** Wrapps a Driver which operates on a temporary file with the content given
in the constructor. */
class DriverOnTmpFile {
public:
  DriverOnTmpFile( const string& content ) :
    m_tmpFile(content),
    m_driver(m_tmpFile.fileName()) {};
  operator Driver&() { return m_driver; }
  Driver& d() { return m_driver; }
private:
  TmpFile m_tmpFile;
  Driver m_driver;
};

TEST(ScannerTest, MAKE_TEST_NAME(
    an_empty_file,
    yylex,
    returns_TOK_END_OF_FILE_AND_succeeds) ) {
  DriverOnTmpFile driver( "" );
  EXPECT_EQ(Parser::token::TOK_END_OF_FILE, yylex(driver).token() );
  EXPECT_FALSE( driver.d().m_gotError );
}

TEST(ScannerTest, MAKE_TEST_NAME(
    concatenated_keywords,
    yylex,
    returns_TOK_ID_AND_succeeds)) {
  DriverOnTmpFile driver( "ifelse" );
  EXPECT_EQ(Parser::token::TOK_ID, yylex(driver).token() );
  EXPECT_FALSE( driver.d().m_gotError );
}

TEST(ScannerTest, MAKE_TEST_NAME(
    keywords_separated_by_blanks,
    yylex_is_called_repeatedly,
    returns_the_keyword_tokens_AND_succeeds)) {
  // ef.l currently cannot handle keywords at the very end of a file, thus the
  // trailing blank
  DriverOnTmpFile driver( "if else " );
  EXPECT_EQ(Parser::token::TOK_IF, yylex(driver).token() );
  EXPECT_EQ(Parser::token::TOK_ELSE, yylex(driver).token() );
  EXPECT_FALSE( driver.d().m_gotError );
}

TEST(ScannerTest, number) {
  {
    // ef.l currently cannot handle numbers at the very end of a file, thus
    // the trailing blank
    DriverOnTmpFile driver( "42 " );
    EXPECT_EQ(Parser::token::TOK_NUMBER, yylex(driver).token() );
    EXPECT_EQ(Parser::token::TOK_END_OF_FILE, yylex(driver).token() );
    EXPECT_FALSE( driver.d().m_gotError );
  }

  {
    DriverOnTmpFile driver( "42if " );
    EXPECT_EQ(Parser::token::TOK_NUMBER, yylex(driver).token() );
    EXPECT_TRUE( driver.d().m_gotError );
    EXPECT_EQ(Parser::token::TOK_END_OF_FILE, yylex(driver).token() );
  }

  {
    DriverOnTmpFile driver( "flo 42 " );
    EXPECT_EQ(Parser::token::TOK_ID, yylex(driver).token() );
    EXPECT_EQ(Parser::token::TOK_NUMBER, yylex(driver).token() );
    EXPECT_EQ(Parser::token::TOK_END_OF_FILE, yylex(driver).token() );
    EXPECT_FALSE( driver.d().m_gotError );
  }

  {
    DriverOnTmpFile driver( "42" );
    Parser::symbol_type st = yylex(driver);
    EXPECT_EQ(42, st.value.as<int>());
  }
}
