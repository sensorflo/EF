#ifndef ERRORHANDLER_H
#define ERRORHANDLER_H
#include <list>
#include <ostream>

class Error {
public:
  enum No {
    eNone,
    eUnknownName,
    eIncompatibleRedaclaration,
    eRedefinition,
    eWriteToImmutable,
    eNoImplicitConversion,
    eInvalidArguments,
    eNoSuchMember,
    eCnt
  };

  Error(No no) : m_no(no) {}
  No no() const { return m_no; }

private:
  No m_no;
};

const char* toStr(Error::No no);

std::ostream& operator<<(std::ostream& os, Error::No no);

std::ostream& operator<<(std::ostream& os, const Error& error);

class ErrorHandler {
public:
  typedef std::list<Error*> Container;

  virtual ~ErrorHandler();

  /** Errorhandler overtakes ownership */
  void add(Error* error) { m_errors.push_back(error); }
  const Container& errors() const { return m_errors; }
  bool hasNoErrors() const { return m_errors.empty(); }

private:
  /** We're the owner of the pointees */
  Container m_errors;
};

std::ostream& operator<<(std::ostream& os, const ErrorHandler& errorHandler);

/** ErrorHandler contains the details. */
class BuildError {};

#endif
