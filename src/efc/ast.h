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
#include <memory>

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
};

class AstValue : public AstNode {
public:
  virtual const ObjType& objType() const =0;
};

class AstNop : public AstValue {
public:
  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;
  virtual const ObjType& objType() const;

  // decorations for IrGen
public:
  virtual llvm::Value* irValue() { return NULL; }
};

class AstCast : public AstValue {
public:
  AstCast(ObjType* objType, AstValue* child);
  AstCast(ObjTypeFunda::EType objType, AstValue* child);
  ~AstCast();
  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;
  AstValue& child() const { return *m_child; }
  virtual const ObjType& objType() const { return *m_objType; }

private:
  /** We're the owner. Is garanteed to be non-null. */
  const ObjType*const m_objType;
  /** We're the owner. Is guaranteed to be non-null */
  AstValue* m_child;

  // decorations for IrGen
public:
  virtual llvm::Value* irValue() { return m_irValue; }
  virtual void setIrValue(llvm::Value* value);
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
  virtual const ObjType& objType() const;

private:
  /** We're the owner. Is garanteed to be non-null */
  AstFunDecl* const m_decl;
  /** We're the owner. Is garanteed to be non-null */
  AstValue* const m_body;

  // decorations for IrGen
public:
  virtual llvm::Value* irValue() { return m_irFunction; }
  virtual void setIrValue(llvm::Value* value);
  virtual llvm::Function* irFunction() { return m_irFunction; }
  virtual void setIrFunction(llvm::Function* function);
public:
  llvm::Function* m_irFunction;
};

class AstFunDecl : public AstValue {
public:
  AstFunDecl(const std::string& name,
    std::list<AstArgDecl*>* args = NULL,
    std::shared_ptr<const ObjType> ret = nullptr,
    std::shared_ptr<SymbolTableEntry> stentry = nullptr);
  AstFunDecl(const std::string& name, AstArgDecl* arg1,
    AstArgDecl* arg2 = NULL, AstArgDecl* arg3 = NULL);
  ~AstFunDecl();
  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;
  virtual const std::string& name() const { return m_name; }
  virtual std::list<AstArgDecl*>const& args() const { return *m_args; }
  virtual const ObjType& retObjType() const;
  virtual const ObjType& objType() const;
  static std::list<AstArgDecl*>* createArgs(AstArgDecl* arg1 = NULL,
    AstArgDecl* arg2 = NULL, AstArgDecl* arg3 = NULL);
  virtual SymbolTableEntry* stentry() const { return m_stentry.get(); }

private:
  void initObjType();

  const std::string m_name;
  /** We're the owner. Is garanteed to be non-null */
  std::list<AstArgDecl*>* const m_args;
  const std::shared_ptr<const ObjType> m_ret;
  const std::shared_ptr<SymbolTableEntry> m_stentry;

// decorations for IrGen
public:
  virtual llvm::Value* irValue() { return m_irFunction; }
  virtual void setIrValue(llvm::Value* value);
  virtual llvm::Function* irFunction() { return m_irFunction; }
  virtual void setIrFunction(llvm::Function* function);
public:
  llvm::Function* m_irFunction;
};

class AstDataDecl : public AstValue {
public:
  AstDataDecl(const std::string& name, ObjType* objType,
    std::shared_ptr<SymbolTableEntry> stentry = nullptr);
  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;
  virtual const std::string& name() const { return m_name; }
  virtual const ObjType& objType() const;
  virtual std::shared_ptr<const ObjType>& objTypeShareOwnership();
  virtual SymbolTableEntry* stentry() const { return m_stentry.get(); }
  virtual void setStentry(std::shared_ptr<SymbolTableEntry> stentry);
  virtual Access access() const { return m_access; }
  virtual void setAccess(Access access, ErrorHandler& ) { m_access = access; }
  
private:
  const std::string m_name;
  /** Guaranteed to be non-null */
  std::shared_ptr<const ObjType> m_objType;
  /** NULL means this DataDecl was not yet put into the environment */
  std::shared_ptr<SymbolTableEntry> m_stentry;
  Access m_access;

// decorations for IrGen
public:
  virtual llvm::Value* irValue() { return m_irValue; }
  virtual void setIrValue(llvm::Value* value);
public:
  llvm::Value* m_irValue;
};

class AstArgDecl : public AstDataDecl {
public:
  AstArgDecl(const std::string& name, ObjType* objType) :
    AstDataDecl(name, &objType->addQualifiers(ObjType::eMutable)) {};
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
  virtual const ObjType& objType() const;
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
  virtual void setIrValue(llvm::Value* value);
public:
  llvm::Value* m_irValue;
};

/** Literal number */
class AstNumber : public AstValue {
public:
  AstNumber(int value, ObjTypeFunda* objType = NULL);
  AstNumber(int value, ObjTypeFunda::EType eType,
    ObjTypeFunda::Qualifiers qualifiers = ObjTypeFunda::eNoQualifier);
  ~AstNumber();
  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;
  int value() const { return m_value; }
  virtual const ObjTypeFunda& objType() const { return *m_objType; }
private:
  const int m_value;
  /** We're the owner. Is garanteed to be non-null. */
  const ObjTypeFunda*const m_objType;

// decorations for IrGen
public:
  virtual llvm::Value* irValue() { return m_irValue; }
  virtual void setIrValue(llvm::Value* value);
public:
  llvm::Value* m_irValue;
};

