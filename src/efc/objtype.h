#pragma once
// If you change this list of header files, you must also modify the
// analogous, redundant list in the Makefile

#include <string>
#include <list>
#include <iostream>
#include <memory>

class ObjTypeFunda;
class ObjTypePtr;
class ObjTypeFun;
class AstValue;
namespace llvm {
  class Type;
}

/** Abstract base class for all object types */
class ObjType {
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
  virtual ~ObjType() {};

  ObjType& addQualifiers(Qualifiers qualifiers);
  ObjType& removeQualifiers(Qualifiers qualifiers);

  bool isVoid() const;
  bool isNoreturn() const;
  bool matchesFully(const ObjType& dst) const;
  bool matchesSaufQualifiers(const ObjType& dst) const;
  /** eMatchButAllQualifiersAreWeaker means that other has weaker qualifiers than
  this, likewise for eMatchButAnyQualifierIsStronger. */
  virtual MatchType match(const ObjType& dst, bool isLevel0 = true) const = 0;
  virtual MatchType match2(const ObjTypeFunda& src, bool isLevel0) const { return eNoMatch; }
  virtual MatchType match2(const ObjTypePtr& src, bool isLevel0) const { return eNoMatch; }
  virtual MatchType match2(const ObjTypeFun& src, bool isLevel0) const { return eNoMatch; }

  virtual bool is(EClass class_) const =0;
  /** Size in bits */
  virtual int size() const =0;

  virtual ObjType* clone() const = 0;

  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>& os) const = 0;
  std::string toStr() const;

  Qualifiers qualifiers() const { return m_qualifiers; }

  /** The objType of the created AstValue is immutable, since the AstValue has
  the semantics of a temporary. Asserts in case of there is no default
  AstValue */
  virtual AstValue* createDefaultAstValue() const = 0;
  virtual llvm::Type* llvmType() const = 0;

  /** Returns true if this type has the given operator as member function.
  Assumes that the operands are of the same type, except for logical and/or,
  where the rhs additionaly can be of type noreturn. The argument may not be
  eSeq, since that operator is never a member. */
  virtual bool hasMember(int op) const = 0;
  virtual bool hasConstructor(const ObjType& other) const = 0;

  static bool matchesFully_(const ObjType& src, const ObjType& dst);
  static bool matchesSaufQualifiers_(const ObjType& src, const ObjType& dst);

protected:
  ObjType(Qualifiers qualifiers) : m_qualifiers(qualifiers) {};
  ObjType(const ObjType& rhs) : m_qualifiers(rhs.m_qualifiers) {};

  Qualifiers m_qualifiers;

private:
  ObjType& operator=(const ObjType&) =delete; // for simplicity reasons
};

inline std::basic_ostream<char>& operator<<(std::basic_ostream<char>& os,
  const ObjType& ot) {
  return ot.printTo(os);
}

std::basic_ostream<char>& operator<<(std::basic_ostream<char>& os,
  ObjType::Qualifiers qualifiers);

std::basic_ostream<char>& operator<<(std::basic_ostream<char>& os,
  ObjType::MatchType mt);


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

  ObjTypeFunda(EType type, Qualifiers qualifiers = eNoQualifier);

  virtual MatchType match(const ObjType& dst, bool isLevel0 = true) const;
  using ObjType::match2;
  virtual MatchType match2(const ObjTypeFunda& src, bool isRoot) const;
  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>& os) const;

  virtual bool is(EClass class_) const;
  virtual int size() const;

  virtual ObjTypeFunda* clone() const;

  EType type() const { return m_type; }
  virtual AstValue* createDefaultAstValue() const;
  virtual llvm::Type* llvmType() const;

  virtual bool hasMember(int op) const;
  virtual bool hasConstructor(const ObjType& other) const;

  bool isValueInRange(double val) const;

private:
  ObjTypeFunda& operator=(const ObjTypeFunda&) =delete; // for simplicity reasons
  const EType m_type;
};


/** Compond-type/pointer */
class ObjTypePtr : public ObjTypeFunda {
public:
  ObjTypePtr(std::shared_ptr<const ObjType> pointee,
    Qualifiers qualifiers = eNoQualifier);
  ObjTypePtr(const ObjTypePtr&);

  virtual MatchType match(const ObjType& dst, bool isLevel0 = true) const;
  using ObjType::match2;
  using ObjTypeFunda::match2;
  virtual MatchType match2(const ObjTypePtr& src, bool isLevel0) const;

  virtual ObjTypePtr* clone() const;
  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>& os) const;

  virtual AstValue* createDefaultAstValue() const;
  virtual llvm::Type* llvmType() const;

  const ObjType& pointee() const;

private:
  ObjTypePtr& operator=(const ObjTypePtr&) =delete; // for simplicity reasons

  /** Guaranteed to be non-null */
  const std::shared_ptr<const ObjType> m_pointee;
};

/** Compound-type/function */
class ObjTypeFun : public ObjType {
public:
  ObjTypeFun(std::list<std::shared_ptr<const ObjType> >* args,
    std::shared_ptr<const ObjType> ret = std::shared_ptr<const ObjType>());
  ObjTypeFun(const ObjTypeFun&);
  static std::list<std::shared_ptr<const ObjType> >* createArgs(const ObjType* arg1 = NULL,
    const ObjType* arg2 = NULL, const ObjType* arg3 = NULL);

  virtual MatchType match(const ObjType& dst, bool isLevel0) const;
  using ObjType::match2;
  virtual MatchType match2(const ObjTypeFun& src, bool isLevel0) const;
  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>& os) const;
  virtual AstValue* createDefaultAstValue() const;
  virtual llvm::Type* llvmType() const;
  virtual bool hasMember(int) const { return false; }
  virtual bool hasConstructor(const ObjType& other) const { return false; }

  virtual bool is(EClass class_) const;
  virtual int size() const { return -1;}

  virtual ObjTypeFun* clone() const;

  const std::list<std::shared_ptr<const ObjType> >& args() const { return *m_args; }
  const ObjType& ret() const { return *m_ret; }

private:
  ObjTypePtr& operator=(const ObjTypePtr&) =delete; // for simplicity reasons

  /** m_args itself and the pointers in the cointainer are garanteed to be
  non-null. */
  const std::unique_ptr<std::list<std::shared_ptr<const ObjType>>> m_args;
  const std::shared_ptr<const ObjType> m_ret;
};
