#include "object.h"

Object::Object() : m_isModifiedOrRevealsAddr{false}, m_ir{*this} {
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
