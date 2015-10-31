#pragma once
// If you change this list of header files, you must also modify the
// analogous, redundant list in the Makefile
#include "entity.h"
#include <string>
#include <list>
#include <iostream>
#include <memory>
#include <vector>

class ObjTypeQuali;
class ObjTypeFunda;
class ObjTypePtr;
class ObjTypeFun;
class ObjTypeClass;
class ObjTypeSymbol;
class AstObject;
namespace llvm {
  class Type;
  class StructType;
  class LLVMContext;
}

/** Abstract base class for all object types.

Multiple AstObjType nodes may refer to one Entity. */
class ObjType : public Entity, public std::enable_shared_from_this<ObjType> {
public:
  // works as bit flags
  enum Qualifiers {
    eNoQualifier = 0,
    eMutable = 1 
    // later: eVolatile = 1<<1
  };
  enum MatchType {
    eNoMatch,
    eMatchButAllQualifiersAreWeaker,
    eMatchButAnyQualifierIsStronger,
    eFullMatch
    // later: implicit castable
  };
  enum EClass {
    eAbstract,

    eScalar,
      eArithmetic,
        eIntegral,
        eFloatingPoint,

    eStoredAsIntegral,

    eFunction
  };

  bool isVoid() const;
  bool isNoreturn() const;
  bool matchesFully(const ObjType& dst) const;
  bool matchesSaufQualifiers(const ObjType& dst) const;
  /** eMatchButAllQualifiersAreWeaker means that other has weaker qualifiers than
  this, likewise for eMatchButAnyQualifierIsStronger. */
  virtual MatchType match(const ObjType& dst, bool isLevel0 = true) const = 0;
  virtual MatchType match2(const ObjTypeQuali& src, bool isLevel0) const;
  virtual MatchType match2(const ObjTypeFunda& src, bool isLevel0) const { return eNoMatch; }
  virtual MatchType match2(const ObjTypePtr& src, bool isLevel0) const { return eNoMatch; }
  virtual MatchType match2(const ObjTypeFun& src, bool isLevel0) const { return eNoMatch; }
  virtual MatchType match2(const ObjTypeClass& src, bool isLevel0) const { return eNoMatch; }
  virtual MatchType match2(const ObjTypeSymbol& src, bool isLevel0) const { return eNoMatch; }

  virtual bool is(EClass class_) const =0;
  /** Size in bits */
  virtual int size() const =0;

  virtual Qualifiers qualifiers() const { return eNoQualifier; }

  /** The objType of the created AstObject is immutable, since the AstObject has
  the semantics of a temporary. Asserts in case of there is no default
  AstObject */
  virtual AstObject* createDefaultAstObject() const = 0;
  virtual llvm::Type* llvmType() const = 0;

  /** Returns true if this type has the given operator as member function.
  Assumes that the operands are of the same type, except for logical and/or,
  where the rhs additionaly can be of type noreturn. The argument may not be
  eSeq, since that operator is never a member. */
  virtual bool hasMember(int op) const = 0;
  virtual bool hasConstructor(const ObjType& other) const = 0;

  static bool matchesFully_(const ObjType& src, const ObjType& dst);
  static bool matchesSaufQualifiers_(const ObjType& src, const ObjType& dst);

  virtual std::shared_ptr<const ObjType> unqualifiedObjType() const;
};

std::basic_ostream<char>& operator<<(std::basic_ostream<char>& os,
  ObjType::Qualifiers qualifiers);

std::basic_ostream<char>& operator<<(std::basic_ostream<char>& os,
  ObjType::MatchType mt);


class ObjTypeQuali : public ObjType {
public:
  /** If type is of dynamic type ObjTypeQuali, this newly created type points
  to the type pointed by type, and the qualifiers are combined. */
  ObjTypeQuali(const Qualifiers qualifiers, std::shared_ptr<const ObjType> type);

  Qualifiers qualifiers() const override { return m_qualifiers; }

  std::basic_ostream<char>& printTo(std::basic_ostream<char>& os) const override;

  MatchType match(const ObjType& dst, bool isLevel0 = true) const override;
  using ObjType::match2;
  MatchType match2(const ObjTypeQuali& src, bool isRoot) const override;
  MatchType match2(const ObjTypeFunda& src, bool isRoot) const override;
  MatchType match2(const ObjTypePtr& src, bool isRoot) const override;
  MatchType match2(const ObjTypeFun& src, bool isRoot) const override;
  MatchType match2(const ObjTypeSymbol& src, bool isLevel0) const override;
  MatchType match2Quali(const ObjType& type, bool isRoot, bool typeIsSrc) const;

  bool is(EClass class_) const override;
  int size() const override;
  AstObject* createDefaultAstObject() const override;
  llvm::Type* llvmType() const override;
  bool hasMember(int op) const override;
  bool hasConstructor(const ObjType& other) const override;
  std::shared_ptr<const ObjType> unqualifiedObjType() const override;

private:
  const Qualifiers m_qualifiers;
  /** Type being qualified. Is never of dynamic type ObjTypeQuali. Garanteed
  to be non-null */
  const std::shared_ptr<const ObjType> m_type;
};

/** Fundamental type (int, bool, double etc) and also pointer type. 'General
scalar' (note that it also includes abstract objects) might be a better
name. */
class ObjTypeFunda : public ObjType {
public:
  enum EType {
    // abstract objects
    eVoid,
    eNoreturn,
    // concrete objects
    eChar,
    eInt,
    eBool,
    eDouble,
      // implemented in derived classes 
      ePointer
  };

