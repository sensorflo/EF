#pragma once
#include "../driver.h"
#include "../env.h"

#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <string>

/** Creates a temporary file with the given content. The file is removed in
the destructor. */
class TmpFile {
public:
  TmpFile(const std::string& content) {
    // todo: make use of YY_INPUT so that the scanner also can read directly
    // from memory instead from files via yyin
    memset(m_fileName, 'X', sizeof(m_fileName) - 1);
    m_fileName[sizeof(m_fileName) - 1] = '\0';
    errno = 0;
    int fd = mkstemp(m_fileName);
    if (!errno) { write(fd, content.c_str(), content.length()); }
    if (!errno) { close(fd); }
    if (errno) {
      std::cerr << __FUNCTION__
                << ": Either of creating / writing-to / closing "
                   "a temporary file failed"
                << strerror(errno);
      exit(1);
    }
  }
  ~TmpFile() { remove(m_fileName); }
  char const* fileName() { return m_fileName; }

private:
  char m_fileName[10]; // at least 7 (6 needed by mkstemp + 1 for '\0')
};

class TestingDriver : public Driver {
public:
  TestingDriver(const std::string& fileName = "",
    std::basic_ostream<char>* ostream = nullptr)
    : Driver(fileName, ostream){};

  using Driver::m_errorHandler;
  using Driver::m_env;
};

/** Wrapps a Driver which operates on a temporary file with the content given
in the constructor. */
class DriverOnTmpFile {
public:
  DriverOnTmpFile(
    const std::string& content, std::basic_ostream<char>* ostream = nullptr)
    : m_tmpFile(content), m_driver(m_tmpFile.fileName(), ostream){};
  operator TestingDriver&() { return m_driver; }
  TestingDriver& d() { return m_driver; }
  Scanner& scanner() { return m_driver.scanner(); }

private:
  TmpFile m_tmpFile;
  TestingDriver m_driver;
};
