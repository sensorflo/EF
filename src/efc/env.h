#ifndef ENV_H
#define ENV_H
#include "objtype.h"
#include <string>
#include <map>
#include <list>
namespace llvm {
  class Value;
}

struct SymbolTableEntry {
  SymbolTableEntry() : m_valueIr(NULL), m_objType(ObjType::eConst),
                       m_isDefined(false) {}
  SymbolTableEntry(llvm::Value* valueIr, ObjType objType, bool isDefined = false) :
    m_valueIr(valueIr), m_objType(objType), m_isDefined(isDefined) {}

  llvm::Value* m_valueIr;
  ObjType m_objType;
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
