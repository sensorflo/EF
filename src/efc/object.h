#pragma once
#include "storageduration.h"
#include "llvm/IR/IRBuilder.h"
#include <string>
#include <memory>

class ObjType;
class ErrorHandler;
namespace llvm {
  class Value;
}

class Object {
public:
  Object();
  virtual ~Object() = default;

  // -- new virtual methods
  virtual const ObjType& objType() const =0;
  virtual std::shared_ptr<const ObjType> objTypeAsSp() const =0;
  virtual StorageDuration storageDuration() const =0;

  // -- misc
  /** See also AstNode::setAccessFromAstParent /
  AstObject::m_accessFromAstParent. */
  void addAccess(Access access);
  bool isModifiedOrRevealsAddr() const;
  bool isStoredInMemory() const;

private:
  /** Combined accesses to the object associated with this AST node. */
  bool m_isModifiedOrRevealsAddr;

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
