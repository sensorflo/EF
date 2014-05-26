#include "env.h"
#include "llvm/IR/Value.h"
#include <cassert>
using namespace std;
using namespace llvm;

SymbolTableEntry::~SymbolTableEntry() {
  delete m_objType;
}

SymbolTable::~SymbolTable() {
  map<string,SymbolTableEntry*>::iterator i = begin();
  for ( ; i!=end(); ++i ) {
    SymbolTableEntry* stentry = i->second;
    if (stentry) { delete stentry; }
  }
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
  assert(keyValue.second);
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
