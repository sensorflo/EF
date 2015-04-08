#pragma once
#include "entity.h"
#include "access.h"
#include <memory>

class ErrorHandler;
class ObjType;
namespace llvm {
  class Value;
}

class Object : public Entity {
public:
  Object(std::shared_ptr<const ObjType> objType);

  const ObjType& objType() const { return *m_objType.get(); }
  bool hasAnyWriteOrTakeAddrAccess() { return m_hasAnyWriteOrTakeAddrAccess; }
  void addAccess(Access access);

private:
  /** Is guaranteed to be non-null */
  const std::shared_ptr<const ObjType> m_objType;
  bool m_hasAnyWriteOrTakeAddrAccess;

  // decorations for IrGen
public:
  llvm::Value* valueIr() const;
  void setValueIr(llvm::Value* valueIr);
private:
  llvm::Value* m_valueIr;
};

