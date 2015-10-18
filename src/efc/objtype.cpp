#include "objtype.h"
#include "ast.h"
#include "llvm/IR/LLVMContext.h"
#include <cassert>
#include <sstream>
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

bool ObjType::matchesFully(const ObjType& dst) const {
  return match(dst) == eFullMatch;
}

bool ObjType::matchesSaufQualifiers(const ObjType& dst) const {
  return this->match(dst) != eNoMatch;
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
  case ObjType::eMatchButAllQualifiersAreWeaker: return os << "MatchButAllQualifiersAreWeaker";
  case ObjType::eMatchButAnyQualifierIsStronger: return os << "MatchButAnyQualifierIsStronger";
  case ObjType::eFullMatch: return os << "FullMatch";
  default: assert(false); return os;
  }
}

ObjTypeFunda::ObjTypeFunda(EType type, Qualifiers qualifiers)  :
  ObjType(qualifiers), m_type(type) {
  if ( type==eVoid ) {
    assert( qualifiers == eNoQualifier );
  }
  // same for eNoret
}

ObjType::MatchType ObjTypeFunda::match(const ObjType& dst, bool isLevel0) const {
  return dst.match2(*this, isLevel0);
}

ObjType::MatchType ObjTypeFunda::match2(const ObjTypeFunda& src, bool isRoot) const {
  if (m_type!=src.m_type) { return eNoMatch; }
  if (m_qualifiers==src.m_qualifiers) { return eFullMatch; }
  if (m_qualifiers & eMutable) { return eMatchButAllQualifiersAreWeaker; }
  return eMatchButAnyQualifierIsStronger;
}

basic_ostream<char>& ObjTypeFunda::printTo(basic_ostream<char>& os) const {
  if (eMutable & m_qualifiers) {
    os << "mut-";
  }
  switch (m_type) {
  case eVoid: os << "void"; break;
  case eNoreturn: os << "noreturn"; break;
  case eChar: os << "char"; break;
  case eInt: os << "int"; break;
  case eBool: os << "bool"; break;
  case eDouble: os << "double"; break;
  case ePointer: os << "raw*"; break;
  };
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
  case ePointer:
    if (class_==eScalar) return true;
    if (class_==eStoredAsIntegral) return m_type!=eDouble;
    switch(m_type) {
    case eBool: // fall through
    case eChar: // fall through
    case ePointer:
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
  case ePointer: return 32;
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
  case ePointer: assert(false); // actually implemented by derived class
  };
  assert(false);
  return NULL;
}

