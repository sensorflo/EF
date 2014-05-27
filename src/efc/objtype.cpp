#include "objtype.h"
#include <cassert>
#include <sstream>
using namespace std;

string ObjType::toStr() const {
  ostringstream ss;
  ss << *this;
  return ss.str();
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
  os << "int";
  if (eMutable==m_qualifier) {
    os << "-mut";
  }
  return os;
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

