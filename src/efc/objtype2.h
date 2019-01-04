#pragma once
#include "envnode.h"

#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <vector>

namespace llvm {
class Type;
}

// defined below
class ObjType;
class UnqualifiedObjType;
class FunUnqualifiedObjType;
class RawPtrUnqualifiedObjType;

/* An Object that is guaranteed to have exactly one member fun, a function call
operator. Maybe use AstFunDef for now. It's UnqualifiedObjType is FunUnqualifiedObjType */
class FunctionObj : public EnvNode {
public:
  FunctionObj(std::string name) : EnvNode(move(name)) {}

  // @todo add signature
  std::string description() const override { return "function " + name(); }
  FunUnqualifiedObjType& objType();
};

template<class T>
using rw = std::reference_wrapper<T>;

using ObjTypes = std::vector<rw<const ObjType>>;

// @todo: move to some more general place. also let scanner use it
template<typename T>
class Deleter {
public:
  void operator()(T* p) const { delete p; }
};

constexpr auto typeHasNoSize = size_t{std::numeric_limits<size_t>::max()};
constexpr auto ctorName = "ctor";
constexpr auto dtorName = "dtor";
constexpr auto funCallOpName = "()";
constexpr auto ptrName = "ptr";

/** Wether or not the pointer can be null */
enum class EPtrCanBeNull { eNo, eYes };

/** How the pointer is initialized */
enum class EPtrInit {
  /** Normal, that is as any other type */
  eNormal,
  /** Automatically the address is taken of the initializer, and from there on
  the normal rules apply. */
  eAutoTakeAddr
};

enum EFindFunStatus {
  eFound,
  eNameNotFound,
  // msgParam3 = obj type as string
  eNotAFunction,
  // msgParam1 = numberOfPassed args, msgParam2 = numberOfExpectedArgs
  eNumberOfArgsMismatches,
  // msgParam1 = offending argument
  eAnArgMismatches
};

struct FindFunResult {
  EFindFunStatus m_status;
  size_t m_msgParam1;
  size_t m_msgParam2;
  std::string m_msgParam3;
  FunctionObj* m_funObj;
  // @todo: ptrs to constructors for each arg, info which args must be
  //      autodereferenced. But note that constructing one param might be given
  //      by a whole ct_list tree. Actually maybe its simpler if we insert all the
  //      necessairy ctor calls and auto derefs ourselves.
};

/** Helper class for ObjType, see there.
@todo Consider renaming 'Raw' to 'WQ' for 'without qualifiers', or 'UQ' for
      'unqualified' (UQObjectType, UnqualifiedObjType) */
class UnqualifiedObjType : public EnvNode {
public:
  // @todo: also template args
  std::string description() const override { return name(); }

  size_t size() const { return m_size; }
  const ObjTypes& templateArgs() const { return m_templateArgs; }
  llvm::Type* llvmType() const { return m_llvmType; }

  virtual bool isRawPtr() const { return false; }

protected:
  /* name (see EnvNode::name()) is the name of the template whose instantiation
  resulted in this type, plus encoded template arguments. templateArgs are the
  template arguments which caused the instantiation of this object type. */
  UnqualifiedObjType(
    std::string name, size_t size, ObjTypes templateArgs, llvm::Type* llvmType);
  ~UnqualifiedObjType() override;

private:
  friend class ObjTypeTemplate;
  friend class Deleter<UnqualifiedObjType>;

  /** size in bytes, or typeHasNoSize if type has no size. */
  const size_t m_size;
  /** See ctor. @todo Maybe only needed for matching pointers? If so, could be
  empty for all others to save memory. Or move to specialed RawPtrObjType class */
  ObjTypes m_templateArgs;

  // -- decorations for IrGen
  llvm::Type* const m_llvmType;
};

