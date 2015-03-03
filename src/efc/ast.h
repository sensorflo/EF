#ifndef AST_H
#define AST_H
#include "objtype.h"
#include "access.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Function.h"
#include <string>
#include <list>
#include <iostream>
#include <map>

#include "astforwards.h"
class AstConstVisitor;
class AstVisitor;
class SymbolTableEntry;
class ErrorHandler;

class AstNode {
public:
  virtual ~AstNode() {};
  virtual void accept(AstVisitor& visitor) =0;
  virtual void accept(AstConstVisitor& visitor) const =0;
  virtual Access access() const { return eRead; }
  virtual void setAccess(Access access, ErrorHandler& errorHandler);
  std::string toStr() const;

  // decorations for IrGen
public:
  virtual llvm::Value* irValue() = 0;
  virtual void setIrValue(llvm::Value*) = 0;
};

class AstValue : public AstNode {
public:
  virtual ObjType& objType() const;
};

class AstCast : public AstValue {
public:
  AstCast(AstValue* child, ObjType* objType);
  AstCast(AstValue* child, ObjTypeFunda::EType objType);
  ~AstCast();
  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;
  AstValue& child() const { return *m_child; }
  ObjType& objType() const { return *m_objType; }

private:
  /** We're the owner. Is guaranteed to be non-null */
  AstValue* m_child;
  /** We're the owner. Is garanteed to be non-null. */
  ObjType* m_objType;

  // decorations for IrGen
public:
  virtual llvm::Value* irValue() { return m_irValue; }
  virtual void setIrValue(llvm::Value* value) { m_irValue = value; }
public:
  llvm::Value* m_irValue;
};

class AstFunDef : public AstValue {
public:
  AstFunDef(AstFunDecl* decl, AstValue* body);
  virtual ~AstFunDef();
  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;
  virtual AstFunDecl& decl() const { return *m_decl; }
  virtual AstValue& body() const { return *m_body; }
private:
  /** We're the owner. Is garanteed to be non-null */
  AstFunDecl* const m_decl;
  /** We're the owner. Is garanteed to be non-null */
  AstValue* const m_body;

  // decorations for IrGen
public:
  virtual llvm::Value* irValue() { return m_irFunction; }
  virtual void setIrValue(llvm::Value* value) { m_irFunction = dynamic_cast<llvm::Function*>(value); }
  virtual llvm::Function* irFunction() { return m_irFunction; }
  virtual void setIrFunction(llvm::Function* function) { m_irFunction = function; }
public:
  llvm::Function* m_irFunction;
};

class AstFunDecl : public AstValue {
public:
  AstFunDecl(const std::string& name, std::list<AstArgDecl*>* args = NULL,
    SymbolTableEntry* stentry = NULL);
  AstFunDecl(const std::string& name, AstArgDecl* arg1,
    AstArgDecl* arg2 = NULL, AstArgDecl* arg3 = NULL);
  ~AstFunDecl();
  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;
  virtual const std::string& name() const { return m_name; }
  virtual std::list<AstArgDecl*>const& args() const { return *m_args; }
  virtual ObjType& objType() const;
  virtual ObjType& objType(bool stealOwnership) const;
  static std::list<AstArgDecl*>* createArgs(AstArgDecl* arg1 = NULL,
    AstArgDecl* arg2 = NULL, AstArgDecl* arg3 = NULL);
  virtual SymbolTableEntry* stentry() const { return m_stentry; }
  virtual void setStentry(SymbolTableEntry* stentry);

private:
  void initObjType();
  
  const std::string m_name;
  /** We're the owner. Is garanteed to be non-null */
  std::list<AstArgDecl*>* const m_args;
  /** If m_ownerOfObjType is true, we're the owner. Is garanteed to be
  non-null. */
  ObjType* m_objType;
  /** See m_objType */
  mutable bool m_ownerOfObjType;
  /** We're _not_ the owner; null means this FunDecl was not yet put into
  the environment */
  SymbolTableEntry* m_stentry;

// decorations for IrGen
public:
  virtual llvm::Value* irValue() { return m_irFunction; }
  virtual void setIrValue(llvm::Value* value) { m_irFunction = dynamic_cast<llvm::Function*>(value); }
  virtual llvm::Function* irFunction() { return m_irFunction; }
  virtual void setIrFunction(llvm::Function* function) { m_irFunction = function; }
public:
  llvm::Function* m_irFunction;
};

