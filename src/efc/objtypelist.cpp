#include "objtypelist.h"
#include "objtype2.h"
using namespace std;

ObjTypeList::ObjTypeList(ObjTypes2Iter begin, ObjTypes2Iter end) :
  m_begin(begin),
  m_end(end) {
}

void ObjTypeList::printTo(basic_ostream<char>& os) const {
  os << "(";
  for (auto objType = m_begin; objType!=m_end; ++objType) {
    if ( objType!=m_begin ) {
      os << ", ";
    }
    os << **objType;
  }
  os << ")";
}

TemplateParamType::MatchType ObjTypeList::match(const TemplateParamType& rhs) const {
  return rhs.match2(*this);
}

TemplateParamType::MatchType ObjTypeList::match2(const ObjTypeList& lhs) const {
  auto lhsObjType = lhs.m_begin;
  auto objType = m_begin;
  for ( /*nop*/;
               objType!=m_end && lhsObjType!=lhs.m_end;
               ++objType, ++lhsObjType) {
    if ( eFullMatch!=(*lhsObjType)->match(**objType) ) {
      return eNoMatch;
    }
  }
  return objType==m_end && lhsObjType==lhs.m_end ? eFullMatch : eNoMatch;
}