bool ObjTypeFunda::hasMember(int op) const {
  // general rules
  // -------------
  // Abstract objects have no members at all.
  if (is(eAbstract)) {
    return false;
  }
  // The addrof operator is applicable to every concrete (i.e. non-abstact)
  // object. It's never a member function operator
  else if (op==AstOperator::eAddrOf) {
    return true;
  }
  // The sequence operator is a global operator, it's never a member function
  // operator
  else if (op==AstOperator::eSeq) {
    assert(false);
  }
  
  // rules specific per object type
  // ------------------------------
  // For abbreviation, summarazied / grouped by operator class and optionally
  // by object type class
  switch (AstOperator::classOf(static_cast<AstOperator::EOperation>(op))) {
  case AstOperator::eAssignment: return is(eScalar);
  case AstOperator::eArithmetic: return is(eArithmetic);
  case AstOperator::eLogical: return m_type == eBool;
  case AstOperator::eComparison: return is(eScalar);
  case AstOperator::eMemberAccess:
    // Currently there is no pointer arithmetic, and currently pointers can't
    // be subtracted from each other, thus currently dereferencing is the only
    // member function of the pointer type
    if      (op==AstOperator::eDeref)  return m_type == ePointer;
    else if (op==AstOperator::eAddrOf) return !is(eAbstract);
    else    break;
  case AstOperator::eOther: break;
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
  case eInt: // fall through
  case ePointer:
    return (otherFunda.m_type != eVoid) && (otherFunda.m_type != eNoreturn);
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
  case ePointer: assert(false);  // not yet implemented
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

ObjTypePtr::ObjTypePtr(shared_ptr<const ObjType> pointee, Qualifiers qualifiers) :
  ObjTypeFunda(ePointer, qualifiers),
  m_pointee(move(pointee)) {
  assert(m_pointee);
}

ObjTypePtr::ObjTypePtr(const ObjTypePtr& other) :
  ObjTypeFunda{other},
  m_pointee{other.m_pointee->clone()} {
}

ObjType::MatchType ObjTypePtr::match(const ObjType& dst, bool isLevel0) const {
  return dst.match2(*this, isLevel0);
}

ObjType::MatchType ObjTypePtr::match2(const ObjTypePtr& src, bool isRoot) const {
  //           dst    =    src
  // level 0:  qualifiers irrelevant to decide whether its a match or not
  // level 1+: currently: type and qualifiers must match completely 
  //           in future: dst must have all stronger qualifiers == src must
  //           have all weaker qualifiers
  const auto pointeeMatch = m_pointee->match(*src.m_pointee.get(), false);
  if ( pointeeMatch!=eFullMatch ) {
    return eNoMatch;
  }
  // now we know pointee is a match

  return ObjTypeFunda::match2(static_cast<const ObjTypeFunda&>(src), isRoot);
}

ObjTypePtr* ObjTypePtr::clone() const {
  return new ObjTypePtr{*this};
};

std::basic_ostream<char>& ObjTypePtr::printTo(
  std::basic_ostream<char>& os) const {
  ObjTypeFunda::printTo(os);
  m_pointee->printTo(os);
  return os;
};

AstValue* ObjTypePtr::createDefaultAstValue() const {
  return new AstNumber(0, clone());
};

llvm::Type* ObjTypePtr::llvmType() const {
  return PointerType::get(m_pointee->llvmType(), 0);
}

const ObjType& ObjTypePtr::pointee() const {
  return *m_pointee.get();
};

ObjTypeFun::ObjTypeFun(list<shared_ptr<const ObjType>>* args, shared_ptr<const ObjType> ret) :
  ObjType{eNoQualifier},
  m_args{ args ?
      std::unique_ptr<list<shared_ptr<const ObjType>>>{args} :
    std::make_unique<list<shared_ptr<const ObjType>>>()},
  m_ret{ ret ? move(ret) : make_shared<const ObjTypeFunda>(ObjTypeFunda::eInt)} {
  assert(m_args);
  assert(m_ret);
  for (list<shared_ptr<const ObjType> >::const_iterator i=m_args->begin(); i!=m_args->end(); ++i) {
    assert(*i);
  }
}

ObjTypeFun::ObjTypeFun(const ObjTypeFun& other) :
  ObjType{other},
  m_args{std::make_unique<list<shared_ptr<const ObjType>>>()},
  m_ret{other.m_ret->clone()} {
  for ( const auto& srcArg : *other.m_args  ) {
    m_args->push_back(shared_ptr<ObjType>{srcArg->clone()});
  }
}

ObjType::MatchType ObjTypeFun::match(const ObjType& dst, bool isLevel0) const {
  return dst.match2(*this, isLevel0);
}

ObjType::MatchType ObjTypeFun::match2(const ObjTypeFun& src, bool isRoot) const {
  if (m_args->size() != src.m_args->size()) return eNoMatch;
  for (list<shared_ptr<const ObjType> >::const_iterator i=m_args->begin(), isrc=src.m_args->begin();
       i!=m_args->end();
       ++i, ++isrc) {
    if ((*i)->match(**isrc) != eFullMatch) return eNoMatch;
  }
  if (m_ret->match(*src.m_ret) != eFullMatch) return eNoMatch;
  return eFullMatch;
}

bool ObjTypeFun::is(ObjType::EClass class_) const {
  return class_ == eFunction;
}

ObjTypeFun* ObjTypeFun::clone() const {
  return new ObjTypeFun(*this);
}

basic_ostream<char>& ObjTypeFun::printTo(basic_ostream<char>& os) const {
  os << "fun((";
  for (list<shared_ptr<const ObjType> >::const_iterator i=m_args->begin(); i!=m_args->end(); ++i) {
    if (i!=m_args->begin()) { os << ", "; }
    os << **i;
  }
  os << ") ";
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

