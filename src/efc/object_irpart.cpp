#include "object_irpart.h"

#include "object.h"

#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Value.h"

#include <cassert>

using namespace std;
using namespace llvm;

Object_IrPart::Object_IrPart(const Object& obj)
  : m_obj{obj}
  , m_irAddrOfIrObject{nullptr}
  , m_irValueOfObject{nullptr}
  , m_phase{eStart} {
}

void Object_IrPart::setAddrOfIrObject(llvm::Value* irAddrOfIrObject) {
  assert(eStart == m_phase);
  assert(m_obj.isStoredInMemory());
  assert(irAddrOfIrObject);
  assert(!m_irAddrOfIrObject); // doesn't make sense to set it twice
  m_irAddrOfIrObject = irAddrOfIrObject;
  m_phase = eAllocated;
}

void Object_IrPart::irObjectIsAnSsaValue() {
  assert(eStart == m_phase);
  assert(m_obj.storageDuration() == StorageDuration::eLocal);
  assert(!m_obj.isStoredInMemory());
  assert(!m_irValueOfObject); // must not yet be set, will be set in
  // initializeIrObject
  m_phase = eAllocated;
}

void Object_IrPart::initializeIrObject(Value* irValue, IRBuilder<>& builder) {
  assert(eAllocated == m_phase);
  assert(irValue);
  if (m_obj.isStoredInMemory()) {
    assert(m_irAddrOfIrObject);
    if (m_obj.storageDuration() == StorageDuration::eStatic) {
      const auto globalVariable =
        static_cast<GlobalVariable*>(m_irAddrOfIrObject);
      const auto constantInitializer = static_cast<Constant*>(irValue);
      globalVariable->setInitializer(constantInitializer);
    }
    else if (m_obj.storageDuration() == StorageDuration::eLocal) {
      builder.CreateStore(irValue, m_irAddrOfIrObject);
    }
  }
  else {
    assert(!m_irValueOfObject); // doesn't make sense to set it twice
    m_irValueOfObject = irValue;
  }
  m_phase = eInitialized;
}

void Object_IrPart::referToIrObject(llvm::Value* irAddrOfIrObject) {
  assert(eStart == m_phase);
  assert(m_obj.storageDuration() != StorageDuration::eLocal);
  assert(m_obj.isStoredInMemory());
  assert(!m_irAddrOfIrObject); // doesn't make sense to set it twice
  assert(irAddrOfIrObject);
  m_irAddrOfIrObject = irAddrOfIrObject;
  m_phase = eInitialized;
}

Value* Object_IrPart::irValueOfIrObject(
  IRBuilder<>& builder, const string& name) const {
  if (m_obj.storageDuration() == StorageDuration::eLocal) {
    assert(eInitialized == m_phase);
  }
  else {
    assert(eStart != m_phase);
  }
  if (m_obj.isStoredInMemory()) {
    assert(m_irAddrOfIrObject);
    return builder.CreateLoad(m_irAddrOfIrObject, name);
  }
  assert(m_irValueOfObject);
  return m_irValueOfObject;
}

void Object_IrPart::setIrValueOfIrObject(Value* irValue, IRBuilder<>& builder) {
  if (m_obj.storageDuration() == StorageDuration::eLocal) {
    assert(eInitialized == m_phase);
  }
  else {
    assert(eStart != m_phase);
  }
  assert(m_obj.isStoredInMemory()); // IR objects not in memmory are SSA values
  // which are immutable
  assert(m_irAddrOfIrObject);
  assert(irValue);
  builder.CreateStore(irValue, m_irAddrOfIrObject);
}

Value* Object_IrPart::irAddrOfIrObject() const {
  if (m_obj.storageDuration() == StorageDuration::eLocal) {
    assert(eInitialized == m_phase);
  }
  else {
    assert(eStart != m_phase);
  }
  assert(m_obj.isStoredInMemory());
  assert(m_irAddrOfIrObject);
  return m_irAddrOfIrObject;
}
