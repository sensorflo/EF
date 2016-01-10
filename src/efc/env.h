#pragma once
#include "declutils.h"
#include "entity.h"
#include "tree.h"
#include <string>
#include <map>
#include <memory>
#include <ostream>

class SymbolTable final : public std::map<std::string, std::shared_ptr<Entity> > {
public:
  typedef value_type KeyValue;
};

std::ostream& operator<<(std::ostream&, const SymbolTable&);

class Env final {
public:
  class AutoScope final {
  public:
    AutoScope(Env& env);
    ~AutoScope();
  private:
    NEITHER_COPY_NOR_MOVEABLE(AutoScope);
    Env& m_env;
  };

  typedef std::pair<SymbolTable::iterator,bool> InsertRet;

  Env();

  InsertRet insertAtGlobalScope(const std::string& name,
    std::shared_ptr<Entity> entity = nullptr);
  InsertRet insert(const std::string& name, std::shared_ptr<Entity> entity = nullptr);
  InsertRet insert(const SymbolTable::KeyValue& keyValue);
  void find(const std::string& name, std::shared_ptr<Entity>& entity);
  void pushScope();
  void popScope();

private:
  friend std::ostream& operator<<(std::ostream&, const Env&);

  void printTo(std::ostream& os, tree<SymbolTable>::iterator node) const;
  NEITHER_COPY_NOR_MOVEABLE(Env);

  /** The environment is a tree of symbol tables. m_sts.begin() is the root
  of the tree, i.e. the top level symbol table. */
  tree<SymbolTable> m_sts;
  /** Denotes, in the form of a stack, the current node (back()) within m_sts,
  inclusive back trace up to the root (front()). Is guaranteed to always
  contain at least one element. */
  std::vector<tree<SymbolTable>::iterator> m_nestedScopes;
};

std::ostream& operator<<(std::ostream&, const Env&);
