#include "env.h"
#include "errorhandler.h"
#include "llvm/IR/Value.h"
#include <cassert>
using namespace std;
using namespace llvm;

SymbolTableEntry::SymbolTableEntry(shared_ptr<const ObjType> objType) :
  m_objType( (assert(objType.get()), move(objType))),
  m_isDefined(false),
  m_valueIr(NULL) {}

void SymbolTableEntry::markAsDefined(ErrorHandler& errorHandler) {
  if (m_isDefined) {
    errorHandler.add(new Error(Error::eRedefinition));
    throw BuildError();
  }
  m_isDefined = true;
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

Env::InsertRet Env::insert(const string& name, shared_ptr<SymbolTableEntry> stentry) {
  return insert(make_pair(move(name), move(stentry)));
}

/** \overload */
Env::InsertRet Env::insert(const SymbolTable::KeyValue& keyValue) {
  return m_ststack.front().insert(move(keyValue));
}

void Env::find(const string& name, shared_ptr<SymbolTableEntry>& stentry) {
  list<SymbolTable>::iterator iter = m_ststack.begin();
  for ( /*nop*/; iter!=m_ststack.end(); ++iter ) {
    SymbolTable& symbolTable = *iter;
    SymbolTable::iterator i = symbolTable.find(name);
    if (i!=symbolTable.end()) {
      stentry = i->second;
      return;
    }
  }
}

void Env::pushScope() {
  m_ststack.push_front(SymbolTable());
}

void Env::popScope() {
  assert(!m_ststack.empty());
  m_ststack.pop_front();
}
