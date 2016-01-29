#include "env.h"
#include "entity.h"
#include <algorithm>
#include <thread>
#include <deque>
using namespace std;

namespace {
  thread_local shared_ptr<Entity>* dummyEntity;
}

Env::Node* Env::Node::insert(string name, const FQNameProvider*& fqNameProvider) {
  if ( find(name) ) {
    return nullptr;
  } else {
    m_children.emplace_back(move(name), this);
    fqNameProvider = &m_children.back();
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

const string& Env::Node::fqName() const {
  if ( m_fqName.empty() ) {
    m_fqName = createFqName();
  }
  return m_fqName;
}

// special case:
//   fully qualified name of root is equal to roots unqualified name: "$root"
//
// normal cases: any descendant of root. Think that roots name is cut away.
//   fully qualified name of a roots child is ".foo"
//   fully qualified name of a roots child child is ".foo.bar"
string Env::Node::createFqName() const {
  const auto thisIsRoot = nullptr == this->m_parent;
  if ( thisIsRoot ) {
    return m_name;
  }

  deque<const Node*> nodes{};
  for (auto node = this; node && node->m_parent; node = node->m_parent) {
    nodes.push_front(node);
  }
  assert(!nodes.empty()); // would mean this is root, but that was handled above

  string fqName{};
  for (const auto& node: nodes) {
    fqName += ".";
    fqName += node->m_name;
  }
  return fqName;
}

Env::Node::Node(string name, Node* parent) :
  m_name(move(name)),
  m_parent(parent) {
  assert(!m_name.empty());
}

thread_local const FQNameProvider* dummyFqNameProvider{};
Env::AutoScope::AutoScope(Env& env, const string& name,
  Env::AutoScope::Action action) :
  AutoScope(env, name, dummyFqNameProvider, dummyEntity, action) {
  assert(dummyEntity);
}

Env::AutoScope::AutoScope(Env& env, const string& name,
  const FQNameProvider*& fqNameProvider, shared_ptr<Entity>*& entity,
  Env::AutoScope::Action action) :
  m_env(env) {
  entity = (action == insertScopeAndDescent) ?
    m_env.insertScopeAndDescent(name, fqNameProvider) :
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

shared_ptr<Entity>* Env::insertLeaf(const string& name,
  const FQNameProvider*& fqNameProvider) {
  const auto newNode = m_currentScope->insert(name, fqNameProvider);
  return newNode ?
    (fqNameProvider = newNode, &newNode->m_entity) :
    nullptr;
}

shared_ptr<Entity>* Env::insertLeafAtGlobalScope(const string& name) {
  const FQNameProvider* dummy;
  const auto newNode = m_rootScope.insert(name, dummy);
  return newNode ? &newNode->m_entity : nullptr;
}

shared_ptr<Entity>* Env::insertScopeAndDescent(const string& name,
  const FQNameProvider*& fqNameProvider) {
  const auto newNode = m_currentScope->insert(name, fqNameProvider);
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

ostream& operator<<(ostream& os, const Env& env) {
  os << "{currentScope=";
  os << env.m_currentScope->fqName();
  os << ", rootScope=";
  env.printTo(os, env.m_rootScope);
  os << "}";
  return os;
}
