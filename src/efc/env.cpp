#include "env.h"
#include "entity.h"
using namespace std;

Env::AutoScope::AutoScope(Env& env) : m_env(env) {
  m_env.pushScope();
}

Env::AutoScope::~AutoScope() {
  m_env.popScope();
}

Env::Env() {
  m_sts.push_front(SymbolTable());
}

Env::InsertRet Env::insertAtGlobalScope(const string& name, shared_ptr<Entity> entity) {
  return m_sts.back().insert(make_pair(name, move(entity)));
}

Env::InsertRet Env::insert(const string& name, shared_ptr<Entity> entity) {
  return insert(make_pair(name, move(entity)));
}

/** \overload */
Env::InsertRet Env::insert(const SymbolTable::KeyValue& keyValue) {
  return m_sts.front().insert(keyValue);
}

void Env::find(const string& name, shared_ptr<Entity>& entity) {
  list<SymbolTable>::iterator iter = m_sts.begin();
  for ( /*nop*/; iter!=m_sts.end(); ++iter ) {
    SymbolTable& symbolTable = *iter;
    SymbolTable::iterator i = symbolTable.find(name);
    if (i!=symbolTable.end()) {
      entity = i->second;
      return;
    }
  }
}

void Env::pushScope() {
  m_sts.push_front(SymbolTable());
}

void Env::popScope() {
  assert(!m_sts.empty());
  m_sts.pop_front();
}

std::ostream& operator<<(std::ostream& os, const SymbolTable& st) {
  os << "{";
  bool isFirstIter = true;
  for (const auto& stentry: st) {
    if ( !isFirstIter ) {
      os << ", ";
    }
    isFirstIter = false;
    os << stentry.first << "=";
    if ( stentry.second ) {
      stentry.second->printTo(os);
    } else {
      os << "null";
    }
  }
  return os << "}";
}

std::ostream& operator<<(std::ostream& os, const Env& env) {
  os << "{";
  bool isFirstIter = true;
  for (auto const& st: env.m_sts) {
    if ( !isFirstIter ) {
      os << ", ";
    }
    isFirstIter = false;
    os << st;
  }
  return os << "}";
}
