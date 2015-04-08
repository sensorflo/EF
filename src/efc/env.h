#pragma once
#include "entity.h"
#include <string>
#include <map>
#include <list>
#include <memory>

class EntityTable : public std::map<std::string, std::shared_ptr<Entity> > {
public:
  typedef value_type KeyValue;
};

class Env {
public:
  class AutoScope {
  public:
    AutoScope(Env& env);
    ~AutoScope();
  private:
    Env& m_env;
  };

  typedef std::pair<EntityTable::iterator,bool> InsertRet;

  Env();
  InsertRet insert(const std::string& name, std::shared_ptr<Entity> stentry);
  InsertRet insert(const EntityTable::KeyValue& keyValue);
  void find(const std::string& name, std::shared_ptr<Entity>& stentry);
  void pushScope();
  void popScope();

private:
  /** symbol table stack. front() is top of stack */
  std::list<EntityTable> m_ststack;
};

