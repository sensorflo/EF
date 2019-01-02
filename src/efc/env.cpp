#include "env.h"

#include <algorithm>
#include <cassert>
#include <deque>
#include <sstream>
#include <thread>
#include <utility>

using namespace std;

static bool dummyBool;

Env::AutoScope::AutoScope(Env& env, EnvNode& node, Action action)
  : AutoScope(env, node, action, dummyBool) {
}

Env::AutoScope::AutoScope(
  Env& env, EnvNode& node, Env::AutoScope::Action action, bool& success)
  : m_env(env) {
  if (action == insertScopeAndDescent) {
    success = m_env.insertScopeAndDescent(node);
  }
  else {
    m_env.descentScope(node);
    success = true;
  }
  m_didDescent = success;
}

Env::AutoScope::~AutoScope() {
  if (m_didDescent) { m_env.ascentScope(); }
}

Env::AutoLetLooseNodes::AutoLetLooseNodes(Env& env) : m_env(env) {
}

Env::AutoLetLooseNodes::~AutoLetLooseNodes() {
  m_env.letLooseNodes();
}

Env::Env() : m_rootScope{"$root"}, m_currentScope{&m_rootScope} {
}

bool Env::insertLeaf(EnvNode& node) {
  return m_currentScope->insert(node);
}

bool Env::insertScopeAndDescent(EnvNode& node) {
  const auto success = m_currentScope->insert(node);
  if (success) { m_currentScope = &node; }
  return success;
}

void Env::descentScope(EnvNode& node) {
  assert(node.m_envParent == m_currentScope);
  m_currentScope = &node;
}

EnvNode* Env::find(const string& name) {
  for (auto scope = m_currentScope; scope != nullptr;
       scope = scope->m_envParent) {
    const auto node = scope->find(name);
    if (node != nullptr) { return node; }
  }
  return nullptr;
}

string Env::makeUniqueInternalName(string baseName) {
  thread_local auto thread_local_cnt = 0U;
  ++thread_local_cnt;
  const auto isEmpty = baseName.empty();
  stringstream ss{std::move(baseName)};
  if (isEmpty) { ss << "$tmp"; }
  ss << this_thread::get_id() << "_" << thread_local_cnt;
  return ss.str();
}

void Env::ascentScope() {
  assert(m_currentScope);
  assert(m_currentScope->m_envParent);
  m_currentScope = m_currentScope->m_envParent;
}

void Env::printTo(ostream& os, const EnvNode& node) const {
  os << "{" << node.name();

  if (!node.m_envChildren.empty()) {
    os << ", children={";
    auto isFirstIter = true;
    for (const auto& child : node.m_envChildren) {
      if (!isFirstIter) { os << ", "; }
      isFirstIter = false;
      printTo(os, *child);
    }
    os << "}";
  }

  os << "}";
}

void Env::letLooseNodes() {
  m_rootScope.m_envChildren.clear();
  m_currentScope = &m_rootScope;
}

ostream& operator<<(ostream& os, const Env& env) {
  os << "{currentScope=";
  os << env.m_currentScope->fqName();
  os << ", rootScope=";
  env.printTo(os, env.m_rootScope);
  os << "}";
  return os;
}
