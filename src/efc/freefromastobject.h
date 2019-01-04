#pragma once
#include "object.h"

#include <memory>

class ObjType;

class FreeFromAstObject : public ConcreteObject {
public:
  FreeFromAstObject(std::shared_ptr<const ObjType>);

  // -- overrides for Object
  const ObjType& objType() const override;
  std::shared_ptr<const ObjType> objTypeAsSp() const override;
  StorageDuration storageDuration() const override;

private:
  std::shared_ptr<const ObjType> m_objType;
};
