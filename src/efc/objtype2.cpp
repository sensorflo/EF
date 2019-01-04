#include "objtype2.h"

using namespace std;

UnqualifiedObjType::UnqualifiedObjType(
  std::string name, size_t size, ObjTypes templateArgs, llvm::Type* llvmType)
  : EnvNode(move(name))
  , m_size{size}
  , m_templateArgs{move(templateArgs)}
  , m_llvmType{llvmType} {
}

UnqualifiedObjType::~UnqualifiedObjType() = default;

ObjType::ObjType(UnqualifiedObjType& unqualified, Qualifiers qualifiers)
  : m_unqualified{unqualified}, m_qualifiers{qualifiers} {
}

// match =
// 1) 1st conceptual level: core method to find out
//    - if a given argument is a valid argument given a parameter object type.
//    - type deduction
//    - ...
// 2) 2nd conceptual level that follows from the first:
//    - it's the same type (modulo qualifiers, see MatchType)
//    - (todo: in a ptr chain, dst may make qualifiers stronger)
//    - all this is preceded by first following the piontee if dst is
//      eAutoTakeAddr ptr
//
// @todo: reverse definition src/dst
// weaker   = dst more 'dangerous' / less restrictions than src: dst has mutable, nonvolatile
// stronger = dst less 'dangerous' / more restrictions than src: dst has immutable, volatile
ObjType::MatchType ObjType::match(const ObjType& dst) const {
  const ObjType* dstDerefed = &dst;
  if (dst.isRawPtr()) {
    const auto& dstAsRawPtr =
      dynamic_cast<const RawPtrUnqualifiedObjType&>(dst.m_unqualified);
    if (dstAsRawPtr.init() == EPtrInit::eAutoTakeAddr) {
      dstDerefed = &dstAsRawPtr.pointeeObjType();
    }
  }

  const auto src = this;
  if (&src->m_unqualified != &dstDerefed->m_unqualified) { return eNoMatch; }
  if (src == dstDerefed) { return eFullMatch; }

  // @todo: if dst is pointer, follow pointee obj type to see if dst has
  //        stronger qualifiers, which would be allowed.

  if (dstDerefed->m_qualifiers & eMutable) {
    return eMatchButAllQualifiersAreWeaker;
  }
  return eMatchButAnyQualifierIsStronger;
}

bool ObjType::matchesFully(const ObjType& dst) const {
  return match(dst) == eFullMatch;
}

bool ObjType::matchesSaufQualifiers(const ObjType& dst) const {
  return match(dst) != eNoMatch;
}

// @todo make 'global' type (actually per module), to which free functions &
//       variables are added.
FindFunResult ObjType::findMemberFun(
  const std::string& name, const ObjType& arg1ObjType) const {
  // if EF already had function overloading, find would return multiple nodes,
  // and we had to choose among those candidates.
  const auto envNode = m_unqualified.find(name);
  if (envNode == nullptr) { return {eNameNotFound, 0, 0, "", nullptr}; }

  const auto funObj = dynamic_cast<FunctionObj*>(envNode);
  if (funObj == nullptr) {
    return {eNotAFunction, 0, 0, envNode->description(), nullptr};
  }

  // now that we have the function, test wether arguments match function's parameters
  auto ret = funObj->objType().matchArguments(arg1ObjType);
  if (eFound == ret.m_status) { ret.m_funObj = funObj; }
  return ret;
}

namespace {
ObjTypes flaten(const ObjType& retObjType, ObjTypes&& paramObjTypes) {
  paramObjTypes.push_back(retObjType);
  return paramObjTypes;
}
}

FunUnqualifiedObjType::FunUnqualifiedObjType(
  const ObjType& retObjType, ObjTypes paramObjTypes)
  : UnqualifiedObjType{funCallOpName, typeHasNoSize,
      flaten(retObjType, move(paramObjTypes)), nullptr /* @todo */} {
}

const ObjType& FunUnqualifiedObjType::retObjType() const {
  return templateArgs().back().get();
}

ObjTypes::const_iterator FunUnqualifiedObjType::paramObjTypesBegin() const {
  return templateArgs().cbegin();
}

ObjTypes::const_iterator FunUnqualifiedObjType::paramObjTypesEnd() const {
  return templateArgs().cend();
}

// @todo
// - there's redundancy with SemanticAnalizer::visit(AstFunCall&
//   funCall). Maybe replace that with this function.
// - and/or extract the below to a function matching arglist and a given signature
// - because anyway, this function should not be a member of FunUnqualifiedObjType.
FindFunResult FunUnqualifiedObjType::matchArguments(
  const ObjType& arg1ObjType) const {
  const auto paramCnt =
    static_cast<size_t>(paramObjTypesEnd() - paramObjTypesBegin());
  if (paramCnt != 1) {
    return {eNumberOfArgsMismatches, 1, paramCnt, "", nullptr};
  }

  // @todo iterate over all args. currently only one. abort if one doesn't match
  {
    const auto& param1ObjType = paramObjTypesBegin()->get();

    // base case of 'recursion': param type is pointer
    // @todo conceptuall only if its autoderef initialzer (adi) pointer
    if (param1ObjType.isRawPtr()) {
      if (arg1ObjType.matchesSaufQualifiers(param1ObjType)) {
        return {eFound, 1, 0, "", nullptr};
      }
      return {eAnArgMismatches, 1, 0, "", nullptr};
    }

    // 'recurse': see if parameter's object type has a constructor taking
    // arg1Objtype as argument
    // @todo error information must be 'recursive', i.e. why wasn't there a
    //       matching constructor for the current argument
    auto ret = param1ObjType.findMemberFun(ctorName, arg1ObjType);
    if (ret.m_status == eFound) { return ret; }
    return {eAnArgMismatches, 1, 0, "", nullptr};
  }
}

RawPtrUnqualifiedObjType::RawPtrUnqualifiedObjType(
  const ObjType& pointeeObjType, EPtrCanBeNull canBeNull, EPtrInit init)
  : UnqualifiedObjType{
      ptrName, 4 /* @todo */, ObjTypes{pointeeObjType}, nullptr /* @todo */} {
}
