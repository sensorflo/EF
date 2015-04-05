#include "objtype.h"
#include "ast.h"
#include <cassert>
#include <sstream>
#include "llvm/IR/LLVMContext.h"
using namespace std;
using namespace llvm;

ObjType& ObjType::addQualifiers(Qualifiers qualifiers) {
  m_qualifiers = static_cast<Qualifiers>(m_qualifiers | qualifiers);
  // don't allow for certain types, e.g. fun, void, noret
  return *this;
}

ObjType& ObjType::removeQualifiers(Qualifiers qualifiers) {
  m_qualifiers = static_cast<Qualifiers>(m_qualifiers & ~qualifiers);
  return *this;
}

string ObjType::toStr() const {
  ostringstream ss;
  ss << *this;
  return ss.str();
}

bool ObjType::isVoid() const {
  static const ObjTypeFunda voidType(ObjTypeFunda::eVoid);
  return matchesFully(voidType);
}

bool ObjType::isNoreturn() const {
  static const ObjTypeFunda noreturnType(ObjTypeFunda::eNoreturn);
  return matchesFully(noreturnType);
}

bool ObjType::matchesFully(const ObjType& other) const {
  return match(other) == eFullMatch;
}

bool ObjType::matchesSaufQualifiers(const ObjType& other) const {
  auto m = this->match(other);
  return (m == eOnlyQualifierMismatches) || (m == eFullMatch);
}

