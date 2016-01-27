#include "object.h"
#include "errorhandler.h"
#include "llvm/IR/Value.h"
#include <cassert>
#include <sstream>
using namespace std;
using namespace llvm;

Object::Object(StorageDuration storageDuration) :
  m_objType(nullptr),
  m_storageDuration(storageDuration),
  m_isModifiedOrRevealsAddr(false),
  m_isInitialized(false),
  m_irValueOrAddr(nullptr) {}

Object::Object(shared_ptr<const ObjType> objType,
  StorageDuration storageDuration) :
  m_objType( (assert(objType.get()), move(objType))),
  m_storageDuration(storageDuration),
  m_isModifiedOrRevealsAddr(false),
  m_isInitialized(false),
  m_irValueOrAddr(NULL) {}

void Object::setObjType(std::shared_ptr<const ObjType> objType) {
  assert(!m_objType); // doesn't make sense to set it twice
  m_objType = move(objType);
}

void Object::addAccess(Access access) {
  m_isModifiedOrRevealsAddr =
    m_isModifiedOrRevealsAddr ||
    access==eWrite || access==eTakeAddress;
}

bool Object::isModifiedOrRevealsAddr() const {
  return m_isModifiedOrRevealsAddr;
}

bool Object::isStoredInMemory() const {
  return m_storageDuration!=StorageDuration::eLocal ||
    m_isModifiedOrRevealsAddr;
}

void Object::irInitLocal(llvm::Value* irValue, llvm::IRBuilder<>& builder,
  const std::string& name) {
  assert( !m_irValueOrAddr );  // It doesn't make sense to set it twice
  if ( isStoredInMemory() ) {
    Function* functionIr = builder.GetInsertBlock()->getParent();
    IRBuilder<> irBuilder(&functionIr->getEntryBlock(),
      functionIr->getEntryBlock().begin());
    m_irValueOrAddr = irBuilder.CreateAlloca(objType().llvmType(), 0, name);
    builder.CreateStore(irValue, m_irValueOrAddr);
  } else {
    m_irValueOrAddr = irValue;
  }
}

llvm::Value* Object::irValue(llvm::IRBuilder<>& builder, const string& name) const {
  assert( m_irValueOrAddr );
  if ( isStoredInMemory() ) {
    return builder.CreateLoad(m_irValueOrAddr, name);
  } else {
    return m_irValueOrAddr;
  }
}

void Object::setIrValue(llvm::Value* irValue, llvm::IRBuilder<>& builder) {
  if ( isStoredInMemory() ) {
    builder.CreateStore(irValue, m_irValueOrAddr);
  } else {
    assert(irValue);
    assert(!m_irValueOrAddr); // It doesn't make sense to set it twice
    m_irValueOrAddr = irValue;
  }
}

llvm::Value* Object::irAddr() const {
  assert( isStoredInMemory() );
  return m_irValueOrAddr;
}

void Object::setIrAddr(llvm::Value* addr) {
  assert( isStoredInMemory() );
  assert( !m_irValueOrAddr ); // It doesn't make sense to set it twice
  m_irValueOrAddr = addr;
}

basic_ostream<char>& Object::printTo(basic_ostream<char>& os) const {
  if ( m_storageDuration!=StorageDuration::eLocal ) {
    os << m_storageDuration << "/";
  }
  if ( m_objType ) {
    os << *m_objType;
  } else {
    os << "<undefined>";
  }
  return os;
}
