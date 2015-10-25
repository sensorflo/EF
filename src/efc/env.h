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
  ~Env() = default;

  InsertRet insert(const std::string& name, std::shared_ptr<Entity> entity);
  InsertRet insert(const SymbolTable::KeyValue& keyValue);
  void find(const std::string& name, std::shared_ptr<Entity>& entity);
  void pushScope();
  void popScope();

private:
  friend std::ostream& operator<<(std::ostream&, const Env&);

  NEITHER_COPY_NOR_MOVEABLE(Env);

  /** symbol table stack. front() is top of stack */
  std::list<SymbolTable> m_ststack;
};

std::ostream& operator<<(std::ostream&, const Env&);
