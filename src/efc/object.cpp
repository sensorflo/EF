#include "object.h"

ConcreteObject::ConcreteObject()
  : m_isModifiedOrRevealsAddr{false}, m_ir{*this} {
}

ConcreteObject::~ConcreteObject() = default;

void ConcreteObject::addAccess(Access access) {
  m_isModifiedOrRevealsAddr = m_isModifiedOrRevealsAddr ||
    access == Access::eWrite || access == Access::eTakeAddress;
}

bool ConcreteObject::isModifiedOrRevealsAddr() const {
  return m_isModifiedOrRevealsAddr;
}

bool ConcreteObject::isStoredInMemory() const {
  assert(storageDuration() != StorageDuration::eYetUndefined);
  return storageDuration() != StorageDuration::eLocal ||
    m_isModifiedOrRevealsAddr;
}
