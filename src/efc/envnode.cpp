#include "envnode.h"
#include <sstream>

using namespace std;

EnvNode::EnvNode(string name) :
  m_name{move(name)},
  m_envParent{} {
}

const string& EnvNode::name() const {
  return m_name;
}

const string& EnvNode::fqName() const {
  if ( m_fqName.empty() ) {
    m_fqName = createFqName();
  }
  return m_fqName;
}

bool EnvNode::insert(EnvNode& node) {
  assert(!node.m_envParent); // ensure node is not already inserted into Env
  assert(node.m_envChildren.empty());

  const auto nameAlreadyExists = find(node.name());
  if ( nameAlreadyExists ) {
    return false;
  } else {
    node.m_envParent = this;
    m_envChildren.push_back(&node);
    return true;
  }
}

EnvNode* EnvNode::find(const string& name) {
  const auto foundNode = find_if(m_envChildren.begin(), m_envChildren.end(),
    [&](const auto& node)->bool{return node->name() == name;});
  return foundNode==m_envChildren.end() ? nullptr : *foundNode;
}

// special case:
//   fully qualified name of root is equal to roots unqualified name: "$root"
//
// normal cases: any descendant of root. Think that roots name is cut away.
//   fully qualified name of a roots child is ".foo"
//   fully qualified name of a roots child child is ".foo.bar"
string EnvNode::createFqName() const {
  const auto thisIsRoot = nullptr == this->m_envParent;
  if ( thisIsRoot ) {
    return m_name;
  }

  deque<const EnvNode*> nodes{};
  for (auto node = this; node && node->m_envParent; node = node->m_envParent) {
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
