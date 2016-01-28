#include "env.h"
#include "entity.h"
#include <algorithm>
#include <thread>
using namespace std;

namespace {
  thread_local shared_ptr<Entity>* dummyEntity;
}

Env::Node* Env::Node::insert(string name) {
  if ( find(name) ) {
    return nullptr;
  } else {
    m_children.emplace_back(move(name), this);
    return &m_children.back();
  }
}

Env::Node* Env::Node::find(const string& name) {
  const auto foundNode = std::find_if(m_children.begin(), m_children.end(),
    [&](const auto& x)->bool{return x.m_name == name;});
  if ( foundNode==m_children.end() ) {
    return nullptr;
  } else {
    return &(*foundNode);
  }
}

Env::Node::Node(string name, Node* parent) :
  m_name(move(name)),
  m_parent(parent) {
  assert(!m_name.empty());
}

Env::AutoScope::AutoScope(Env& env, const string& name,
  Env::AutoScope::Action action) :
  AutoScope(env, name, dummyEntity, action) {
  assert(dummyEntity);
}

Env::AutoScope::AutoScope(Env& env, const string& name,
  shared_ptr<Entity>*& entity, Env::AutoScope::Action action) :
  m_env(env) {
  entity = (action == insertScopeAndDescent) ?
    m_env.insertScopeAndDescent(name) :
    m_env.descentScope(name);
  m_didDescent = (entity!=nullptr);
}

Env::AutoScope::~AutoScope() {
  if ( m_didDescent ) {
    m_env.ascentScope();
  }
}

Env::Env() :
  m_rootScope{"$root", nullptr},
  m_currentScope{&m_rootScope} {
}

shared_ptr<Entity>* Env::insertLeaf(const string& name) {
  const auto newNode = m_currentScope->insert(name);
  return newNode ? &newNode->m_entity : nullptr;
}

shared_ptr<Entity>* Env::insertLeafAtGlobalScope(const string& name) {
  const auto newNode = m_rootScope.insert(name);
  return newNode ? &newNode->m_entity : nullptr;
}

shared_ptr<Entity>* Env::insertScopeAndDescent(const string& name) {
  const auto newNode = m_currentScope->insert(name);
  if ( newNode ) {
    m_currentScope = newNode;
    return &newNode->m_entity;
  } else {
    return nullptr;
  }
}

shared_ptr<Entity>* Env::descentScope(const string& name) {
  const auto foundChildNode = m_currentScope->find(name);
  if (foundChildNode) {
    m_currentScope = foundChildNode;
    return &foundChildNode->m_entity;
  } else {
    return nullptr;
  }
}

shared_ptr<Entity>* Env::find(const string& name) {
  for (auto currentScope = m_currentScope;
       currentScope;
       currentScope = currentScope->m_parent) {
    const auto foundChildNode = currentScope->find(name);
    if (foundChildNode) {
      return &foundChildNode->m_entity;
    }
  }
  return nullptr;
}

string Env::makeUniqueInternalName(string baseName) {
  thread_local auto thread_local_cnt = 0U;
  ++thread_local_cnt;
  stringstream ss{baseName};
  if ( baseName.empty() ) {
    ss << "$tmp";
  }
  ss << this_thread::get_id() << "_" << thread_local_cnt;
  return ss.str();
}

void Env::ascentScope() {
  assert(m_currentScope);
  assert(m_currentScope->m_parent);
  m_currentScope = m_currentScope->m_parent;
}

bool Env::isAtGlobalScope() {
  return m_currentScope == &m_rootScope;
}

void Env::printTo(ostream& os, const Env::Node& node) const {
  os << "{name=" << node.m_name;

  os << ", m_entity=";
  if ( node.m_entity ) {
    os << *node.m_entity;
  } else {
    os << "null";
  }

  if ( !node.m_children.empty() ) {
    os << ", children={";
    auto isFirstIter = true;
    for ( const auto& child : node.m_children  ) {
      if ( !isFirstIter ) {
        os << ", ";
      }
      isFirstIter = false;
      printTo(os, child);
    }
    os << "}";
  }

  os << "}";
}

void Env::printFullyQualifiedName(std::ostream& os, const Node& node) const {
  std::stack<const Node*> pathToRoot;
  for (auto currentNode = &node;
       currentNode;
       currentNode = currentNode->m_parent) {
    pathToRoot.push(currentNode);
  }
  auto isFirstIter = true;
  while (!pathToRoot.empty()) {
    if ( !isFirstIter ) {
      os << ".";
    }
    isFirstIter = false;
    os << pathToRoot.top()->m_name;
    pathToRoot.pop();
  }
}

ostream& operator<<(ostream& os, const Env& env) {
  os << "{currentScope=";
  env.printFullyQualifiedName(os, *env.m_currentScope);
  os << ", rootScope=";
  env.printTo(os, env.m_rootScope);
  os << "}";
  return os;
}
