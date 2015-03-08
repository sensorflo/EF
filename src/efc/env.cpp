#include "env.h"
#include "errorhandler.h"
#include "llvm/IR/Value.h"
#include <cassert>
using namespace std;
using namespace llvm;

SymbolTableEntry::~SymbolTableEntry() {
  delete &m_objType;
}

void SymbolTableEntry::markAsDefined(ErrorHandler& errorHandler) {
  if (m_isDefined) {
    errorHandler.add(new Error(Error::eRedefinition));
    throw BuildError();
  }
  m_isDefined = true;
}

SymbolTable::~SymbolTable() {
  map<string,SymbolTableEntry*>::iterator i = begin();
  for ( ; i!=end(); ++i ) {
    SymbolTableEntry* stentry = i->second;
    if (stentry) { delete stentry; }
  }
}

llvm::Value* SymbolTableEntry::valueIr() const {
  return m_valueIr;
}

void SymbolTableEntry::setValueIr(llvm::Value* valueIr) {
  assert(valueIr);
  assert(!m_valueIr); // It doesn't make sense to set it twice
  m_valueIr = valueIr;
}

Env::Env() {
  m_ststack.push_front(SymbolTable());
}

/** Upon _successfull_ insertion, the symbol table takes ownership of stentry. A null
stentry is not allowed. */
Env::InsertRet Env::insert(const string& name, SymbolTableEntry* stentry) {
  return insert(make_pair(name, stentry));
}

/** \overload */
Env::InsertRet Env::insert(const SymbolTable::KeyValue& keyValue) {
  return m_ststack.front().insert(keyValue);
}

SymbolTableEntry* Env::find(const string& name) {
  list<SymbolTable>::iterator iter = m_ststack.begin();
  for ( /*nop*/; iter!=m_ststack.end(); ++iter ) {
    SymbolTable& symbolTable = *iter;
    SymbolTable::iterator i = symbolTable.find(name);
    if (i!=symbolTable.end()) { return i->second; }
  }
  return NULL;
}

void Env::pushScope() {
  m_ststack.push_front(SymbolTable());
}

void Env::popScope() {
  assert(!m_ststack.empty());
  m_ststack.pop_front();
}
