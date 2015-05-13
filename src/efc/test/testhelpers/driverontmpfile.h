#pragma once
#include "../driver.h"
#include "../env.h"
#include <string>
#include <fstream>
#include <errno.h>
#include <stdlib.h>

/** Creates a temporary file with the given content. The file is removed in
the destructor. */
class TmpFile {
public:
  TmpFile(const std::string& content) {
    // todo: make use of YY_INPUT so that the scanner also can read directly
    // from memory instead from files via yyin
    memset(m_fileName, 'X', sizeof(m_fileName)-1);
    m_fileName[sizeof(m_fileName)-1] = '\0';
    errno = 0;
    int fd = mkstemp(m_fileName);
    if (!errno) { write(fd, content.c_str(), content.length()); }
    if (!errno) { close(fd); }
    if (errno) {
      std::cerr << __FUNCTION__ << ": Either of creating / writing-to / closing "
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
  DriverOnTmpFile(const std::string& content, std::basic_ostream<char>* ostream = NULL) :
    m_tmpFile(content),
    m_driver(m_tmpFile.fileName(), ostream) {};
  operator Driver&() { return m_driver; }
  Driver& d() { return m_driver; }
  Scanner& scanner() { return m_driver.scanner(); }
private:
  TmpFile m_tmpFile;
  Driver m_driver;
};