class AstDataDecl : public AstValue {
public:
  AstDataDecl(const std::string& name, ObjType* objType,
    SymbolTableEntry* stentry = NULL);
  ~AstDataDecl();
  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;
  virtual const std::string& name() const { return m_name; }
  virtual ObjType& objType() const;
  virtual ObjType& objType(bool stealOwnership) const;
  virtual SymbolTableEntry* stentry() const { return m_stentry; }
  virtual void setStentry(SymbolTableEntry* stentry);
  virtual Access access() const { return m_access; }
  virtual void setAccess(Access access, ErrorHandler& ) { m_access = access; }
  
private:
  const std::string m_name;
  /** If m_ownerOfObjType is true, we're the owner. Is garanteed to be
  non-null. */
  ObjType* m_objType;
  /** See m_objType */
  mutable bool m_ownerOfObjType;
  /** We're _not_ the owner; null means this DataDecl was not yet put into
  the environment */
  SymbolTableEntry* m_stentry;
  Access m_access;

// decorations for IrGen
public:
  virtual llvm::Value* irValue() { return m_irValue; }
  virtual void setIrValue(llvm::Value* value) { m_irValue = value; }
public:
  llvm::Value* m_irValue;
};

class AstArgDecl : public AstDataDecl {
public:
  AstArgDecl(const std::string& name, ObjType* objType) :
    AstDataDecl(name, &objType->addQualifier(ObjType::eMutable)) {};
  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;
};

class AstDataDef : public AstValue {
public:
  AstDataDef(AstDataDecl* decl, AstValue* initValue);
  AstDataDef(AstDataDecl* decl, AstCtList* ctorArgs = NULL);
  ~AstDataDef();
  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;
  virtual AstDataDecl& decl() const { return *m_decl; }
  AstCtList& ctorArgs() const { return *m_ctorArgs; }
  virtual AstValue& initValue() const;
  virtual Access access() const { return m_access; }
  virtual void setAccess(Access access, ErrorHandler& ) { m_access = access; }
private:
  /** We're the owner. Is garanteed to be non-null */
  AstDataDecl* const m_decl;
  /** We're the owner. Is garanteed to be non-null. */
  AstCtList* const m_ctorArgs;
  /** We're the owner. Is _NOT_ guaranteed to  be non-null. */
  AstValue* m_implicitInitializer;
  Access m_access;

// decorations for IrGen
public:
  virtual llvm::Value* irValue() { return m_irValue; }
  virtual void setIrValue(llvm::Value* value) { m_irValue = value; }
public:
  llvm::Value* m_irValue;
};

/** Literal number */
class AstNumber : public AstValue {
public:
  AstNumber(int value, ObjType* objType = NULL);
  AstNumber(int value, ObjTypeFunda::EType eType,
    ObjTypeFunda::Qualifier qualifier = ObjTypeFunda::eNoQualifier);
  ~AstNumber();
  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;
  int value() const { return m_value; }
  virtual ObjType& objType() const { return *m_objType; }
private:
  const int m_value;
  /** We're the owner. Is garanteed to be non-null. */
  ObjType* m_objType;

// decorations for IrGen
public:
  virtual llvm::Value* irValue() { return m_irValue; }
  virtual void setIrValue(llvm::Value* value) { m_irValue = value; }
public:
  llvm::Value* m_irValue;
};

class AstSymbol : public AstValue {
public:
  AstSymbol(const std::string& name, Access access = eRead) :
    m_name(name), m_access(access) {};
  virtual ~AstSymbol() {};
  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;
  const std::string& name() const { return m_name; }
  virtual Access access() const { return m_access; }
  virtual void setAccess(Access access, ErrorHandler& ) { m_access = access; }
private:
  const std::string m_name;
  Access m_access;

// decorations for IrGen
public:
  virtual llvm::Value* irValue() { return m_irValue; }
  virtual void setIrValue(llvm::Value* value) { m_irValue = value; }
public:
  llvm::Value* m_irValue;
};

class AstFunCall : public AstValue {
public:
  AstFunCall(AstValue* address, AstCtList* args = NULL);
  virtual ~AstFunCall();
  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;
  virtual AstValue& address () const { return *m_address; }
  AstCtList& args () const { return *m_args; }
private:
  /** We're the owner. Is garanteed to be non-null */
  AstValue* const m_address;
  /** We're the owner. Is garanteed to be non-null */
  AstCtList* const m_args;

// decorations for IrGen
public:
  virtual llvm::Value* irValue() { return m_irValue; }
  virtual void setIrValue(llvm::Value* value) { m_irValue = value; }
public:
  llvm::Value* m_irValue;
};