/** A concrete (that is after template instantiation) object type, inclusive
qualifiers. */
class ObjType {
public:
  // works as bit flags. non-zero is makes a qualifier stronger
  enum Qualifiers {
    eNoQualifier = 0,
    eMutable = 1
    // later: eVolatile = 1<<1
  };
  enum MatchType {
    eNoMatch, // error rational might contain info where in the chain a mismatch happened
    eMatchButAllQualifiersAreWeaker, // considering only top level qualifiers
    eMatchButAnyQualifierIsStronger, // considering only top level qualifiers
    eFullMatch // considering only top level qualifiers
    // later: implicit castable
  };

  const std::string& fqName() const { return m_unqualified.fqName(); }

  /** Currently, 'match' is in the context of matching argument-type -
  parameter-type */
  MatchType match(const ObjType& dst) const;
  bool matchesFully(const ObjType& dst) const;
  /** @todo: rename sauf -> except */
  bool matchesExceptQualifiers(const ObjType& dst) const;

  // todo: rename to defaultObjType. or even something better. Its currently
  // always (I think) used for (simple) type deduction.
  std::shared_ptr<const ObjType> unqualifiedObjType() const;
  Qualifiers qualifiers() const { return m_qualifiers; }

  // -- delegating to UnqualifiedObjType
  enum EClass {
    // not required to know hopefully
    // eAbstract, eScalar, eArithmetic, eIntegral, eFloatingPoint

    eStoredAsIntegral, // applies also to unity types. only as summary for
    // respective builtin types, so not always a full list of
    // or comparisions must be made

    eNotStoredAsIntegral
  };

  /** Returns the Function that would be called given the argument arg1, or
  nullptr if there is no such function obj. The callee is not the owner of the
  pointee.

  @todo: Each arg is actualy a ct_list (tree) */
  FindFunResult findMemberFun(
    const std::string& name, const ObjType& arg1) const;
  // todo: memberVar(const std::string& name) const

  /** see m_raw.m_size */
  int size() const { return m_unqualified.size(); }
  /** see m_raw.m_llvmType */
  llvm::Type* llvmType() const { return m_unqualified.llvmType(); }

  // convenience methods wrapping match
  bool isVoid() const;
  bool isNoreturn() const;
  bool isRawPtr() const { return m_unqualified.isRawPtr(); }
  bool is(EClass class_) const;

  // should be part of envnode, no?
  // virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>& os) const = 0;

protected:
  ObjType(UnqualifiedObjType& unqualified, Qualifiers qualifiers);

private:
  friend class ObjTypeTemplate;
  friend class Deleter<ObjType>;

  UnqualifiedObjType& m_unqualified;
  Qualifiers m_qualifiers;
};

/* Guaranteed to have only one member, which is a member function having the
name of the function call operator. The type of that function is exactly this
FunObjType. */
class FunUnqualifiedObjType : public UnqualifiedObjType {
public:
  const ObjType& retObjType() const;
  ObjTypes::const_iterator paramObjTypesBegin() const;
  ObjTypes::const_iterator paramObjTypesEnd() const;

  // for simplicity, start with one argument
  FindFunResult matchArguments(const ObjType& arg1ObjType) const;

private:
  friend class ObjTypeTemplate;

  FunUnqualifiedObjType(const ObjType& retObjType, ObjTypes paramObjTypes);
};

class RawPtrUnqualifiedObjType : public UnqualifiedObjType {
public:
  bool isRawPtr() const override { return true; }
  const ObjType& pointeeObjType() const { return templateArgs().front(); }
  EPtrCanBeNull canBeNull() const { return m_canBeNull; }
  EPtrInit init() const { return m_init; }

private:
  friend class ObjTypeTemplate;

  RawPtrUnqualifiedObjType(
    const ObjType& pointeeObjType, EPtrCanBeNull canBeNull, EPtrInit init);

  // @todo m_canBeNull, m_init should be template arguments. but currently
  //       template arguments can only be ObjType.

  EPtrCanBeNull m_canBeNull;
  EPtrInit m_init;
};
