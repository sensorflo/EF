#pragma once

#include "llvm/IR/IRBuilder.h"

#include <string>

class Object;

enum class EInitStatus { eUNinitialized, eInitialized };

/** The part of Object that is for use by IrGen. Represents an IR object, which
is either an SSA value or memory. */
class Object_IrPart {
public:
  Object_IrPart(const Object& obj);

  /** True if we want to place the object in memory, false if we store it as
  SSA value. */
  bool isSSAValue() const;

  // -- Allocation phase: Note that SSA values are implicitely allocated.

  /** Associates this Object_IrPart with an already allocated IR object. status
  specifies the initialization status of that IR Object. */
  void setAddrOfIrObject(
    llvm::Value* addr, EInitStatus status = EInitStatus::eUNinitialized);

  // -- Initialization phase: Initialize the already allocated but not yet
  //    initialized IR object.

  void initializeIrObject(llvm::Value* irValue, llvm::IRBuilder<>& builder);

  // -- Usage phase: Use the already initialized IR object.
  llvm::Value* irValueOfIrObject(
    llvm::IRBuilder<>& builder, const std::string& name = "") const;
  void setIrValueOfIrObject(llvm::Value* irValue, llvm::IRBuilder<>& builder);
  llvm::Value* irAddrOfIrObject() const;

private:
  enum Phase { eStart, eAllocated, eInitialized };

  const Object& m_obj;

  /** Only used if isSSAValue() is false: points to the IR object */
  llvm::Value* m_irAddrOfIrObject;
  /** Only used if isSSAValue() is true: directly the IR object. I.e. the
  IR object is an SSA value. */
  llvm::Value* m_irValueOfObject;
  /** Tracks whether Object is allocated and initialized. Is not strictly
  required. Is only used for defensive programming to test that the clients does
  not call methods in the wrong order, e.g. uses an uninitialized IR object. */
  Phase m_phase;
};
