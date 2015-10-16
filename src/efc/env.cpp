#include "env.h"
#include "symboltableentry.h"
using namespace std;

Env::AutoScope::AutoScope(Env& env) : m_env(env) {
  m_env.pushScope();
}

Env::AutoScope::~AutoScope() {
  m_env.popScope();
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
