#pragma once

#include "llvm/IR/IRBuilder.h"

#include <string>

class Object;

/** The part of Object that is for use by IrGen */
class Object_IrPart {
public:
  Object_IrPart(const Object& obj);

  bool isStoredInMemory() const;

  // -- either allocate and initialize IR object or refer to an already
  //    existing IR Object:

  // ---- allocate IR object
  void setAddrOfIrObject(llvm::Value* irAddrOfIrObject);
  void irObjectIsAnSsaValue();

  // ---- initialize (allocated) IR object
  void initializeIrObject(llvm::Value* irValue, llvm::IRBuilder<>& builder);

  // ---- refer to an already existing IR object
  /** As setAddrOfIrObject, only that setAddrOfIrObject must be followed
  by an initialization of the IR object whereas referToIrObject does notl. */
  void referToIrObject(llvm::Value* irAddrOfIrObject);

  // -- use (initialized) IR object
  llvm::Value* irValueOfIrObject(
    llvm::IRBuilder<>& builder, const std::string& name = "") const;
  void setIrValueOfIrObject(llvm::Value* irValue, llvm::IRBuilder<>& builder);
  llvm::Value* irAddrOfIrObject() const;

private:
  enum Phase { eStart, eAllocated, eInitialized };

  const Object& m_obj;

  /** Only used if isStoredInMemory() is true: points to the IR object */
  llvm::Value* m_irAddrOfIrObject;
  /** Only used if isStoredInMemory() is false: directly the IR object. I.e. the
  IR object is an SSA value. */
  llvm::Value* m_irValueOfObject;
  Phase m_phase;
};
