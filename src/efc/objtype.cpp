#include "objtype.h"
#include "ast.h"
#include <cassert>
#include <sstream>
using namespace std;

ObjType& ObjType::addQualifier(Qualifier qualifier) {
  m_qualifier = static_cast<Qualifier>(m_qualifier | qualifier);
  return *this;
}

string ObjType::toStr() const {
  ostringstream ss;
  ss << *this;
  return ss.str();
}

basic_ostream<char>& operator<<(basic_ostream<char>& os, ObjType::Qualifier qualifier) {
  if (ObjType::eNoQualifier==qualifier) {
    return os << "no-qualifier";
  }

  if (qualifier & ObjType::eMutable) { os << "mut"; }
  return os;
}

basic_ostream<char>& operator<<(basic_ostream<char>& os, ObjType::MatchType mt) {
  switch (mt) {
  case ObjType::eNoMatch: return os << "NoMatch";
  case ObjType::eOnlyQualifierMismatches: return os << "OnlyQualifierMismatches";
  case ObjType::eFullMatch: return os << "FullMatch";
  default: assert(false); return os;
  }
}

ObjType::MatchType ObjTypeFunda::match2(const ObjTypeFunda& other) const {
  if (m_type!=other.m_type) { return eNoMatch; }
  if (m_qualifier!=other.m_qualifier) { return eOnlyQualifierMismatches; }
  return eFullMatch;
}

basic_ostream<char>& ObjTypeFunda::printTo(basic_ostream<char>& os) const {
  switch (m_type) {
  case eInt: os << "int"; break;
  case eBool: os << "bool"; break;
  };
  if (eMutable & m_qualifier) {
    os << "-mut";
  }
  return os;
}

AstValue* ObjTypeFunda::createDefaultAstValue() const {
  return new AstNumber(0, new ObjTypeFunda(m_type));
}

ObjTypeFun::ObjTypeFun(list<ObjType*>* args, ObjType* ret) :
  ObjType(eNoQualifier),
  m_args( args ? args : new list<ObjType*>),
  m_ret( ret ? ret : new ObjTypeFunda(ObjTypeFunda::eInt) ){
  assert(m_args);
  assert(m_ret);
  for (list<ObjType*>::const_iterator i=m_args->begin(); i!=m_args->end(); ++i) {
    assert(*i);
  }
}

ObjTypeFun::~ObjTypeFun() {
  for (list<ObjType*>::const_iterator i=m_args->begin(); i!=m_args->end(); ++i) {
    delete *i;
  }
  delete m_ret;
}

ObjType::MatchType ObjTypeFun::match2(const ObjTypeFun& other) const {
  if (m_args->size() != other.m_args->size()) return eNoMatch;
  for (list<ObjType*>::const_iterator i=m_args->begin(), iother=other.m_args->begin();
       i!=m_args->end();
       ++i, ++iother) {
    if (eFullMatch!=(*i)->match(**iother)) return eNoMatch;
  }
  if (eFullMatch!=m_ret->match(*other.m_ret)) return eNoMatch;
  return eFullMatch;
}

basic_ostream<char>& ObjTypeFun::printTo(basic_ostream<char>& os) const {
  os << "fun((";
  for (list<ObjType*>::const_iterator i=m_args->begin(); i!=m_args->end(); ++i) {
    if (i!=m_args->begin()) { os << ", "; }
    os << **i;
  }
  os << "), ";
  os << *m_ret;  
  os << ")";
  return os;
}

list<ObjType*>* ObjTypeFun::createArgs(ObjType* arg1, ObjType* arg2,
  ObjType* arg3) {
  list<ObjType*>* l = new list<ObjType*>;
  if (arg1) { l->push_back(arg1); }
  if (arg2) { l->push_back(arg2); }
  if (arg3) { l->push_back(arg3); }
  return l;
}

AstValue* ObjTypeFun::createDefaultAstValue() const {
  // The liskov substitution principle is broken here, ObjTypeFun cannot
  // provide a createDefaultAstValue method. A possible solution would be to introduce
  // a 'ObjTypeData' abstract class into the ObjType hierarchy, but currently
  // I think that is overkill.
  assert(false);
}

