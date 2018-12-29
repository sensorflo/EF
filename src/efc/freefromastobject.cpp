#include "freefromastobject.h"

FreeFromAstObject::FreeFromAstObject(std::shared_ptr<const ObjType> objType)
  : m_objType(move(objType)) {
}

const ObjType& FreeFromAstObject::objType() const {
  assert(m_objType);
  return *m_objType;
}

std::shared_ptr<const ObjType> FreeFromAstObject::objTypeAsSp() const {
  return m_objType;
}

StorageDuration FreeFromAstObject::storageDuration() const {
  return StorageDuration::eUnknown;
}
