#pragma once
#include "declutils.h"
#include "envnode.h"
#include <string>
#include <ostream>

/** The environment is a tree. Each node has a name which is unqiue among its
siblings. Internal nodes, i.e. nodes with one or more children, are also
called Scope. Insertion operations are relative to the so called current
node. The current node can be reseated by the descent and ascent
operations.

The EnvNode s added to the Env shall live longer than the Env since the
environment tree relation ship pointers between the EnvNode s are raw
pointers. That is they become dangling pointers when (part of) the nodes are
destructed. Typically the nodes are created during parsing. The env is already
needed by the ParserExt. So use AutoLetLooseNodes after parsing to ensure that
Env does not access dangling pointers.

    Env env;
    unique_ptr<AstNode> astRoot =
      ... parse or otherwise create AST while env is available ...
    Env::AutoLetLooseNodes dummy(env);

Env's destructor does not access the nodes. Thus technically AutoLetLooseNodes
is not needed if you're sure the env object is not used after the AST has been
destroyed. */
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

    AutoScope(Env& env, EnvNode& node, Action action = descentScope);
    AutoScope(Env& env, EnvNode& node, Action action, bool& success);
    ~AutoScope();

  private:
    NEITHER_COPY_NOR_MOVEABLE(AutoScope);
    bool m_didDescent;
    Env& m_env;
  };

  /** When goes out of scope, calls letLooseNodes on passed env. See also
  class comment. */
  class AutoLetLooseNodes {
  public:
    AutoLetLooseNodes(Env& env);
    ~AutoLetLooseNodes();
  private:
    Env& m_env;
  };

  Env();

  /** Inserts a new child node with the given name to the current node and
  returns true for success. Returning false means an EnvNode with the same
  name already exists within the current node. */
  bool insertLeaf(EnvNode& node);

  /** Starting at the current node, searches the name in each node, on the
  path up to the root, returning the first occurence found. */
  EnvNode* find(const std::string& name);

  static std::string makeUniqueInternalName(std::string baseName = "");

private:
  NEITHER_COPY_NOR_MOVEABLE(Env);
  friend std::ostream& operator<<(std::ostream&, const Env&);

  bool insertScopeAndDescent(EnvNode& node);
  void descentScope(EnvNode& node);
  void ascentScope();
  void printTo(std::ostream& os, const EnvNode& node) const;
  /** The root node (which is owned by Env), lets loose all its children, and
  the current node is reseated to point to the root. See also class
  comment. */
  void letLooseNodes();

  EnvNode m_rootScope;
  /** Guaranteed to be non-null. We're not the owner. */
  EnvNode* m_currentScope;
};

std::ostream& operator<<(std::ostream&, const Env&);
