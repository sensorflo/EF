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
  // create the toplevel (aka root) symboltable
  m_nestedScopes.push_back(m_sts.insert(m_sts.begin(), SymbolTable()));
}

Env::InsertRet Env::insertAtGlobalScope(const string& name, shared_ptr<Entity> entity) {
  return m_nestedScopes.front()->insert(make_pair(name, move(entity)));
}

Env::InsertRet Env::insert(const string& name, shared_ptr<Entity> entity) {
  return insert(make_pair(name, move(entity)));
}

/** \overload */
Env::InsertRet Env::insert(const SymbolTable::KeyValue& keyValue) {
  return m_nestedScopes.back()->insert(keyValue);
}

void Env::find(const string& name, shared_ptr<Entity>& entity) {
  for (auto iter = m_nestedScopes.rbegin(); iter!=m_nestedScopes.rend(); ++iter) {
    SymbolTable& symbolTable = **iter;
    SymbolTable::iterator i = symbolTable.find(name);
    if (i!=symbolTable.end()) {
      entity = i->second;
      return;
    }
  }
}

void Env::pushScope() {
  m_nestedScopes.push_back(m_sts.append_child(m_nestedScopes.back(), SymbolTable()));
}

void Env::popScope() {
  assert(m_nestedScopes.size()>=1); // we can't pop the top level symbol table
  m_nestedScopes.pop_back();
}

void Env::printTo(std::ostream& os, tree<SymbolTable>::iterator node) const {
  os << "{ST=" << *node;
  os << ", childs=";

  bool isFirstIter = true;
  tree<SymbolTable>::sibling_iterator sib=m_sts.begin(node);
  os << "{";
  for ( ; sib!=m_sts.end(node); ++sib) {
    if ( !isFirstIter ) {
      os << ", ";
    }
    isFirstIter = false;
    printTo(os, sib);
  }
  os << "}}";
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
  env.printTo(os, env.m_sts.begin());
  return os;
}