  ObjTypeFunda(EType type);

  virtual MatchType match(const ObjType& dst, bool isLevel0 = true) const;
  using ObjType::match2;
  virtual MatchType match2(const ObjTypeFunda& src, bool isRoot) const;
  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>& os) const;

  virtual bool is(EClass class_) const;
  virtual int size() const;

  EType type() const { return m_type; }
  virtual AstObject* createDefaultAstObject() const;
  virtual llvm::Type* llvmType() const;

  virtual bool hasMember(int op) const;
  virtual bool hasConstructor(const ObjType& other) const;

  bool isValueInRange(double val) const;

private:
  const EType m_type;
};


/** Compond-type/pointer */
class ObjTypePtr : public ObjTypeFunda {
public:
  ObjTypePtr(std::shared_ptr<const ObjType> pointee);

  virtual MatchType match(const ObjType& dst, bool isLevel0 = true) const;
  using ObjType::match2;
  using ObjTypeFunda::match2;
  virtual MatchType match2(const ObjTypePtr& src, bool isLevel0) const;

  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>& os) const;

  virtual llvm::Type* llvmType() const;

  std::shared_ptr<const ObjType> pointee() const;

private:
  /** Guaranteed to be non-null */
  const std::shared_ptr<const ObjType> m_pointee;
};

/** Compound-type/function */
class ObjTypeFun : public ObjType {
public:
  ObjTypeFun(std::list<std::shared_ptr<const ObjType> >* args,
    std::shared_ptr<const ObjType> ret = std::shared_ptr<const ObjType>());

  static std::list<std::shared_ptr<const ObjType> >* createArgs(const ObjType* arg1 = NULL,
    const ObjType* arg2 = NULL, const ObjType* arg3 = NULL);

  virtual MatchType match(const ObjType& dst, bool isLevel0) const;
  using ObjType::match2;
  virtual MatchType match2(const ObjTypeFun& src, bool isLevel0) const;
  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>& os) const;
  virtual AstObject* createDefaultAstObject() const;
  virtual llvm::Type* llvmType() const;
  virtual bool hasMember(int) const { return false; }
  virtual bool hasConstructor(const ObjType& other) const { return false; }

  virtual bool is(EClass class_) const;
  virtual int size() const { return -1;}

  const std::list<std::shared_ptr<const ObjType> >& args() const { return *m_args; }
  const ObjType& ret() const { return *m_ret; }

private:
  /** m_args itself and the pointers in the cointainer are garanteed to be
  non-null. */
  const std::unique_ptr<std::list<std::shared_ptr<const ObjType>>> m_args;
  const std::shared_ptr<const ObjType> m_ret;
};

/** Compount-type/class */
class ObjTypeClass : public ObjType {
public:
  ObjTypeClass(const std::string& name,
    std::vector<std::shared_ptr<const ObjType>>&& members);
  ObjTypeClass(const std::string& name,
    std::shared_ptr<const ObjType> member1 = nullptr,
    std::shared_ptr<const ObjType> member2 = nullptr,
    std::shared_ptr<const ObjType> member3 = nullptr);
  ~ObjTypeClass();

  static std::vector<std::shared_ptr<const ObjType>> createMembers(
    std::shared_ptr<const ObjType> member1 = nullptr,
    std::shared_ptr<const ObjType> member2 = nullptr,
    std::shared_ptr<const ObjType> member3 = nullptr);

  std::basic_ostream<char>& printTo(std::basic_ostream<char>& os) const override;
  MatchType match(const ObjType& dst, bool isLevel0) const override;
  using ObjType::match2;
  MatchType match2(const ObjTypeClass& src, bool isLevel0) const override;
  AstObject* createDefaultAstObject() const override;
  llvm::Type* llvmType() const override;
  void setLlvmType(llvm::StructType* llvmType) const;
  bool hasMember(int) const override;
  bool hasConstructor(const ObjType& other) const override;
  bool is(EClass class_) const override;
  int size() const override;

  const std::string& name() const { return m_name; }
  const std::vector<std::shared_ptr<const ObjType>>& members() const { return m_members; }

private:
  /** Redundant to AstClassDef::m_name */
  const std::string m_name;
  const std::vector<std::shared_ptr<const ObjType>> m_members;
  /** Largely redundant to ObjTypeClass; it's just the llvm version of the
  same idea / information content. We're _not_ the owner. Is mutable because
  it's more like a cache than like a state. */
  mutable llvm::StructType* m_llvmType;
};

class ObjTypeSymbol : public ObjType {
public:
  ObjTypeSymbol(const std::string& name);

  std::basic_ostream<char>& printTo(std::basic_ostream<char>& os) const override;
  MatchType match(const ObjType& dst, bool isLevel0  = true) const override;
  MatchType match2(const ObjTypeSymbol& src, bool isLevel0) const override;
  bool is(EClass class_) const override;
  int size() const override;
  AstObject* createDefaultAstObject() const override;
  llvm::Type* llvmType() const override;
  bool hasMember(int op) const override;
  bool hasConstructor(const ObjType& other) const override;

  const std::string& name() const { return m_name; }

private:
  const std::string m_name;
};
