#include "errorhandler.h"

#include <sstream>
#include <string>
#include <utility>

using namespace std;

Error::Error(Error::No no, string message)
  : m_no(no), m_message(move(message)) {
}

void Error::throwError(
  ErrorHandler& errorHandler, No no, string additionalMsg, Location loc) {
  if (!errorHandler.isReportingDisabledFor(no)) {
    stringstream ss;
    if (loc.begin.m_fileName) { ss << *loc.begin.m_fileName << ":"; }
    ss << loc.begin.m_line << ":" << loc.begin.m_column << ": error "
       << additionalMsg << "[" << no << "]";

    auto error = shared_ptr<Error>(new Error{no, ss.str()});
    errorHandler.add(error);
    throw BuildError(error);
  }
}

void Error::throwError(ErrorHandler& errorHandler, No no, Location loc) {
  throwError(errorHandler, no, "", loc);
}

const char* toStr(Error::No no) {
  switch (no) {
    // clang-format off
  case Error::eNone: return "eNone";
  case Error::eInternalError: return "eInternalError";
  case Error::eUnknownIntegralLiteralSuffix: return "eUnknownIntegralLiteralSuffix";
  case Error::eLiteralOutOfValidRange: return "eLiteralOutOfValidRange";
  case Error::eUnknownName: return "eUnknownName";
  case Error::eRedefinition: return "eRedefinition";
  case Error::eWriteToImmutable: return "eWriteToImmutable";
  case Error::eCantOpenFileForReading: return "eCantOpenFileForReading";
  case Error::eNoImplicitConversion: return "eNoImplicitConversion";
  case Error::eInvalidArguments: return "eInvalidArguments";
  case Error::eNoSuchMember: return "eNoSuchMember";
  case Error::eNotInFunBodyContext: return "eNotInFunBodyContext";
  case Error::eUnreachableCode: return "eUnreachableCode";
  case Error::eCTConstRequired: return "eCTConstRequired";
  case Error::eRetTypeCantHaveMutQualifier: return "eRetTypeCantHaveMutQualifier";
  case Error::eSameArgWasDefinedMultipleTimes: return "eSameArgWasDefinedMultipleTimes";
  case Error::eObjectExpected: return "eObjectExpected";
  case Error::eScanOrParseFailed: return "eScanOrParseFailed";
  case Error::eOnlyLocalStorageDurationApplicable: return "eOnlyLocalStorageDurationApplicable";
  case Error::eTypeInferenceIsNotYetSupported: return "eTypeInferenceIsNotYetSupported";
  case Error::eNonIgnoreAccessToLocalDataObjectBeforeItsInitialization: return "eNonIgnoreAccessToLocalDataObjectBeforeItsInitialization";
  case Error::eCnt: return "<unknown>";
    // clang-format on
  }
  return "<unknown>";
}

ostream& operator<<(ostream& os, Error::No no) {
  return os << toStr(no);
}

ostream& operator<<(ostream& os, const Error& error) {
  return os << error.message();
}

ErrorHandler::ErrorHandler() : m_disabledErrors{} {
}

void ErrorHandler::add(shared_ptr<Error> error) {
  m_errors.emplace_back(move(error));
}

void ErrorHandler::disableReportingOf(Error::No no) {
  m_disabledErrors[no] = true;
}

bool ErrorHandler::isReportingDisabledFor(Error::No no) const {
  return m_disabledErrors[no];
}

ErrorHandler::~ErrorHandler() = default;

ostream& operator<<(ostream& os, const ErrorHandler& errorHandler) {
  os << "{";
  auto isFirstIter = true;
  for (const auto& e : errorHandler.errors()) {
    if (!isFirstIter) {
      os << ", ";
      isFirstIter = false;
    }
    os << e->no();
  }
  return os << "}";
}

BuildError::BuildError(std::shared_ptr<const Error> error)
  : logic_error{error->message()}, m_error{error} {
}

BuildError::~BuildError() = default;

std::ostream& operator<<(std::ostream& os, const BuildError& error) {
  os << error.error();
  return os;
}
