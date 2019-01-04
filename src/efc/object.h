#pragma once
#include "access.h"
#include "object_irpart.h"
#include "storageduration.h"

#include <memory>

class ObjType;

class Object {
public:
  virtual ~Object() = default;

  // -- new (pure) virtual methods
  virtual const ObjType& objType() const = 0;
  virtual std::shared_ptr<const ObjType> objTypeAsSp() const = 0;
  virtual StorageDuration storageDuration() const = 0;

  virtual void addAccess(Access access) = 0;
  virtual bool isModifiedOrRevealsAddr() const = 0;
  virtual bool isStoredInMemory() const = 0;
  virtual const Object_IrPart& ir() const = 0;
  virtual Object_IrPart& ir() = 0;
};

/** Concrete in the sense of not indirect */
class ConcreteObject : public Object {
public:
  ConcreteObject();
  ~ConcreteObject() override;

  // -- misc
  /** See also AstNode::setAccessFromAstParent /
  AstObject::m_accessFromAstParent. */
  void addAccess(Access access) override;
  bool isModifiedOrRevealsAddr() const override;
  bool isStoredInMemory() const override;

  const Object_IrPart& ir() const override { return m_ir; }
  Object_IrPart& ir() override { return m_ir; }

private:
  /** Combined accesses to the object associated with this AST node. */
  bool m_isModifiedOrRevealsAddr;
  Object_IrPart m_ir;
};
