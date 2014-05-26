#ifndef ENV_H
#define ENV_H
#include "objtype.h"
#include <string>
#include <map>
#include <list>
namespace llvm {
  class Value;
}

class SymbolTableEntry {
public:
  SymbolTableEntry() :
    m_valueIr(NULL), m_objType(new ObjTypeFunda), m_isDefined(false) {}
  SymbolTableEntry(llvm::Value* valueIr, ObjType* objType, bool isDefined = false) :
    m_valueIr(valueIr),
    m_objType(objType ? objType : new ObjTypeFunda),
    m_isDefined(isDefined) {}
  ~SymbolTableEntry();

  llvm::Value*& valueIr() { return m_valueIr; }
  ObjType& objType() { return *m_objType; }
  bool& isDefined() { return m_isDefined; }

private:
  llvm::Value* m_valueIr;
  /** We're the owner. Is garanteed to be non-null */
  ObjType* m_objType;
  /** Opposed to only declared */
  bool m_isDefined;
};

class SymbolTable : public std::map<std::string,SymbolTableEntry*> {
public:
  typedef value_type KeyValue;
  virtual ~SymbolTable();
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
