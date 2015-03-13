#ifndef OBJTYPE_H
#define OBJTYPE_H
#include <string>
#include <list>
#include <iostream>
#include <memory>

class ObjTypeFunda;
class ObjTypeFun;
class AstValue;

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

  bool matchesFully(const ObjType& other) const;
  bool matchesSaufQualifiers(const ObjType& other) const;
  virtual MatchType match(const ObjType& other) const = 0;
  virtual MatchType match2(const ObjTypeFunda& other) const { return eNoMatch; }
  virtual MatchType match2(const ObjTypeFun& other) const { return eNoMatch; }

  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>& os) const = 0;
  std::string toStr() const;

  Qualifiers qualifiers() const { return m_qualifiers; }

  virtual AstValue* createDefaultAstValue() const = 0;

protected:
  ObjType(Qualifiers qualifiers) : m_qualifiers(qualifiers) {};

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

  virtual MatchType match(const ObjType& other) const { return other.match2(*this); }
  using ObjType::match2;
  virtual MatchType match2(const ObjTypeFunda& other) const;
  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>& os) const;

  EType type() const { return m_type; }
  virtual AstValue* createDefaultAstValue() const;

  bool isValueInRange(int val) const;

private:
  EType m_type;
};


/** Compound-type/function */
class ObjTypeFun : public ObjType {
public:
  ObjTypeFun(std::list<std::shared_ptr<const ObjType> >* args, const ObjType* ret);
  virtual ~ObjTypeFun();
  static std::list<std::shared_ptr<const ObjType> >* createArgs(const ObjType* arg1 = NULL,
    const ObjType* arg2 = NULL, const ObjType* arg3 = NULL);

  virtual MatchType match(const ObjType& other) const { return other.match2(*this); }
  using ObjType::match2;
  virtual MatchType match2(const ObjTypeFun& other) const;
  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>& os) const;
  virtual AstValue* createDefaultAstValue() const;

private:
  /** We're the owner of the container object. m_args itself and the pointers
  in the cointainer are garanteed to be non-null. */
  const std::list<std::shared_ptr<const ObjType> >* const m_args;
  /** */
  const ObjType* const m_ret;
};

#endif
