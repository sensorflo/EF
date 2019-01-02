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
    eNoSuchCtor,
    eNoSuchMemberFun,
    eNotInFunBodyContext,
    eUnreachableCode,
    eCTConstRequired,
    eRetTypeCantHaveMutQualifier,
    eMultipleInitializers,
    eObjectExpected,
    eOnlyLocalStorageDurationApplicable,
    eTypeInferenceIsNotYetSupported,
    eNonIgnoreAccessToLocalDataObjectBeforeItsInitialization,
    eParseFailed,
    eUnexpectedCharacter,
    eCnt
  };

  /** Unless given error is disabled, adds a new Error to ErrorHandler and
  throws an according BuildError. Regarding msgParamX, see method describe. */
  static void throwError(ErrorHandler& errorHandler, No no,
    Location loc = s_nullLoc, std::string msgParam1 = "",
    std::string msgParam2 = "", std::string msgParam3 = "");

  No no() const { return m_no; }
  const std::string& message() const { return m_message; }

  /** Returns a verbose description of the error. */
  static std::string describe(Error::No no, const std::string& msgParam1 = "",
    const std::string& msgParam2 = "", const std::string& msgParam3 = "");

  const std::string& msgParam1() const { return m_msgParam1; }
  const std::string& msgParam2() const { return m_msgParam2; }
  const std::string& msgParam3() const { return m_msgParam3; }

private:
  NEITHER_COPY_NOR_MOVEABLE(Error);

  Error(No no, std::string message, std::string msgParam1 = "",
    std::string msgParam2 = "", std::string msgParam3 = "");

  const No m_no;
  const std::string m_message;

  // m_message already contains the msg params, see the describe method, but for
  // automated tests it is convenient to access them explicitly individually.
  const std::string m_msgParam1;
  const std::string m_msgParam2;
  const std::string m_msgParam3;
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
  bool hasErrors() const { return !m_errors.empty(); }
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
