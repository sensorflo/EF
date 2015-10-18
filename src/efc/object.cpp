#include "object.h"
#include "errorhandler.h"
#include "llvm/IR/Value.h"
#include <cassert>
#include <sstream>
using namespace std;
using namespace llvm;

Object::Object(shared_ptr<const ObjType> objType,
  StorageDuration storageDuration) :
  m_objType( (assert(objType.get()), move(objType))),
  m_storageDuration(storageDuration),
  m_isModifiedOrRevealsAddr(false),
  m_irValueOrAddr(NULL) {}

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

string Object::toStr() const {
  ostringstream ss;
  ss << *this;
  return ss.str();
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

basic_ostream<char>& operator<<(basic_ostream<char>& os,
  const Object& obj) {
  if ( obj.storageDuration()!=StorageDuration::eLocal ) {
    os << obj.storageDuration() << "/";
  }
  os << obj.objType();
  return os;
}
