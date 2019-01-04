#pragma once
#include "access.h"
#include "object_irpart.h"
#include "storageduration.h"

#include <memory>

class ObjType;

class Object {
public:
  Object();
  virtual ~Object() = default;

  // -- new (pure) virtual methods
  virtual const ObjType& objType() const = 0;
  virtual std::shared_ptr<const ObjType> objTypeAsSp() const = 0;
  virtual StorageDuration storageDuration() const = 0;

  // -- misc
  /** See also AstNode::setAccessFromAstParent /
  AstObject::m_accessFromAstParent. */
  void addAccess(Access access);
  bool isModifiedOrRevealsAddr() const;
  bool isStoredInMemory() const;

  const Object_IrPart& ir() const { return m_ir; }
  Object_IrPart& ir() { return m_ir; }

private:
  /** Combined accesses to the object associated with this AST node. */
  bool m_isModifiedOrRevealsAddr;
  Object_IrPart m_ir;
};