basic_ostream<char>& operator<<(basic_ostream<char>& os, ObjType::Qualifiers qualifiers) {
  if (ObjType::eNoQualifier==qualifiers) {
    return os << "no-qualifier";
  }

  if (qualifiers & ObjType::eMutable) { os << "mut"; }
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

basic_ostream<char>& operator<<(basic_ostream<char>& os, ObjType::StorageDuration sd) {
  switch (sd) {
  case ObjType::eLocal: return os;
  case ObjType::eStatic: return os << "static";
  default: assert(false); return os;
  }
  return os;
}

ObjTypeFunda::ObjTypeFunda(EType type, Qualifiers qualifiers,
  StorageDuration storageDuration)  :
  ObjType(qualifiers), m_type(type), m_storageDuration(storageDuration) {
  if ( type==eVoid ) {
    assert( qualifiers == eNoQualifier );
  }
  // same for eNoret
}

ObjTypeFunda::ObjTypeFunda(EType type, StorageDuration storageDuration) :
  ObjTypeFunda(type, eNoQualifier, storageDuration) {}

ObjType::MatchType ObjTypeFunda::match2(const ObjTypeFunda& other) const {
  if (m_type!=other.m_type) { return eNoMatch; }
  if (m_qualifiers!=other.m_qualifiers) { return eOnlyQualifierMismatches; }
  return eFullMatch;
}

basic_ostream<char>& ObjTypeFunda::printTo(basic_ostream<char>& os) const {
  switch (m_type) {
  case eVoid: os << "void"; break;
  case eNoreturn: os << "noreturn"; break;
  case eChar: os << "char"; break;
  case eInt: os << "int"; break;
  case eBool: os << "bool"; break;
  case eDouble: os << "double"; break;
  };
  if (eMutable & m_qualifiers) {
    os << "-mut";
  }
  if (m_storageDuration != eLocal) {
    os << "-" << m_storageDuration;
  }
  return os;
}

bool ObjTypeFunda::is(ObjType::EClass class_) const {
  switch(m_type) {
  case eVoid:
  case eNoreturn:
    return class_==eAbstract;
  case eChar:
  case eBool:
  case eInt:
  case eDouble:
    if (class_==eScalar) return true;
    if (class_==eStoredAsIntegral) return m_type!=eDouble;
    switch(m_type) {
    case eBool: // fall through
    case eChar:
      return false;
    case eInt:
    case eDouble:
      if (class_==eArithmetic) return true;
      if (class_==eIntegral) return m_type==eInt;
      if (class_==eFloatingPoint) return m_type==eDouble;
      return false;
    default: assert(false);
    }
    return false;
  }
  return false;
}

int ObjTypeFunda::size() const {
  switch(m_type) {
  case eVoid:
  case eNoreturn: return -1;
  case eBool: return 1;
  case eChar: return 8;
  case eInt: return 32;
  case eDouble: return 64;
  }
  assert(false);
  return -1;
}

AstValue* ObjTypeFunda::createDefaultAstValue() const {
  assert(!is(eAbstract));
  return new AstNumber(0, new ObjTypeFunda(m_type));
}

llvm::Type* ObjTypeFunda::llvmType() const {
  switch (m_type) {
  case eVoid: return Type::getVoidTy(getGlobalContext());
  case eNoreturn: return nullptr;
  case eChar: return Type::getInt8Ty(getGlobalContext());
  case eInt: return Type::getInt32Ty(getGlobalContext());
  case eDouble: return Type::getDoubleTy(getGlobalContext());
  case eBool: return Type::getInt1Ty(getGlobalContext());
  };
  assert(false);
  return NULL;
}

bool ObjTypeFunda::hasMember(int op) const {
  switch (AstOperator::classOf(static_cast<AstOperator::EOperation>(op))) {
  case AstOperator::eAssignment: return is(eScalar);
  case AstOperator::eArithmetic: return is(eArithmetic);
  case AstOperator::eLogical: return m_type == eBool;
  case AstOperator::eComparison: return is(eScalar);
  case AstOperator::eOther: assert(false); return false;
  }
  assert(false);
  return false;
}

bool ObjTypeFunda::hasConstructor(const ObjType& other) const {
  // double dispatch is not yet required; currently it's more practical to
  // just use RTTI
  if (typeid(ObjTypeFunda) != typeid(other)) {
    return false;
  }
  const ObjTypeFunda& otherFunda = static_cast<const ObjTypeFunda&>(other);
  switch (m_type) {
  case eVoid: // fall through
  case eNoreturn: return false;
  case eBool: // fall through
  case eChar: // fall through
  case eDouble: // fall through
  case eInt:
    return (otherFunda.m_type != eVoid) && (otherFunda.m_type != eNoreturn);
  default: assert(false);
  }
  return false;
}

bool ObjTypeFunda::isValueInRange(double val) const {
  switch (m_type) {
  case eVoid: return false;
  case eNoreturn: return false;
  case eChar: return (0<=val && val<=0xFF) && (val==static_cast<int>(val));
  case eInt: return (INT_MIN<=val && val<=INT_MAX) && (val==static_cast<int>(val));
  case eDouble: return true;
  case eBool: return val==0.0 || val==1.0;
  };
  assert(false);
  return false;
}

ObjTypeFunda* ObjTypeFunda::clone() const {
  return new ObjTypeFunda(*this);
}

bool ObjType::matchesFully_(const ObjType& rhs, const ObjType& lhs) {
  return rhs.matchesFully(lhs);
}

bool ObjType::matchesSaufQualifiers_(const ObjType& rhs, const ObjType& lhs) {
  return rhs.matchesSaufQualifiers(lhs);
}

ObjTypeFun::ObjTypeFun(list<shared_ptr<const ObjType> >* args, shared_ptr<const ObjType> ret) :
  ObjType(eNoQualifier),
  m_args( args ? args : new list<shared_ptr<const ObjType> >),
  m_ret( ret ? move(ret) : make_shared<const ObjTypeFunda>(ObjTypeFunda::eInt) ){
  assert(m_args);
  assert(m_ret);
  for (list<shared_ptr<const ObjType> >::const_iterator i=m_args->begin(); i!=m_args->end(); ++i) {
    assert(*i);
  }
}

ObjType::MatchType ObjTypeFun::match2(const ObjTypeFun& other) const {
  if (m_args->size() != other.m_args->size()) return eNoMatch;
  for (list<shared_ptr<const ObjType> >::const_iterator i=m_args->begin(), iother=other.m_args->begin();
       i!=m_args->end();
       ++i, ++iother) {
    if ((*i)->match(**iother) != eFullMatch) return eNoMatch;
  }
  if (m_ret->match(*other.m_ret) != eFullMatch) return eNoMatch;
  return eFullMatch;
}

bool ObjTypeFun::is(ObjType::EClass class_) const {
  return class_ == eFunction;
}

ObjTypeFun* ObjTypeFun::clone() const {
  std::list<std::shared_ptr<const ObjType> >* dstArgs =
    new list<shared_ptr<const ObjType> >;
  for ( const auto& srcArg : *m_args  ) {
    dstArgs->push_back(shared_ptr<ObjType>{srcArg->clone()});
  }
  return new ObjTypeFun( dstArgs, shared_ptr<const ObjType>(m_ret->clone()));
}

basic_ostream<char>& ObjTypeFun::printTo(basic_ostream<char>& os) const {
  os << "fun((";
  for (list<shared_ptr<const ObjType> >::const_iterator i=m_args->begin(); i!=m_args->end(); ++i) {
    if (i!=m_args->begin()) { os << ", "; }
    os << **i;
  }
  os << "), ";
  os << *m_ret;  
  os << ")";
  return os;
}

list<shared_ptr<const ObjType> >* ObjTypeFun::createArgs(const ObjType* arg1, const ObjType* arg2,
  const ObjType* arg3) {
  list<shared_ptr<const ObjType> >* l = new list<shared_ptr<const ObjType> >;
  if (arg1) { l->push_back(shared_ptr<const ObjType>(arg1)); }
  if (arg2) { l->push_back(shared_ptr<const ObjType>(arg2)); }
  if (arg3) { l->push_back(shared_ptr<const ObjType>(arg3)); }
  return l;
}

AstValue* ObjTypeFun::createDefaultAstValue() const {
  // The liskov substitution principle is broken here, ObjTypeFun cannot
  // provide a createDefaultAstValue method. A possible solution would be to introduce
  // a 'ObjTypeData' abstract class into the ObjType hierarchy, but currently
  // I think that is overkill.
  assert(false);
}

llvm::Type* ObjTypeFun::llvmType() const {
  assert(false); // not implemented yet
}