class AstSymbol : public AstValue {
public:
  AstSymbol(const std::string& name, Access access = eRead) :
    m_name(name), m_access(access), m_stentry(NULL), m_irValue(NULL) {};
  virtual ~AstSymbol() {};
  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;
  const std::string& name() const { return m_name; }
  virtual const ObjType& objType() const;
  virtual Access access() const { return m_access; }
  virtual void setAccess(Access access, ErrorHandler& ) { m_access = access; }
  virtual SymbolTableEntry* stentry() { return m_stentry.get(); }
  virtual void setStentry(std::shared_ptr<SymbolTableEntry> stentry);

private:
  const std::string m_name;
  Access m_access;
  /** We're not the owner, can be NULL */
  std::shared_ptr<SymbolTableEntry> m_stentry;

// decorations for IrGen
public:
  virtual llvm::Value* irValue() { return m_irValue; }
  virtual void setIrValue(llvm::Value* value);
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
  virtual const ObjType& objType() const;
private:
  /** We're the owner. Is garanteed to be non-null */
  AstValue* const m_address;
  /** We're the owner. Is garanteed to be non-null */
  AstCtList* const m_args;
  mutable std::unique_ptr<const ObjType> m_ret;

// decorations for IrGen
public:
  virtual llvm::Value* irValue() { return m_irValue; }
  virtual void setIrValue(llvm::Value* value);
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
    eNot = '!',
    eSeq = ';',
    eAnd = 128,
    eOr,
    eEqualTo,
    eDotAssign
  };
  enum EClass {
    eAssignment,
    eArithmetic,
    eLogical,
    eComparison,
    eOther
  };
  AstOperator(char op, AstCtList* args);
  AstOperator(const std::string& op, AstCtList* args);
  AstOperator(EOperation op, AstCtList* args);
  AstOperator(char op, AstValue* operand1 = NULL, AstValue* operand2 = NULL);
  AstOperator(const std::string& op, AstValue* operand1 = NULL, AstValue* operand2 = NULL);
  AstOperator(EOperation op, AstValue* operand1 = NULL, AstValue* operand2 = NULL);
  virtual ~AstOperator();
  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;
  virtual Access access() const { return m_access; }
  virtual void setAccess(Access access, ErrorHandler& );
  EOperation op() const { return m_op; }
  AstCtList& args() const { return *m_args; }
  virtual const ObjType& objType() const;
  void setObjType(std::unique_ptr<ObjType> objType);
  EClass class_() const;
  static EClass classOf(AstOperator::EOperation op);
  static EOperation toEOperation(const std::string& op);

private:
  friend std::basic_ostream<char>& operator<<(std::basic_ostream<char>&,
    AstOperator::EOperation);

  AstOperator(const AstOperator&);
  AstOperator& operator=(const AstOperator&);

  const EOperation m_op;
  /** We're the owner. Is garanteed to be non-null */
  AstCtList* const m_args;
  std::unique_ptr<ObjType> m_objType;
  Access m_access;
  static const std::map<const std::string, const EOperation> m_opMap;
  static const std::map<const EOperation, const std::string> m_opReverseMap;

// decorations for IrGen
public:
  virtual llvm::Value* irValue() { return m_irValue; }
  virtual void setIrValue(llvm::Value* value);
public:
  llvm::Value* m_irValue;
};

std::basic_ostream<char>& operator<<(std::basic_ostream<char>& os,
  AstOperator::EOperation op);

/* If flow control expression */
class AstIf : public AstValue {
public:
  AstIf(AstValue* cond, AstValue* action, AstValue* elseAction = NULL);
  virtual ~AstIf();
  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;
  AstValue& condition() const { return *m_condition; }
  AstValue& action() const { return *m_action; }
  AstValue* elseAction() const { return m_elseAction; }
  virtual const ObjType& objType() const;
  void setObjType(std::unique_ptr<ObjType> objType);

private:
  /** We're the owner. Is garanteed to be non-null */
  AstValue* const m_condition;
  /** We're the owner. Is garanteed to be non-null */
  AstValue* const m_action;
  /** We're the owner. Is NOT garanteed to be non-null */
  AstValue* const m_elseAction;
  std::unique_ptr<ObjType> m_objType;

// decorations for IrGen
public:
  virtual llvm::Value* irValue() { return m_irValue; }
  virtual void setIrValue(llvm::Value* value);
public:
  llvm::Value* m_irValue;
};

class AstCtList : public AstNode {
public:
  AstCtList(std::list<AstValue*>* childs);
  AstCtList(AstValue* child1 = NULL);
  AstCtList(AstValue* child1, AstValue* child2, AstValue* child3 = NULL);
  ~AstCtList();
  void releaseOwnership();
  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;
  AstCtList* Add(AstValue* child);
  AstCtList* Add(AstValue* child1, AstValue* child2, AstValue* child3 = NULL);
  /** The elements are guaranteed to be non-null */
  std::list<AstValue*>& childs() const { return *m_childs; }
  virtual const ObjType& objType() const;

private:
  AstCtList(const AstCtList&);
  AstCtList& operator=(const AstCtList&);

  /** We're the owner of the list and of the pointees. Pointers are garanteed
  to be non null*/
  std::list<AstValue*>*const m_childs;
  bool m_owner = true;

  // decorations for IrGen
public:
  virtual llvm::Value* irValue() { static llvm::Value* v = NULL; return v; }
  virtual void setIrValue(llvm::Value*) { }
};

#endif
