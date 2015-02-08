#ifndef ERRORHANDLER_H
#define ERRORHANDLER_H
#include <list>

class Error {
public:
  enum No {
    eNone,
    eUnknownName,
    eIncompatibleRedaclaration,
    eCnt
  };

  Error(No no) : m_no(no) {}
  No no() const { return m_no; }

private:
  No m_no;
};

class ErrorHandler {
public:
  typedef std::list<Error*> Container;

  virtual ~ErrorHandler();

  /** Errorhandler overtakes ownership */
  void add(Error* error) { m_errors.push_back(error); }
  const Container& errors() { return m_errors; }

private:
  /** We're the owner of the pointees */
  Container m_errors;
};

/** ErrorHandler contains the details. */
class BuildError {};

#endif
