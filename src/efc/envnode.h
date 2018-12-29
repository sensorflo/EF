#pragma once
#include "declutils.h"

#include <deque>
#include <ostream>
#include <string>

class EnvNode {
public:
  explicit EnvNode(std::string name);
  virtual ~EnvNode() = default;

  // -- misc
  const std::string& name() const;
  const std::string& fqName() const;

private:
  NEITHER_COPY_NOR_MOVEABLE(EnvNode);

  friend class Env;

  bool insert(EnvNode& node);
  EnvNode* find(const std::string& name);
  std::string createFqName() const;

  const std::string m_name;
  mutable std::string m_fqName;
  /** Might be empty, say for data objects. We're not the owner of the
  pointees, see also Env's class comment */
  std::deque<EnvNode*> m_envChildren;
  /** The root node has nullptr as parent, all other nodes are
  non-nullptr. We're not the owner, see also Env's class comment. */
  EnvNode* m_envParent;
};
