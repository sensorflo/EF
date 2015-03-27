#include "errorhandler.h"
#include <sstream>
using namespace std;

Error::Error(Error::No no) :
  m_no(no) {
}

void Error::throwError(ErrorHandler& errorHandler, No no) {
  auto error = new Error(no);
  errorHandler.add(error);
  stringstream ss;
  ss << *error;
  throw BuildError(ss.str());
}

const char* toStr(Error::No no) {
  switch (no) {
  case Error::eNone: return "eNone";
  case Error::eUnknownName: return "eUnknownName";
  case Error::eIncompatibleRedaclaration: return "eIncompatibleRedaclaration";
  case Error::eRedefinition: return "eRedefinition";
  case Error::eWriteToImmutable: return "eWriteToImmutable";
  case Error::eNoImplicitConversion: return "eNoImplicitConversion";
  case Error::eInvalidArguments: return "eInvalidArguments";
  case Error::eNoSuchMember: return "eNoSuchMember";
  case Error::eNotInFunBodyContext: return "eNotInFunBodyContext";
  case Error::eUnreachableCode: return "eUnreachableCode";
  case Error::eComputedValueNotUsed: return "eComputedValueNotUsed";
  case Error::eCnt: return "<unknown>";
  }
  return "<unknown>";
}

ostream& operator<<(ostream& os, Error::No no) {
  return os << toStr(no);
}

ostream& operator<<(ostream& os, const Error& error) {
  return os << error.no();
}

ErrorHandler::~ErrorHandler() {
  for ( Container::iterator i = m_errors.begin(); i!=m_errors.end(); ++i) {
    delete *i;
  }
}

ostream& operator<<(ostream& os, const ErrorHandler& errorHandler) {
  os << "{";
  for ( ErrorHandler::Container::const_iterator i = errorHandler.errors().begin();
        i != errorHandler.errors().end(); ++i) {
    if ( i!=errorHandler.errors().begin() ) { os << ", "; }
    os << (*i)->no();
  }
  return os << "}";
}

BuildError::BuildError(const string& what) :
  logic_error(what){
}
