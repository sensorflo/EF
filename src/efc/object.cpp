#include "object.h"

#include "errorhandler.h"

#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Value.h"

#include <cassert>
#include <sstream>

using namespace std;
using namespace llvm;

Object::Object()
  : m_isModifiedOrRevealsAddr{false}
  , m_irValueOfObject{nullptr}
  , m_phase{eStart} {
}

void Object::addAccess(Access access) {
  m_isModifiedOrRevealsAddr = m_isModifiedOrRevealsAddr ||
    access == Access::eWrite || access == Access::eTakeAddress;
}

bool Object::isModifiedOrRevealsAddr() const {
  return m_isModifiedOrRevealsAddr;
}

bool Object::isStoredInMemory() const {
  assert(storageDuration() != StorageDuration::eYetUndefined);
  return storageDuration() != StorageDuration::eLocal ||
    m_isModifiedOrRevealsAddr;
}

void Object::setAddrOfIrObject(llvm::Value* irAddrOfIrObject) {
  assert(eStart == m_phase);
  assert(isStoredInMemory());
  assert(irAddrOfIrObject);
  assert(!m_irAddrOfIrObject); // doesn't make sense to set it twice
  m_irAddrOfIrObject = irAddrOfIrObject;
  m_phase = eAllocated;
}

void Object::irObjectIsAnSsaValue() {
  assert(eStart == m_phase);
  assert(storageDuration() == StorageDuration::eLocal);
  assert(!isStoredInMemory());
  assert(!m_irValueOfObject); // must not yet be set, will be set in
    // initializeIrObject
  m_phase = eAllocated;
}

void Object::initializeIrObject(Value* irValue, IRBuilder<>& builder) {
  assert(eAllocated == m_phase);
  assert(irValue);
  if (isStoredInMemory()) {
    assert(m_irAddrOfIrObject);
    if (storageDuration() == StorageDuration::eStatic) {
      const auto globalVariable =
        static_cast<GlobalVariable*>(m_irAddrOfIrObject);
      const auto constantInitializer = static_cast<Constant*>(irValue);
      globalVariable->setInitializer(constantInitializer);
    }
    else if (storageDuration() == StorageDuration::eLocal) {
      builder.CreateStore(irValue, m_irAddrOfIrObject);
    }
  }
  else {
    assert(!m_irValueOfObject); // doesn't make sense to set it twice
    m_irValueOfObject = irValue;
  }
  m_phase = eInitialized;
}

void Object::referToIrObject(llvm::Value* irAddrOfIrObject) {
  assert(eStart == m_phase);
  assert(storageDuration() != StorageDuration::eLocal);
  assert(isStoredInMemory());
  assert(!m_irAddrOfIrObject); // doesn't make sense to set it twice
  assert(irAddrOfIrObject);
  m_irAddrOfIrObject = irAddrOfIrObject;
  m_phase = eInitialized;
}

Value* Object::irValueOfIrObject(
  IRBuilder<>& builder, const string& name) const {
  if (storageDuration() == StorageDuration::eLocal) {
    assert(eInitialized == m_phase);
  }
  else {
    assert(eStart != m_phase);
  }
  if (isStoredInMemory()) {
    assert(m_irAddrOfIrObject);
    return builder.CreateLoad(m_irAddrOfIrObject, name);
  }
  else {
    assert(m_irValueOfObject);
    return m_irValueOfObject;
  }
}

void Object::setIrValueOfIrObject(Value* irValue, IRBuilder<>& builder) {
  if (storageDuration() == StorageDuration::eLocal) {
    assert(eInitialized == m_phase);
  }
  else {
    assert(eStart != m_phase);
  }
  assert(isStoredInMemory()); // IR objects not in memmory are SSA values
    // which are immutable
  assert(m_irAddrOfIrObject);
  assert(irValue);
  builder.CreateStore(irValue, m_irAddrOfIrObject);
}

Value* Object::irAddrOfIrObject() const {
  if (storageDuration() == StorageDuration::eLocal) {
    assert(eInitialized == m_phase);
  }
  else {
    assert(eStart != m_phase);
  }
  assert(isStoredInMemory());
  assert(m_irAddrOfIrObject);
  return m_irAddrOfIrObject;
}
