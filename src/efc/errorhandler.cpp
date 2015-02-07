#include "errorhandler.h"

ErrorHandler::~ErrorHandler() {
  for ( Container::iterator i = m_errors.begin(); i!=m_errors.end(); ++i) {
    delete *i;
  }
}
