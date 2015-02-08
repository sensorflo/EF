#include "errorhandler.h"
using namespace std;

const char* toStr(Error::No no) {
  switch (no) {
  case Error::eNone: return "eNone";
  case Error::eUnknownName: return "eUnknownName";
  case Error::eIncompatibleRedaclaration: return "eIncompatibleRedaclaration";
  case Error::eRedefinition: return "eRedefinition";
  case Error::eCnt: return "<unknown>";
  }
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
