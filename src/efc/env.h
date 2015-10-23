#pragma once
#include "entity.h"
#include <string>
#include <map>
#include <list>
#include <memory>
#include <ostream>

class SymbolTable : public std::map<std::string, std::shared_ptr<Entity> > {
public:
  typedef value_type KeyValue;
};

std::ostream& operator<<(std::ostream&, const SymbolTable&);

class Env {
public:
  class AutoScope {
  public:
    AutoScope(Env& env);
    ~AutoScope();
  private:
    Env& m_env;
  };

  typedef std::pair<SymbolTable::iterator,bool> InsertRet;

  Env();
  InsertRet insert(const std::string& name, std::shared_ptr<Entity> entity);
  InsertRet insert(const SymbolTable::KeyValue& keyValue);
  void find(const std::string& name, std::shared_ptr<Entity>& entity);
  void pushScope();
  void popScope();

private:
  friend std::ostream& operator<<(std::ostream&, const Env&);
  /** symbol table stack. front() is top of stack */
  std::list<SymbolTable> m_ststack;
};

std::ostream& operator<<(std::ostream&, const Env&);