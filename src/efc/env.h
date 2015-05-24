#pragma once
#include "objtype.h"
#include "storageduration.h"
#include "llvm/IR/IRBuilder.h"
#include <string>
#include <ostream>
#include <map>
#include <list>
#include <cassert>
#include <memory>

class ErrorHandler;
namespace llvm {
  class Value;
}

/** Currently it is two things; should eventually be splitted in these two. 
- Object: 
- Symbol table entry: for objects having a name and thus are in the symbol
  table. Contains a pointer to its object. */
class SymbolTableEntry {
public:
  SymbolTableEntry(std::shared_ptr<const ObjType> objType,
    StorageDuration storageDuration);

  const ObjType& objType() const { return *m_objType.get(); }
  StorageDuration storageDuration() const { return m_storageDuration; }
  bool isDefined() const { return m_isDefined; }
  void markAsDefined(ErrorHandler& errorHandler);
  void addAccessToObject(Access access);
  bool objectIsModifiedOrRevealsAddr() const;
  /** Semantically every non-abstract object is stored in memory. However
  certain objects can be optimized to live only as an SSA
  value. `isStoredInMemory' is both in the sense of `must be stored in memory'
  (equals `can't be optimized to be a SSA value only') and `actually is stored
  in memory'. */
  bool isStoredInMemory() const;
  std::string toStr() const;

private:
  /** Is guaranteed to be non-null */
  const std::shared_ptr<const ObjType> m_objType;
  const StorageDuration m_storageDuration;
  /** Opposed to only declared */
  bool m_isDefined;
  bool m_objectIsModifiedOrRevealsAddr;

  // decorations for IrGen
public:
  void irInitLocal(llvm::Value* irValue, llvm::IRBuilder<>& builder,
    const std::string& name = "");
  llvm::Value* irValue(llvm::IRBuilder<>& builder, const std::string& name = "") const;
  void setIrValue(llvm::Value* irValue, llvm::IRBuilder<>& builder);
  llvm::Value* irAddr() const;
  void setIrAddr(llvm::Value* addr);

private:
  /** is an irValue for isStoredInMemory() being false and an irAddr
  otherwise */
  llvm::Value* m_irValueOrAddr;
};

/** Only for the Object part of SymbolTableEntry */
std::basic_ostream<char>& operator<<(std::basic_ostream<char>& os,
  const SymbolTableEntry& stentry);

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
