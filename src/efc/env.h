#pragma once
#include "declutils.h"
#include "entity.h"
#include <string>
#include <map>
#include <list>
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

  NEITHER_COPY_NOR_MOVEABLE(Env);

  /** symbol tables, organized as stack, where as front() is top of
  stack. Each nested scope corresponds to an entry in the stack. Function body
  is a top level scope, i.e. when in function body, m_sts.size() is 1, when
  outside a function body m_sts.size() is 0.  */
  std::list<SymbolTable> m_sts;
};

std::ostream& operator<<(std::ostream&, const Env&);
