#ifndef ENV_H
#define ENV_H
#include "objtype.h"
#include <string>
#include <map>
#include <list>
#include <cassert>

class ErrorHandler;
namespace llvm {
  class Value;
}

class SymbolTableEntry {
public:
  SymbolTableEntry(ObjType* objType) :
    m_objType( (assert(objType), *objType) ),
    m_isDefined(false),
    m_valueIr(NULL) {}
  ~SymbolTableEntry();

  ObjType& objType() { return m_objType; }
  bool isDefined() { return m_isDefined; }
  void markAsDefined(ErrorHandler& errorHandler);

private:
  /** We're the owner. */
  ObjType& m_objType;
  /** Opposed to only declared */
  bool m_isDefined;

  // decorations for IrGen
public:
  llvm::Value* valueIr() const;
  void setValueIr(llvm::Value* valueIr);
private:
  llvm::Value* m_valueIr;
};

class SymbolTable : public std::map<std::string,SymbolTableEntry*> {
public:
  typedef value_type KeyValue;
  virtual ~SymbolTable();
  // we're the owner of the SymbolTableEntry objects pointet to
};

class Env {
public:
  typedef std::pair<SymbolTable::iterator,bool> InsertRet;

  Env();
  InsertRet insert(const std::string& name, SymbolTableEntry* stentry); 
  InsertRet insert(const SymbolTable::KeyValue& keyValue); 
  SymbolTableEntry* find(const std::string& name); 
  void pushScope();
  void popScope();

private:
  /** symbol table stack. front() is top of stack */
  std::list<SymbolTable> m_ststack;
};

#endif
