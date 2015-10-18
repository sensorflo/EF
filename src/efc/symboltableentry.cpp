#include "symboltableentry.h"
#include "errorhandler.h"
#include "llvm/IR/Value.h"
#include <cassert>
#include <sstream>
using namespace std;
using namespace llvm;

SymbolTableEntry::SymbolTableEntry(shared_ptr<const ObjType> objType,
  StorageDuration storageDuration) :
  m_objType( (assert(objType.get()), move(objType))),
  m_storageDuration(storageDuration),
  m_objectIsModifiedOrRevealsAddr(false),
  m_irValueOrAddr(NULL) {}

void SymbolTableEntry::addAccessToObject(Access access) {
  m_objectIsModifiedOrRevealsAddr =
    m_objectIsModifiedOrRevealsAddr ||
    access==eWrite || access==eTakeAddress;
}

bool SymbolTableEntry::objectIsModifiedOrRevealsAddr() const {
  return m_objectIsModifiedOrRevealsAddr;
}

bool SymbolTableEntry::isStoredInMemory() const {
  return m_storageDuration!=StorageDuration::eLocal ||
    m_objectIsModifiedOrRevealsAddr;
}

string SymbolTableEntry::toStr() const {
  ostringstream ss;
  ss << *this;
  return ss.str();
}

void SymbolTableEntry::irInitLocal(llvm::Value* irValue, llvm::IRBuilder<>& builder,
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

llvm::Value* SymbolTableEntry::irValue(llvm::IRBuilder<>& builder, const string& name) const {
  assert( m_irValueOrAddr );
  if ( isStoredInMemory() ) {
    return builder.CreateLoad(m_irValueOrAddr, name);
  } else {
    return m_irValueOrAddr;
  }
}

void SymbolTableEntry::setIrValue(llvm::Value* irValue, llvm::IRBuilder<>& builder) {
  if ( isStoredInMemory() ) {
    builder.CreateStore(irValue, m_irValueOrAddr);
  } else {
    assert(irValue);
    assert(!m_irValueOrAddr); // It doesn't make sense to set it twice
    m_irValueOrAddr = irValue;
  }
}

llvm::Value* SymbolTableEntry::irAddr() const {
  assert( isStoredInMemory() );
  return m_irValueOrAddr;
}

void SymbolTableEntry::setIrAddr(llvm::Value* addr) {
  assert( isStoredInMemory() );
  assert( !m_irValueOrAddr ); // It doesn't make sense to set it twice
  m_irValueOrAddr = addr;
}

basic_ostream<char>& operator<<(basic_ostream<char>& os,
  const SymbolTableEntry& stentry) {
  if ( stentry.storageDuration()!=StorageDuration::eLocal ) {
    os << stentry.storageDuration() << "/";
  }
  os << stentry.objType();
  return os;
}
