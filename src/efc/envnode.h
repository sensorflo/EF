#pragma once
#include "declutils.h"

#include <deque>
#include <ostream>
#include <string>

/** Special name denoting an anonymous node, that is a node without a name. */
extern const std::string s_anonymousName;

class EnvNode {
public:
  explicit EnvNode(std::string name);
  virtual ~EnvNode() = default;

  // -- misc
  const std::string& name() const;
  const std::string& fqName() const;

  /** End user readable description. Usually for the case he uses a name to
  refer to something, but what was found under that name does not fulfill
  semantic requirements. Meant to be overriden by AstNodes being definitions.*/
  virtual std::string description() const;

  bool insert(EnvNode& node);
  /** Searches only within childs of this node */
  EnvNode* find(const std::string& name);

private:
  NEITHER_COPY_NOR_MOVEABLE(EnvNode);

  friend class Env;

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
