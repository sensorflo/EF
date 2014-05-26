#ifndef OBJTYPE_H
#define OBJTYPE_H
#include <string>
#include <list>
#include <iostream>

class ObjTypeFunda;
class ObjTypeFun;

/** Abstract base class for all object types */
class ObjType {
public:
  // works as bit flags
  enum Qualifier {
    eConst   = 0,
    eMutable = 1 
    // later: eVolatile = 1<<1, eNonVolatile = 0
  };
  enum MatchType {
    eNoMatch,
    eOnlyQualifierMismatches,
    eFullMatch
    // later: implicit castable
  };
  virtual ~ObjType() {};

  virtual MatchType match(const ObjType& other) const = 0;
  virtual MatchType match2(const ObjTypeFunda& other) const { return eNoMatch; }
  virtual MatchType match2(const ObjTypeFun& other) const { return eNoMatch; }
  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>& os) const = 0;
  std::string toStr() const;

  Qualifier qualifier() const { return m_qualifier; }

protected:
  ObjType(Qualifier qualifier) : m_qualifier(qualifier) {};

  Qualifier m_qualifier;
};

inline std::basic_ostream<char>& operator<<(std::basic_ostream<char>& os,
  const ObjType& ot) {
  return ot.printTo(os);
}

std::basic_ostream<char>& operator<<(std::basic_ostream<char>& os,
  ObjType::MatchType mt);


/** Fundamental type: int, bool, double etc */
class ObjTypeFunda : public ObjType {
public:
  enum EType {
    eInt
    // later: bool, double etc
  };

  ObjTypeFunda(EType type, Qualifier qualifier = eConst) :
    ObjType(qualifier), m_type(type) {};

  virtual MatchType match(const ObjType& other) const { return other.match2(*this); }
  virtual MatchType match2(const ObjTypeFunda& other) const;
  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>& os) const;

  EType type() const { return m_type; }

private:
  EType m_type;
};


/** Compound-type/function */
class ObjTypeFun : public ObjType {
public:
  ObjTypeFun(std::list<ObjType*>* args, ObjType* ret);
  virtual ~ObjTypeFun();
  static std::list<ObjType*>* createArgs(ObjType* arg1 = NULL, ObjType* arg2 = NULL,
    ObjType* arg3 = NULL);

  virtual MatchType match(const ObjType& other) const { return other.match2(*this); }
  virtual MatchType match2(const ObjTypeFun& other) const;
  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>& os) const;

private:
  /** We're the owner of the container object and the objects pointed to by
  the members of the container. m_args itself and the pointers in the
  cointainer are garanteed to be non-null. */
  const std::list<ObjType*>* const m_args;
  /** */
  const ObjType* const m_ret;
};

#endif
