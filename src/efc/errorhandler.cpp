#include "errorhandler.h"

#include <sstream>
#include <string>
#include <utility>

using namespace std;

namespace {
string plural(const string& noun, const string& count) {
  if (count == "0") { return "zero " + noun + "s"; }
  if (count == "1") { return count + " " + noun; }
  return count + " " + noun + "s";
}

string conjucateToBe(const string& count) {
  if (count == "1") { return "is"; }
  return "are";
}
}

Error::Error(Error::No no, string message, string msgParam1, string msgParam2,
  string msgParam3)
  : m_no{no}
  , m_message{move(message)}
  , m_msgParam1{move(msgParam1)}
  , m_msgParam2{move(msgParam2)}
  , m_msgParam3{move(msgParam3)} {
}

void Error::throwError(ErrorHandler& errorHandler, No no, Location loc,
  string msgParam1, string msgParam2, string msgParam3) {
  if (!errorHandler.isReportingDisabledFor(no)) {
    stringstream ss{};
    if (!loc.isNull()) {
      if (loc.begin.m_fileName != nullptr) { ss << *loc.begin.m_fileName; }
      else {
        ss << "<anonymous file>";
      }
      ss << ":" << loc.begin.m_line << ":" << loc.begin.m_column << ": ";
    }
    ss << "error: " << describe(no, msgParam1, msgParam2, msgParam3) << " ["
       << no << "]";

    auto error = shared_ptr<Error>{new Error{
      no, ss.str(), move(msgParam1), move(msgParam2), move(msgParam3)}};
    errorHandler.add(error);
    throw BuildError{error};
  }
}

string Error::describe(Error::No no, const string& msgParam1,
  const string& msgParam2, const string& /*msgParam3*/) {
  switch (no) {
    // clang-format off
  case Error::eNone: return "no error";
  case Error::eInternalError: return "internal error: " + msgParam1;
  case Error::eUnknownIntegralLiteralSuffix: return "unknown literal number suffix '" + msgParam1 + "'";
  case Error::eLiteralOutOfValidRange: return "literal is outside of the valid range [" + msgParam1 + ", " + msgParam2 + "]";
  case Error::eUnknownName: return "use of undefined name '" + msgParam1 + "'";
  case Error::eRedefinition: return "redefinition of name '" + msgParam1 + "'";
  case Error::eWriteToImmutable: return "can't modify immutable object";
  case Error::eCantOpenFileForReading: return "Can't open file '" + msgParam1 + "' for reading (" + msgParam2 + ")";
  case Error::eNoImplicitConversion: return "there's no implicit conversion from type '" + msgParam1+ "' to type '" + msgParam2 + "'";
  case Error::eInvalidArguments: return "passed " + plural("argument", msgParam1) + ", but " + plural("argument", msgParam2) + " " + conjucateToBe(msgParam2) + " expected";
    // we take advantage of the fact that currently constructors only have one argument
  case Error::eNoSuchCtor: return "type '" + msgParam1 + "' has no constructor callable with (" + msgParam2 + ")";
  case Error::eNoSuchMemberFun: return "type '" + msgParam1 + "' has no member function '" + msgParam2 + "'";
  case Error::eNotInFunBodyContext: return "return is not allowed outside a function definition";
  case Error::eUnreachableCode: return "leaves control flow and the following code is not reachable";
  case Error::eCTConstRequired: return "currently static objects can only be initialized with compile time const expressions (which in turn are currently limited to literals)";
  case Error::eRetTypeCantHaveMutQualifier: return "currently the return type cannot have the mutable qualifier";
  case Error::eMultipleInitializers: return "definition has multipile initializers";
  case Error::eObjectExpected: return "expecting an expression of meta type object";
  case Error::eParseFailed: return msgParam1;
  case Error::eUnexpectedCharacter: return "encountered unexpected character '" + msgParam1 + "'";
  case Error::eInvalidStorageDurationInDef: return "storage duration '" + msgParam1 + "' is invalid for this definition";
  case Error::eTypeInferenceIsNotYetSupported: return "currently type inference is not yet supported";
  case Error::eNonIgnoreAccessToLocalDataObjectBeforeItsInitialization: return "local object '" + msgParam1 + "' is accessed before it is initialized";
  case Error::eCnt: return "<unknown>";
    // clang-format on
  }
  return "<unknown>";
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
  case Error::eNoSuchCtor: return "eNoSuchCtor";
  case Error::eNoSuchMemberFun: return "eNoSuchMemberFun";
  case Error::eNotInFunBodyContext: return "eNotInFunBodyContext";
  case Error::eUnreachableCode: return "eUnreachableCode";
  case Error::eCTConstRequired: return "eCTConstRequired";
  case Error::eRetTypeCantHaveMutQualifier: return "eRetTypeCantHaveMutQualifier";
  case Error::eMultipleInitializers: return "eMultipleInitializers";
  case Error::eObjectExpected: return "eObjectExpected";
  case Error::eParseFailed: return "eParseFailed";
  case Error::eUnexpectedCharacter: return "eUnexpectedCharacter";
  case Error::eInvalidStorageDurationInDef: return "eInvalidStorageDurationInDef";
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
  m_disabledErrors.at(no) = true;
}

bool ErrorHandler::isReportingDisabledFor(Error::No no) const {
  return m_disabledErrors.at(no);
}

ErrorHandler::~ErrorHandler() = default;

ostream& operator<<(ostream& os, const ErrorHandler& errorHandler) {
  auto isFirstIter = true;
  for (const auto& e : errorHandler.errors()) {
    if (!isFirstIter) {
      os << "\n";
      isFirstIter = false;
    }
    os << *e;
  }
  return os;
}

BuildError::BuildError(shared_ptr<const Error> error)
  : logic_error{error->message()}, m_error{move(error)} {
}

BuildError::~BuildError() = default;

ostream& operator<<(ostream& os, const BuildError& error) {
  os << error.error();
  return os;
}
