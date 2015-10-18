#pragma once
#include "storageduration.h"
#include "entity.h"
#include "llvm/IR/IRBuilder.h"
#include <string>
#include <memory>
#include <ostream>

class ObjType;
class ErrorHandler;
namespace llvm {
  class Value;
}

/** Represents an object in the target program, however not uniquely. That is
one object in the target program might have multiple associated Object
instances. Actually we would want a one to one relationship, but that's not
feasible.

Multiple AstValue nodes may refer to one Entity. */
class Object : public Entity {
public:
  Object(std::shared_ptr<const ObjType> objType,
    StorageDuration storageDuration);

  const ObjType& objType() const { return *m_objType.get(); }
  StorageDuration storageDuration() const { return m_storageDuration; }
  void addAccess(Access access);
  bool isModifiedOrRevealsAddr() const;
  /** Semantically every non-abstract object is stored in memory. However
  certain objects can be optimized to live only as an SSA
  value. `isStoredInMemory' is both in the sense of `must be stored in memory'
  (equals `can't be optimized to be a SSA value only') and `actually is stored
  in memory'. */
  bool isStoredInMemory() const;
  virtual std::string toStr() const override;

private:
  /** Is guaranteed to be non-null */
  const std::shared_ptr<const ObjType> m_objType;
  const StorageDuration m_storageDuration;
  bool m_isModifiedOrRevealsAddr;

  // decorations for IrGen
public:
  void irInitLocal(llvm::Value* irValue, llvm::IRBuilder<>& builder,
    const std::string& name = "");
  llvm::Value* irValue(llvm::IRBuilder<>& builder, const std::string& name = "") const;
  void setIrValue(llvm::Value* irValue, llvm::IRBuilder<>& builder);
  llvm::Value* irAddr() const;
  void setIrAddr(llvm::Value* addr);

private:
  /** is an irValue for isStoredInMemory() being false and an irAddr
  otherwise */
  llvm::Value* m_irValueOrAddr;
};

std::basic_ostream<char>& operator<<(std::basic_ostream<char>& os,
  const Object& obj);
