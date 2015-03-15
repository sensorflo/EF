#ifndef OBJTYPE_H
#define OBJTYPE_H
#include <string>
#include <list>
#include <iostream>
#include <memory>

class ObjTypeFunda;
class ObjTypeFun;
class AstValue;
class AstOperator;

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
    eOnlyQualifierMismatches,
    eFullMatch
    // later: implicit castable
  };
  virtual ~ObjType() {};

  ObjType& addQualifiers(Qualifiers qualifiers);
  ObjType& removeQualifiers(Qualifiers qualifiers);

  bool matchesFully(const ObjType& other) const;
  bool matchesSaufQualifiers(const ObjType& other) const;
  virtual MatchType match(const ObjType& other) const = 0;
  virtual MatchType match2(const ObjTypeFunda& other) const { return eNoMatch; }
  virtual MatchType match2(const ObjTypeFun& other) const { return eNoMatch; }

  virtual ObjType* clone() const = 0;

  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>& os) const = 0;
  std::string toStr() const;

  Qualifiers qualifiers() const { return m_qualifiers; }

  virtual AstValue* createDefaultAstValue() const = 0;

  /** Returns true if this type has the given operator as member function.
  Assumes that the operands are of the same type. The argument may not be
  eSeq, since that operator is never a member. */
  virtual bool hasMember(int op) const = 0;

  static bool matchesFully_(const ObjType& rhs, const ObjType& lhs);
  static bool matchesSaufQualifiers_(const ObjType& rhs, const ObjType& lhs);

protected:
  ObjType(Qualifiers qualifiers) : m_qualifiers(qualifiers) {};
  ObjType(const ObjType& rhs) : m_qualifiers(rhs.m_qualifiers) {};

  Qualifiers m_qualifiers;
};

inline std::basic_ostream<char>& operator<<(std::basic_ostream<char>& os,
  const ObjType& ot) {
  return ot.printTo(os);
}

std::basic_ostream<char>& operator<<(std::basic_ostream<char>& os,
  ObjType::Qualifiers qualifiers);

std::basic_ostream<char>& operator<<(std::basic_ostream<char>& os,
  ObjType::MatchType mt);


/** Fundamental type: int, bool, double etc */
class ObjTypeFunda : public ObjType {
public:
  enum EType {
    eInt,
    eBool
    // later: double etc
  };

  ObjTypeFunda(EType type, Qualifiers qualifiers = eNoQualifier) :
    ObjType(qualifiers), m_type(type) {};
  ObjTypeFunda(const ObjTypeFunda& rhs) :
    ObjType(rhs), m_type{rhs.m_type} {};

  virtual MatchType match(const ObjType& other) const { return other.match2(*this); }
  using ObjType::match2;
  virtual MatchType match2(const ObjTypeFunda& other) const;
  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>& os) const;

  virtual ObjTypeFunda* clone() const;

  EType type() const { return m_type; }
  virtual AstValue* createDefaultAstValue() const;

  virtual bool hasMember(int op) const;

  bool isValueInRange(int val) const;

private:
  EType m_type;
};


/** Compound-type/function */
class ObjTypeFun : public ObjType {
public:
  ObjTypeFun(std::list<std::shared_ptr<const ObjType> >* args,
    std::shared_ptr<const ObjType> ret = std::shared_ptr<const ObjType>());
  static std::list<std::shared_ptr<const ObjType> >* createArgs(const ObjType* arg1 = NULL,
    const ObjType* arg2 = NULL, const ObjType* arg3 = NULL);

  virtual MatchType match(const ObjType& other) const { return other.match2(*this); }
  using ObjType::match2;
  virtual MatchType match2(const ObjTypeFun& other) const;
  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>& os) const;
  virtual AstValue* createDefaultAstValue() const;
  virtual bool hasMember(int) const { return false; }

  virtual ObjTypeFun* clone() const;

  const std::list<std::shared_ptr<const ObjType> >& args() const { return *m_args; }

private:
  /** We're the owner of the container object. m_args itself and the pointers
  in the cointainer are garanteed to be non-null. */
  const std::list<std::shared_ptr<const ObjType> >* const m_args;
  const std::shared_ptr<const ObjType> m_ret;
};

#endif
