#include "objtype.h"

#include "ast.h"
#include "irgen.h"

#include <cassert>
#include <sstream>
using namespace std;
using namespace llvm;

ObjType::ObjType(string name) : EnvNode(move(name)) {
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

ObjType::MatchType ObjType::match2(
  const ObjTypeQuali& src, bool isLevel0) const {
  return src.match2Quali(*this, isLevel0, false);
}

std::shared_ptr<const ObjType> ObjType::unqualifiedObjType() const {
  return shared_from_this();
}

string ObjType::completeName() const {
  std::ostringstream ss;
  printTo(ss);
  return ss.str();
}

std::basic_ostream<char>& operator<<(
  std::basic_ostream<char>& os, const ObjType& objType) {
  return objType.printTo(os);
}

basic_ostream<char>& operator<<(
  basic_ostream<char>& os, ObjType::Qualifiers qualifiers) {
  if (ObjType::eNoQualifier == qualifiers) { return os << "no-qualifier"; }

  if (qualifiers & ObjType::eMutable) { os << "mut"; }
  return os;
}

basic_ostream<char>& operator<<(
  basic_ostream<char>& os, ObjType::MatchType mt) {
  switch (mt) {
  case ObjType::eNoMatch: return os << "NoMatch";
  case ObjType::eMatchButAllQualifiersAreWeaker:
    return os << "MatchButAllQualifiersAreWeaker";
  case ObjType::eMatchButAnyQualifierIsStronger:
    return os << "MatchButAnyQualifierIsStronger";
  case ObjType::eFullMatch: return os << "FullMatch";
  default: assert(false); return os;
  }
}

ObjTypeQuali::ObjTypeQuali(
  Qualifiers qualifiers, std::shared_ptr<const ObjType> type)
  : ObjType("")
  , m_qualifiers(static_cast<Qualifiers>(qualifiers | type->qualifiers()))
  , m_type((assert(type), type->unqualifiedObjType())) {
  assert(m_type);
  assert(not(qualifiers != eNoQualifier && m_type->is(eAbstract)));
}

std::basic_ostream<char>& ObjTypeQuali::printTo(
  std::basic_ostream<char>& os) const {
  if (eMutable & m_qualifiers) { os << "mut-"; }
  return os << *m_type;
}

ObjType::MatchType ObjTypeQuali::match(
  const ObjType& dst, bool isLevel0) const {
  return dst.match2(*this, isLevel0);
}

ObjType::MatchType ObjTypeQuali::match2(
  const ObjTypeQuali& src, bool isRoot) const {
  switch (src.m_type->match(*m_type)) {
  case eNoMatch: return eNoMatch;
  case eFullMatch:
    if (src.m_qualifiers == m_qualifiers) { return eFullMatch; }
    else if (m_qualifiers & eMutable) {
      return eMatchButAllQualifiersAreWeaker;
    }
    else {
      return eMatchButAnyQualifierIsStronger;
    }
  default: assert(false); return eNoMatch;
  }
}

ObjType::MatchType ObjTypeQuali::match2(
  const ObjTypeFunda& src, bool isRoot) const {
  return match2Quali(src, isRoot, true);
}

ObjType::MatchType ObjTypeQuali::match2(
  const ObjTypePtr& src, bool isRoot) const {
  return match2Quali(src, isRoot, true);
}

ObjType::MatchType ObjTypeQuali::match2(
  const ObjTypeFun& src, bool isRoot) const {
  return match2Quali(src, isRoot, true);
}

ObjType::MatchType ObjTypeQuali::match2Quali(
  const ObjType& type, bool isRoot, bool typeIsSrc) const {
  switch (type.match(*m_type)) {
  case eNoMatch: return eNoMatch;
  case eFullMatch:
    if (m_qualifiers & eMutable) {
      return typeIsSrc ? eMatchButAllQualifiersAreWeaker
                       : eMatchButAnyQualifierIsStronger;
    }
    else {
      return typeIsSrc ? eMatchButAnyQualifierIsStronger
                       : eMatchButAllQualifiersAreWeaker;
    }
  default: assert(false); return eNoMatch;
  }
}

bool ObjTypeQuali::is(EClass class_) const {
  return m_type->is(class_);
}

int ObjTypeQuali::size() const {
  return m_type->size();
}

llvm::Type* ObjTypeQuali::llvmType() const {
  return m_type->llvmType();
}

bool ObjTypeQuali::hasMember(int op) const {
  return m_type->hasMember(op);
}

bool ObjTypeQuali::hasConstructor(const ObjType& other) const {
  return m_type->hasConstructor(other);
}

std::shared_ptr<const ObjType> ObjTypeQuali::unqualifiedObjType() const {
  return m_type;
}

namespace {
std::string toStr(ObjTypeFunda::EType type) {
  switch (type) {
  case ObjTypeFunda::eVoid: return "void";
  case ObjTypeFunda::eNoreturn: return "noreturn";
  case ObjTypeFunda::eInfer: return "infer";
  case ObjTypeFunda::eChar: return "char";
  case ObjTypeFunda::eInt: return "int";
  case ObjTypeFunda::eBool: return "bool";
  case ObjTypeFunda::eDouble: return "double";
  case ObjTypeFunda::eNullptr: return "Nullptr";
  case ObjTypeFunda::ePointer: return "raw*";
  case ObjTypeFunda::eTypeCnt: assert(false);
  };
}
}

ObjTypeFunda::ObjTypeFunda(EType type) : ObjType(::toStr(type)), m_type(type) {
}

ObjType::MatchType ObjTypeFunda::match(
  const ObjType& dst, bool isLevel0) const {
  return dst.match2(*this, isLevel0);
}

ObjType::MatchType ObjTypeFunda::match2(
  const ObjTypeFunda& src, bool isRoot) const {
  return m_type == src.m_type ? eFullMatch : eNoMatch;
}

basic_ostream<char>& ObjTypeFunda::printTo(basic_ostream<char>& os) const {
  return os << name();
}

bool ObjTypeFunda::is(ObjType::EClass class_) const {
  switch (m_type) {
  case eVoid:
  case eNoreturn:
  case eInfer: return class_ == eAbstract;
  case eChar:
  case eBool:
  case eInt:
  case eDouble:
  case ePointer:
  case eNullptr:
    if (class_ == eScalar) return true;
    if (class_ == eStoredAsIntegral) return m_type != eDouble;
    switch (m_type) {
    case eBool: // fall through
    case eChar: // fall through
    case ePointer: // fall through
    case eNullptr: return false;
    case eInt:
    case eDouble:
      if (class_ == eArithmetic) return true;
      if (class_ == eIntegral) return m_type == eInt;
      if (class_ == eFloatingPoint) return m_type == eDouble;
      return false;
    default: assert(false);
    }
    return false;
  case eTypeCnt: assert(false);
  }
  return false;
}

int ObjTypeFunda::size() const {
  switch (m_type) {
  case eVoid:
  case eNoreturn: return -1;
  case eInfer: return -1;
  case eNullptr: return 0;
  case eBool: return 1;
  case eChar: return 8;
  case eInt: return 32;
  case eDouble: return 64;
  case ePointer: return 32;
  case eTypeCnt: assert(false);
  }
  assert(false);
  return -1;
}

llvm::Type* ObjTypeFunda::llvmType() const {
  switch (m_type) {
  case eVoid: return Type::getVoidTy(llvmContext);
  case eNoreturn: return nullptr;
  case eInfer: return nullptr;
  case eChar: return Type::getInt8Ty(llvmContext);
  case eInt: return Type::getInt32Ty(llvmContext);
  case eDouble: return Type::getDoubleTy(llvmContext);
  case eBool: return Type::getInt1Ty(llvmContext);
  case ePointer: assert(false); // actually implemented by derived class
  case eNullptr: assert(false);
  case eTypeCnt: assert(false);
  };
  assert(false);
  return nullptr;
}

bool ObjTypeFunda::hasMember(int op) const {
  // general rules
  // -------------
  // Abstract objects have no members at all.
  if (is(eAbstract)) { return false; }
  // The addrof operator is applicable to every concrete (i.e. non-abstact)
  // object. It's never a member function operator
  else if (op == AstOperator::eAddrOf) {
    return true;
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
    if (op == AstOperator::eDeref)
      return m_type == ePointer;
    else if (op == AstOperator::eAddrOf)
      return !is(eAbstract);
    else
      break;
  case AstOperator::eOther: break;
  }

  assert(false);
  return false;
}

bool ObjTypeFunda::hasConstructor(const ObjType& other) const {
  // double dispatch is not yet required; currently it's more practical to
  // just use RTTI
  if (typeid(ObjTypeFunda) != typeid(other)) { return false; }
  const auto& otherFunda = static_cast<const ObjTypeFunda&>(other);
  switch (m_type) {
  case eVoid: // fall through
  case eNoreturn: // fall through
  case eInfer: return false;

  case eBool: // fall through
  case eChar: // fall through
  case eDouble: // fall through
  case eInt: // fall through
  case ePointer:
    if ((otherFunda.m_type == eVoid) || (otherFunda.m_type == eNoreturn) ||
      (otherFunda.m_type == eInfer)) {
      return false;
    }
    else if (ePointer == m_type) {
      return true;
    }
    else {
      return otherFunda.m_type != eNullptr;
    }

  case eNullptr: return otherFunda.m_type == eNullptr;

  case eTypeCnt: assert(false);
  }
  return false;
}

bool ObjType::matchesFully_(const ObjType& rhs, const ObjType& lhs) {
  return rhs.matchesFully(lhs);
}

bool ObjType::matchesSaufQualifiers_(const ObjType& rhs, const ObjType& lhs) {
  return rhs.matchesSaufQualifiers(lhs);
}

ObjTypePtr::ObjTypePtr(shared_ptr<const ObjType> pointee)
  : ObjTypeFunda(ePointer), m_pointee(move(pointee)) {
  assert(m_pointee);
}

ObjType::MatchType ObjTypePtr::match(const ObjType& dst, bool isLevel0) const {
  return dst.match2(*this, isLevel0);
}

ObjType::MatchType ObjTypePtr::match2(
  const ObjTypePtr& src, bool isRoot) const {
  //           dst    =    src
  // level 0:  qualifiers irrelevant to decide whether its a match or not
  // level 1+: currently: type and qualifiers must match completely
  //           in future: dst must have all stronger qualifiers == src must
  //           have all weaker qualifiers
  const auto pointeeMatch = m_pointee->match(*src.m_pointee.get(), false);
  if (pointeeMatch != eFullMatch) { return eNoMatch; }
  // now we know pointee is a match

  return ObjTypeFunda::match2(static_cast<const ObjTypeFunda&>(src), isRoot);
}

std::basic_ostream<char>& ObjTypePtr::printTo(
  std::basic_ostream<char>& os) const {
  ObjTypeFunda::printTo(os);
  m_pointee->printTo(os);
  return os;
}

llvm::Type* ObjTypePtr::llvmType() const {
  return PointerType::get(m_pointee->llvmType(), 0);
}

std::shared_ptr<const ObjType> ObjTypePtr::pointee() const {
  return m_pointee;
}

ObjTypeFun::ObjTypeFun(
  vector<shared_ptr<const ObjType>>* args, shared_ptr<const ObjType> ret)
  : ObjType("fun") // todo: what shall the correct type name of a function be?
  , m_args{args ? std::unique_ptr<vector<shared_ptr<const ObjType>>>{args}
                : std::make_unique<vector<shared_ptr<const ObjType>>>()}
  , m_ret{
      ret ? move(ret) : make_shared<const ObjTypeFunda>(ObjTypeFunda::eInt)} {
  assert(m_args);
  assert(m_ret);
  for (const auto& arg : *m_args) { assert(arg); }
}

ObjType::MatchType ObjTypeFun::match(const ObjType& dst, bool isLevel0) const {
  return dst.match2(*this, isLevel0);
}

ObjType::MatchType ObjTypeFun::match2(
  const ObjTypeFun& src, bool isRoot) const {
  if (m_args->size() != src.m_args->size()) return eNoMatch;
  for (auto i = m_args->begin(), isrc = src.m_args->begin(); i != m_args->end();
       ++i, ++isrc) {
    if ((*i)->match(**isrc) != eFullMatch) return eNoMatch;
  }
  if (m_ret->match(*src.m_ret) != eFullMatch) return eNoMatch;
  return eFullMatch;
}

bool ObjTypeFun::is(ObjType::EClass class_) const {
  return class_ == eFunction;
}

basic_ostream<char>& ObjTypeFun::printTo(basic_ostream<char>& os) const {
  os << "fun((";
  for (auto i = m_args->begin(); i != m_args->end(); ++i) {
    if (i != m_args->begin()) { os << ", "; }
    os << **i;
  }
  os << ") ";
  os << *m_ret;
  os << ")";
  return os;
}

vector<shared_ptr<const ObjType>>* ObjTypeFun::createArgs(
  const ObjType* arg1, const ObjType* arg2, const ObjType* arg3) {
  auto l = new vector<shared_ptr<const ObjType>>;
  if (arg1) { l->push_back(shared_ptr<const ObjType>(arg1)); }
  if (arg2) { l->push_back(shared_ptr<const ObjType>(arg2)); }
  if (arg3) { l->push_back(shared_ptr<const ObjType>(arg3)); }
  return l;
}

llvm::Type* ObjTypeFun::llvmType() const {
  assert(false); // not implemented yet
}

ObjTypeCompound::ObjTypeCompound(
  const string& name, vector<shared_ptr<const ObjType>>&& members)
  : ObjType(move(name)), m_members(move(members)) {
}

ObjTypeCompound::ObjTypeCompound(const string& name,
  shared_ptr<const ObjType> member1, shared_ptr<const ObjType> member2,
  shared_ptr<const ObjType> member3)
  : ObjTypeCompound(name, createMembers(member1, member2, member3)) {
}

vector<shared_ptr<const ObjType>> ObjTypeCompound::createMembers(
  shared_ptr<const ObjType> member1, shared_ptr<const ObjType> member2,
  shared_ptr<const ObjType> member3) {
  vector<shared_ptr<const ObjType>> members;
  if (member1) { members.emplace_back(member1); }
  if (member2) { members.emplace_back(member2); }
  if (member3) { members.emplace_back(member3); }
  return members;
}

basic_ostream<char>& ObjTypeCompound::printTo(basic_ostream<char>& os) const {
  os << "class(" << name();
  for (const auto& member : m_members) { os << " " << *member; }
  return os << ")";
}

ObjType::MatchType ObjTypeCompound::match(
  const ObjType& dst, bool isLevel0) const {
  return dst.match2(*this, isLevel0);
}

ObjType::MatchType ObjTypeCompound::match2(
  const ObjTypeCompound& src, bool isRoot) const {
  // Each ObjTypeCompound instance represents a distinct type
  return this == &src ? eFullMatch : eNoMatch;
}

bool ObjTypeCompound::is(EClass class_) const {
  return false;
}

int ObjTypeCompound::size() const {
  int sum = 0;
  for (const auto& member : m_members) { sum += member->size(); }
  return sum;
}

llvm::Type* ObjTypeCompound::llvmType() const {
  assert(false);
  return nullptr;
}

bool ObjTypeCompound::hasMember(int op) const {
  assert(false);
  return false;
}

bool ObjTypeCompound::hasConstructor(const ObjType& other) const {
  assert(false);
  return false;
}
