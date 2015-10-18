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

class SymbolTableEntry : public Entity {
public:
  SymbolTableEntry(std::shared_ptr<const ObjType> objType,
    StorageDuration storageDuration);

  const ObjType& objType() const { return *m_objType.get(); }
  StorageDuration storageDuration() const { return m_storageDuration; }
  void addAccessToObject(Access access);
  bool objectIsModifiedOrRevealsAddr() const;
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
  bool m_objectIsModifiedOrRevealsAddr;

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

/** Only for the Object part of SymbolTableEntry */
std::basic_ostream<char>& operator<<(std::basic_ostream<char>& os,
  const SymbolTableEntry& stentry);
