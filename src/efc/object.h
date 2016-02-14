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

  // -- decorations for IrGen
  // Here an 'IR object' is of LLVM first class value type. An IR object is
  // stored either directly as an (LLVM SSA aka IR) value, or in memory.
public:
  // ---- either allocate and initialize IR object or refer to an already
  //      existing IR Object:

  // ------ allocate IR object
  void setAddrOfIrObject(llvm::Value* irAddrOfIrObject);
  void irObjectIsAnSsaValue();

  // ------ initialize (allocated) IR object
  void initializeIrObject(llvm::Value* irValue, llvm::IRBuilder<>& builder);

  // ------ refer to an already existing IR object
  /** As setAddrOfIrObject, only that setAddrOfIrObject must be followed
  by an initialization of the IR object whereas referToIrObject does notl. */
  void referToIrObject(llvm::Value* irAddrOfIrObject);

  // ---- use (initialized) IR object
  llvm::Value* irValueOfIrObject(llvm::IRBuilder<>& builder,
    const std::string& name = "") const;
  void setIrValueOfIrObject(llvm::Value* irValue, llvm::IRBuilder<>& builder);
  llvm::Value* irAddrOfIrObject() const;

private:
  enum Phase { eStart, eAllocated, eInitialized };
  union {
    /** in case isStoredInMemory() is true: points to the IR object */
    llvm::Value* m_irAddrOfIrObject;
    /** in case isStoredInMemory() is false: directly the IR object. I.e. the
    IR object is an SSA value. */
    llvm::Value* m_irValueOfObject;
  };
  Phase m_phase;
};
