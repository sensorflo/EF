#ifndef OBJTYPE_H
#define OBJTYPE_H
#include <string>

class ObjType {
public:
  enum EType {
    eInt
    // later: more fundamental types. Compound types such as pointer, array,
    // enum, struct, class etc
  };
  enum EQualifier {
    eConst, 
    eMutable 
    // later: eVolatile
  };

  ObjType(EQualifier qualifier = eConst) : m_qualifier(qualifier) {};
  EQualifier qualifier() const { return m_qualifier; }

private:
  // later: for compound types, reference to depended-on ObjType
  // later: store name of custom type
  EQualifier m_qualifier;
};

#endif
