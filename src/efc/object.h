#pragma once
#include "entity.h"
#include "access.h"
#include <memory>

class ErrorHandler;
class ObjType;
namespace llvm {
  class Value;
}

/** Represents an object in the target program, however not uniquely. That is
one object in the target program might have multiple associated Object
instances. */
class Object {
public:
  //Object(std::shared_ptr<const ObjType> objType);
  Object();

  //const ObjType& objType() const { return *m_objType.get(); }
  bool wasModifiedOrRevealedAddr() { return m_wasModifiedOrRevealedAddr; }
  void addAccess(Access access);

private:
  // /** Is guaranteed to be non-null */
  // const std::shared_ptr<const ObjType> m_objType;
  bool m_wasModifiedOrRevealedAddr;

  // decorations for IrGen
public:
  llvm::Value* value() const;
  void setValue(llvm::Value* valueIr);
  llvm::Value* address() const;
  void setAddr(llvm::Value* addr);
private:
  llvm::Value* m_valueIr;
};

