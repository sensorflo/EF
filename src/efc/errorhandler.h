#pragma once
#include "declutils.h"
#include <list>
#include <ostream>
#include <stdexcept>
#include <array>

class ErrorHandler;

/** See also BuildError */
class Error final {
public:
  enum No {
    eNone,
    eUnknownName,
    eRedefinition,
    eWriteToImmutable,
    eNoImplicitConversion,
    eInvalidArguments,
    eNoSuchMember,
    eNotInFunBodyContext,
    eUnreachableCode,
    eCTConstRequired,
    eRetTypeCantHaveMutQualifier,
    eSameArgWasDefinedMultipleTimes,
    eObjectExpected,
    eOnlyLocalStorageDurationApplicable,
    eNonIgnoreAccessToLocalDataObjectBeforeItsInitialization,
    eCnt
  };

  ~Error() = default;

  /** Unless given error is disabled, adds a new Error to ErrorHandler and
  throws an according BuildError. */
  static void throwError(ErrorHandler& errorHandler, No no,
    std::string additionalMsg = "");

  No no() const { return m_no; }

private:
  NEITHER_COPY_NOR_MOVEABLE(Error);

  Error(No no);
  No m_no;
};

const char* toStr(Error::No no);

std::ostream& operator<<(std::ostream& os, Error::No no);

std::ostream& operator<<(std::ostream& os, const Error& error);

class ErrorHandler final {
public:
  typedef std::vector<Error*> Container;

  ErrorHandler();
  ~ErrorHandler();

  /** Errorhandler overtakes ownership */
  void add(Error* error) { m_errors.push_back(error); }
  const Container& errors() const { return m_errors; }
  bool hasNoErrors() const { return m_errors.empty(); }
  void disableReportingOf(Error::No no);
  bool isReportingDisabledFor(Error::No no) const;

private:
  NEITHER_COPY_NOR_MOVEABLE(ErrorHandler);

  /** We're the owner of the pointees */
  Container m_errors;
  std::array<bool,Error::eCnt> m_disabledErrors;
};

std::ostream& operator<<(std::ostream& os, const ErrorHandler& errorHandler);

/** Class used to throw by value via Error::throwError and be catched by
reference. The 'what' description is redundant to the associated Error
instance and is only intended to be used if an instance of BuildError is
catched at an wide outside safety net catch, i.e. at a place where controlled
error handling with proper counter actions is no longer possible and the only
option is to print as mutch info as we can and then probably die.

Rational for not throwing Error directly but inventing a new class which is
used to throw: Exceptions should be thrown by value, thus can't be owned by
anybode. But ErrorHandler owns its exceptions. We could throw pointers to
Error instances, but that does not work well together with gtest in the case
of an test throwing unexpectetly. gtest writes the .what of an unexpected
exception only if it is thrown by value. */
class BuildError : public std::logic_error {
private:
  friend class Error;
  BuildError(const std::string& what);
};
