#include "freefromastobject.h"

using namespace std;

FreeFromAstObject::FreeFromAstObject(shared_ptr<const ObjType> objType)
  : m_objType{move(objType)} {
}

const ObjType& FreeFromAstObject::objType() const {
  assert(m_objType);
  return *m_objType;
}

shared_ptr<const ObjType> FreeFromAstObject::objTypeAsSp() const {
  return m_objType;
}

StorageDuration FreeFromAstObject::storageDuration() const {
  return StorageDuration::eUnknown;
}
