#pragma once
#include "declutils.h"
#include "location.h"

#include <array>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

class ErrorHandler;

/** See also BuildError */
class Error final {
public:
  enum No {
    eNone,
    eInternalError,
    eUnknownIntegralLiteralSuffix,
    eLiteralOutOfValidRange,
    eUnknownName,
    eRedefinition,
    eWriteToImmutable,
    eCantOpenFileForReading,
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
    eTypeInferenceIsNotYetSupported,
    eNonIgnoreAccessToLocalDataObjectBeforeItsInitialization,
    eScanOrParseFailed,
    eCnt
  };

  /** Unless given error is disabled, adds a new Error to ErrorHandler and
  throws an according BuildError. */
  static void throwError(ErrorHandler& errorHandler, No no,
    std::string additionalMsg = "", Location loc = s_nullLoc);
  static void throwError(ErrorHandler& errorHandler, No no, Location loc);

  No no() const { return m_no; }
  const std::string& message() const { return m_message; }

private:
  NEITHER_COPY_NOR_MOVEABLE(Error);

  Error(No no, std::string message);

  const No m_no;
  const std::string m_message;
};

const char* toStr(Error::No no);

std::ostream& operator<<(std::ostream& os, Error::No no);

std::ostream& operator<<(std::ostream& os, const Error& error);

class ErrorHandler final {
public:
  using Container = std::vector<std::shared_ptr<Error>>;

  ErrorHandler();
  ~ErrorHandler();

  void add(std::shared_ptr<Error> error);
  const Container& errors() const { return m_errors; }
  bool hasNoErrors() const { return m_errors.empty(); }
  void disableReportingOf(Error::No no);
  bool isReportingDisabledFor(Error::No no) const;

private:
  NEITHER_COPY_NOR_MOVEABLE(ErrorHandler);

  /** The ptrs are guaranteed to be non-nullptr */
  Container m_errors;
  std::array<bool, Error::eCnt> m_disabledErrors;
};

std::ostream& operator<<(std::ostream& os, const ErrorHandler& errorHandler);

/** Wrapper arround Error so we can throw by value.

Rational for not throwing directly Error but inventing a new class which is used
to throw: We could throw a smart pointer to Error, but that does not work well
together with gtest in the case of an test throwing unexpectetly. gtest can't
know all the types a test probably will throw, and the most general thing it can
catch which still provides some information is std::exception */
class BuildError : public std::logic_error {
public:
  ~BuildError() override;

  const Error& error() const { return *m_error; }

  // convenience methods
  Error::No no() const { return m_error->no(); }
  const std::string& message() const { return m_error->message(); }
  const char* what() const noexcept override {
    return m_error->message().c_str();
  }

private:
  friend class Error;
  BuildError(std::shared_ptr<const Error> error);

  std::shared_ptr<const Error> m_error;
};

std::ostream& operator<<(std::ostream& os, const BuildError& error);

inline std::ostream& operator<<(
  std::ostream& os, const std::shared_ptr<Error>& error) {
  // to do: nullptr check
  os << *error;
  return os;
}
