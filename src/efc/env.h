#pragma once
#include "declutils.h"
#include "envnode.h"
#include "fqnameprovider.h"
#include <string>
#include <deque>
#include <ostream>

/** The environment is a tree. Each node has a name which is unqiue among its
siblings. Internal nodes, i.e. nodes with one or more children, are also
called Scope. Insertion operations are relative to the so called current
node. The current node can be reseated by the descent and ascent
operations. */
class Env final {
public:
  /** Ensures that ascentScope is always called when an insertAndDescent or
  descent succeeded. */
  class AutoScope final {
  public:
    enum Action {
      insertScopeAndDescent,
      descentScope
    };

    AutoScope(Env& env, const std::string& name, Action action);
    AutoScope(Env& env, const std::string& name,
      const FQNameProvider*& fqNameProvider, EnvNode**& node,
      Action action);
    ~AutoScope();

  private:
    NEITHER_COPY_NOR_MOVEABLE(AutoScope);
    bool m_didDescent;
    Env& m_env;
  };

  Env();

  /** Inserts a new child node with the given name to the current node and
  returns a raw pointer to the node member of the new node. Returns nullptr
  if the current node already has a child with that name. */
  EnvNode** insertLeaf(const std::string& name,
    const FQNameProvider*& fqNameProvider);

  /** Starting at the current node, searches the name in each node, on the
  path up to the root, returning the first occurence found. */
  EnvNode** find(const std::string& name);

  static std::string makeUniqueInternalName(std::string baseName = "");

private:
  class Node final : public FQNameProvider {
  public:
    Node(std::string name, Node* parent);
    /** Returns the just inserted node. Returns null if the given name already
    exists. */
    Node* insert(std::string name, const FQNameProvider*& fqNameProvider);
    Node* find(const std::string& name);
    const std::string& fqName() const override;
    std::string createFqName() const;

    // guaranteed to be non-empty
    const std::string m_name;
    mutable std::string m_fqName;
    // might be null, e.g. for AstBlocks
    EnvNode* m_node;
    // might be empty, e.g. for data objects. Note that it must be a container
    // that does never invalidate pointers when adding elements.
    std::deque<Node> m_children;
    // the root node has nullptr as parent, all other nodes are non-nullptr
    Node*const m_parent;
  };

  friend std::ostream& operator<<(std::ostream&, const Env&);

  /** As insert, but additionally makes the new node the current node. */
  EnvNode** insertScopeAndDescent(const std::string& name,
    const FQNameProvider*& fqNameProvider);
  EnvNode** descentScope(const std::string& name);
  void ascentScope();
  void printTo(std::ostream& os, const Node& node) const;
  NEITHER_COPY_NOR_MOVEABLE(Env);

  Node m_rootScope;
  /** Guaranteed to be non-null. We're not the owner. */
  Node* m_currentScope;
};

std::ostream& operator<<(std::ostream&, const Env&);
