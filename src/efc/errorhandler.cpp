#include "errorhandler.h"

#include <sstream>

using namespace std;

Error::Error(Error::No no) : m_no(no) {
}

void Error::throwError(
  ErrorHandler& errorHandler, No no, string additionalMsg) {
  if (!errorHandler.isReportingDisabledFor(no)) {
    auto error = std::unique_ptr<Error>(new Error(no));
    stringstream ss;
    ss << *error;
    if (!additionalMsg.empty()) { ss << ", " << additionalMsg; }
    errorHandler.add(std::move(error));
    throw BuildError(ss.str());
  }
}

const char* toStr(Error::No no) {
  switch (no) {
    // clang-format off
  case Error::eNone: return "eNone";
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
  return os << error.no();
}

ErrorHandler::ErrorHandler() : m_disabledErrors{} {
}

void ErrorHandler::add(std::unique_ptr<Error> error) {
  m_errors.emplace_back(std::move(error));
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

BuildError::BuildError(const string& what) : logic_error(what) {
}
