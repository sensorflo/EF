#pragma once
#include "storageduration.h"
#include "envnode.h"
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

Multiple AstObject nodes may refer to one Object. */
class Object : public EnvNode {
public:
  Object(StorageDuration storageDuration);
  Object(std::shared_ptr<const ObjType> objType,
    StorageDuration storageDuration);

  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>& os) const override;

  void setObjType(std::shared_ptr<const ObjType> objType);
  const ObjType& objType() const { assert(m_objType); return *m_objType; }
  std::shared_ptr<const ObjType> objTypeAsSp() const { return m_objType; }
  StorageDuration storageDuration() const { return m_storageDuration; }
  void addAccess(Access access);
  bool isModifiedOrRevealsAddr() const;
  /** Semantically every non-abstract object is stored in memory. However
  certain objects can be optimized to live only as an SSA
  value. `isStoredInMemory' is both in the sense of `must be stored in memory'
  (equals `can't be optimized to be a SSA value only') and `actually is stored
  in memory'. */
  bool isStoredInMemory() const;
  bool isInitialized() { return m_isInitialized; }
  void setIsInitialized(bool isInitialized) { m_isInitialized = isInitialized; }

private:
  std::shared_ptr<const ObjType> m_objType;
  const StorageDuration m_storageDuration;
  bool m_isModifiedOrRevealsAddr;
  bool m_isInitialized;

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
