#ifndef ENV_H
#define ENV_H
#include "objtype.h"
#include <string>
#include <map>
#include <list>
#include <cassert>
#include <memory>

class ErrorHandler;
namespace llvm {
  class Value;
}

class SymbolTableEntry {
public:
  SymbolTableEntry(std::shared_ptr<const ObjType> objType);

  const ObjType& objType() const { return *m_objType.get(); }
  bool isDefined() const { return m_isDefined; }
  void markAsDefined(ErrorHandler& errorHandler);
  void addAccessToObject(Access access);
  bool objectWasModifiedOrRevealedAddr() const;

private:
  /** Is guaranteed to be non-null */
  const std::shared_ptr<const ObjType> m_objType;
  /** Opposed to only declared */
  bool m_isDefined;
  bool m_objectWasModifiedOrRevealedAddr;

  // decorations for IrGen
public:
  llvm::Value* valueIr() const;
  void setValueIr(llvm::Value* valueIr);
private:
  llvm::Value* m_valueIr;
};

class SymbolTable : public std::map<std::string, std::shared_ptr<SymbolTableEntry> > {
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

  typedef std::pair<SymbolTable::iterator,bool> InsertRet;

  Env();
  InsertRet insert(const std::string& name, std::shared_ptr<SymbolTableEntry> stentry);
  InsertRet insert(const SymbolTable::KeyValue& keyValue);
  void find(const std::string& name, std::shared_ptr<SymbolTableEntry>& stentry);
  void pushScope();
  void popScope();

private:
  /** symbol table stack. front() is top of stack */
  std::list<SymbolTable> m_ststack;
};

#endif