class AstOperator : public AstValue {
public:
  enum EOperation {
    eAssign = '=',
    eAdd = '+',
    eSub = '-',
    eMul = '*',
    eDiv = '/',
    ePlus = '+',
    eNot = '!',
    eSeq = ';',
    eAnd = 128,
    eOr,
    eEqualTo
  };
  AstOperator(char op, AstCtList* args);
  AstOperator(const std::string& op, AstCtList* args);
  AstOperator(EOperation op, AstCtList* args);
  AstOperator(char op, AstValue* operand1 = NULL, AstValue* operand2 = NULL,
    AstValue* operand3 = NULL);
  AstOperator(const std::string& op, AstValue* operand1 = NULL, AstValue* operand2 = NULL,
    AstValue* operand3 = NULL);
  AstOperator(EOperation op, AstValue* operand1 = NULL, AstValue* operand2 = NULL,
    AstValue* operand3 = NULL);
  virtual ~AstOperator();
  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;
  EOperation op() const { return m_op; }
  AstCtList& args() const { return *m_args; }

private:
  static EOperation toEOperation(const std::string& op);
  AstOperator(const AstOperator&);
  AstOperator& operator=(const AstOperator&);
  const EOperation m_op;
  /** We're the owner. Is garanteed to be non-null */
  AstCtList* const m_args;
  static std::map<std::string, EOperation> m_opMap;

// decorations for IrGen
public:
  virtual llvm::Value* irValue() { return m_irValue; }
  virtual void setIrValue(llvm::Value* value) { m_irValue = value; }
public:
  llvm::Value* m_irValue;
};

std::basic_ostream<char>& operator<<(std::basic_ostream<char>& os,
  AstOperator::EOperation op);

/* If flow control expression */
class AstIf : public AstValue {
public:
  struct ConditionActionPair {
    ConditionActionPair(AstValue* condition, AstValue* action) :
      m_condition(condition), m_action(action) {}
    /** We're the owner. Is garanteed to be non-null */
    AstValue* m_condition;
    /** We're the owner. Is garanteed to be non-null */
    AstValue* m_action;
  };
  AstIf(std::list<ConditionActionPair>* conditionActionPairs, AstValue* elseAction = NULL);
  AstIf(AstValue* cond, AstValue* action, AstValue* elseAction = NULL);
  virtual ~AstIf();
  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;
  std::list<ConditionActionPair>& conditionActionPairs() const { return *m_conditionActionPairs; }
  AstValue* elseAction() const { return m_elseAction; }
private:
  static std::list<ConditionActionPair>* makeDefaultConditionActionPairs();
  static std::list<ConditionActionPair>* makeConditionActionPairs(
    AstValue* cond, AstValue* action);
  /** We're the owner. Is garanteed to be non-null and size>=1*/
  std::list<ConditionActionPair>* const m_conditionActionPairs;
  /** We're the owner. Is NOT garanteed to be non-null */
  AstValue* const m_elseAction;

// decorations for IrGen
public:
  virtual llvm::Value* irValue() { return m_irValue; }
  virtual void setIrValue(llvm::Value* value) { m_irValue = value; }
public:
  llvm::Value* m_irValue;
};

class AstCtList : public AstNode {
public:
  AstCtList(std::list<AstValue*>* childs);
  AstCtList(AstValue* child1 = NULL);
  AstCtList(AstValue* child1, AstValue* child2, AstValue* child3 = NULL);
  ~AstCtList();
  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;
  AstCtList* Add(AstValue* child);
  AstCtList* Add(AstValue* child1, AstValue* child2, AstValue* child3 = NULL);
  /** The elements are guaranteed to be non-null */
  std::list<AstValue*>& childs() const { return *m_childs; }
private:
  AstCtList(const AstCtList&);
  AstCtList& operator=(const AstCtList&);

  /** We're the owner of the list and of the pointees. Pointers are garanteed
  to be non null*/
  std::list<AstValue*>*const m_childs;

  // decorations for IrGen
public:
  virtual llvm::Value* irValue() { static llvm::Value* v = NULL; return v; }
  virtual void setIrValue(llvm::Value*) { }
};

#endif
