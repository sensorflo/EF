#pragma once
#include "access.h"
#include "object_irpart.h"
#include "storageduration.h"

#include <memory>

class ObjType;

/** An Object is a region of storage. It is either a data object or a function (object). */
class Object {
public:
  virtual ~Object() = default;

  virtual const ObjType& objType() const = 0;
  virtual std::shared_ptr<const ObjType> objTypeAsSp() const = 0;
  virtual StorageDuration storageDuration() const = 0;

  virtual void addAccess(Access access) = 0;
  virtual bool isModifiedOrRevealsAddr() const = 0;

  virtual const Object_IrPart& ir() const = 0;
  virtual Object_IrPart& ir() = 0;
};

/** Concrete in the sense of not indirect. Some abstract methods are still left
abstract. */
class ConcreteObject : public virtual Object {
public:
  ConcreteObject();
  ~ConcreteObject() override;

  // -- misc
  /** See also AstNode::setAccessFromAstParent /
  AstObject::m_accessFromAstParent. */
  void addAccess(Access access) override;
  bool isModifiedOrRevealsAddr() const override;

  const Object_IrPart& ir() const override { return m_ir; }
  Object_IrPart& ir() override { return m_ir; }

private:
  /** Combined accesses to the object associated with this AST node. */
  bool m_isModifiedOrRevealsAddr;
  Object_IrPart m_ir;
};

/** Meant as convenience class so not each class wanting to implement the
Object interface in a trivial way has to do it again himself. Thus the members
variables are public */
class FullConcreteObject : public ConcreteObject {
public:
  FullConcreteObject() = default;
  FullConcreteObject(
    std::shared_ptr<const ObjType> objType, StorageDuration storageDuration)
    : m_objType{move(objType)}, m_storageDuration{storageDuration} {}

  // clang-format off
  const ObjType& objType() const override { return *m_objType; }
  std::shared_ptr<const ObjType> objTypeAsSp() const override { return m_objType; }
  StorageDuration storageDuration() const override { return m_storageDuration; }
  // clang-format on

  std::shared_ptr<const ObjType> m_objType;
  StorageDuration m_storageDuration = StorageDuration::eYetUndefined;
};

/** An Object that delegates all calls to an referenced Object */
class ObjectDelegate : public virtual Object {
public:
  virtual Object& referencedObj() const = 0;

  // clang-format off
  const ObjType& objType() const override { return referencedObj().objType(); }
  std::shared_ptr<const ObjType> objTypeAsSp() const override { return referencedObj().objTypeAsSp(); }
  StorageDuration storageDuration() const override { return referencedObj().storageDuration(); };
  void addAccess(Access access) override { return referencedObj().addAccess(access); };
  bool isModifiedOrRevealsAddr() const override { return referencedObj().isModifiedOrRevealsAddr(); };
  const Object_IrPart& ir() const override { return referencedObj().ir(); };
  Object_IrPart& ir() override { return referencedObj().ir(); };
  // clang-format on
};
