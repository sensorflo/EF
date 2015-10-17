#pragma once
// If you change this list of header files, you must also modify the
// analogous, redundant list in the Makefile
#include "objtype.h"
#include "access.h"
#include "generalvalue.h"
#include "storageduration.h"

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
class Object;

class AstNode {
public:
  AstNode(Access access = eRead) : m_access(access) {};
  virtual ~AstNode() {};

  virtual void accept(AstVisitor& visitor) =0;
  virtual void accept(AstConstVisitor& visitor) const =0;
  /** Access to this node. Contrast this with access to the object refered to
  by this node. */
  virtual Access access() const { return m_access; }
  virtual void setAccess(Access access);
  std::string toStr() const;

protected:
  Access m_access;
};

class AstValue : public AstNode {
public:
  AstValue(Access access, std::shared_ptr<SymbolTableEntry> stentry);
  AstValue(Access access = eRead);
  AstValue(std::shared_ptr<SymbolTableEntry> stentry);
  virtual ~AstValue();

  virtual bool isCTConst() const { return false; }

  // associated object
  /** After SemanticAnalizer guaranteed to return non-null */
 	SymbolTableEntry* stentry() const { return m_stentry.get(); }
  std::shared_ptr<SymbolTableEntry>& stentryAsSp() { return m_stentry; }
 	void setStentry(std::shared_ptr<SymbolTableEntry> stentry);
 	const ObjType& objType() const;
  bool objectIsModifiedOrRevealsAddr() const;
  
protected:
  std::shared_ptr<SymbolTableEntry> m_stentry;
};

class AstNop : public AstValue {
public:
  AstNop();
  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;
};

class AstBlock : public AstValue {
public:
  AstBlock(AstValue* body);
  virtual ~AstBlock();
  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;
  AstValue& body() const { return *m_body.get(); }

private:
  std::unique_ptr<AstValue> m_body;
};

class AstCast : public AstValue {
public:
  AstCast(ObjType* objType, AstValue* child);
  AstCast(ObjTypeFunda::EType objType, AstValue* child);
  virtual ~AstCast();
  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;
  AstValue& child() const { return *m_child; }

private:
  /** We're the owner. Is guaranteed to be non-null */
  AstValue* m_child;
};

class AstFunDef : public AstValue {
public:
  AstFunDef(const std::string& name,
    std::shared_ptr<SymbolTableEntry> stentry,
    std::shared_ptr<const ObjType> ret,
    AstValue* body);
  AstFunDef(const std::string& name,
    std::shared_ptr<SymbolTableEntry> stentry,
    std::list<AstArgDecl*>* args,
    std::shared_ptr<const ObjType> ret,
    AstValue* body);
  virtual ~AstFunDef();
  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;
  virtual const std::string& name() const { return m_name; }
  virtual std::list<AstArgDecl*>const& args() const { return *m_args; }
  virtual const ObjType& retObjType() const;
  virtual AstValue& body() const { return *m_body; }
  static std::list<AstArgDecl*>* createArgs(AstArgDecl* arg1 = NULL,
    AstArgDecl* arg2 = NULL, AstArgDecl* arg3 = NULL);

private:
  void initObjType();

  const std::string m_name;
  /** We're the owner. Is garanteed to be non-null */
  std::list<AstArgDecl*>* const m_args;
  const std::shared_ptr<const ObjType> m_ret;
  /** We're the owner. Is garanteed to be non-null */
  AstValue* const m_body;
};

class AstDataDecl : public AstValue {
public:
  AstDataDecl(const std::string& name, const ObjType* declaredObjType,
    StorageDuration declaredStorageDuration = StorageDuration::eLocal);
  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;
  virtual const std::string& name() const { return m_name; }

  const ObjType& declaredObjType() const;
  std::shared_ptr<const ObjType>& declaredObjTypeAsSp();
  StorageDuration declaredStorageDuration() const;
  std::shared_ptr<SymbolTableEntry>& createAndSetStEntryUsingDeclaredObjType();  

private:
  const std::string m_name;
  std::shared_ptr<const ObjType> m_declaredObjType;
  const StorageDuration m_declaredStorageDuration;
};

class AstArgDecl : public AstDataDecl {
public:
  AstArgDecl(const std::string& name, const ObjType* declaredObjType) :
    AstDataDecl(name, declaredObjType, StorageDuration::eLocal) {};
  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;
};

class AstDataDef : public AstValue {
public:
  AstDataDef(AstDataDecl* decl, AstValue* initValue);
  AstDataDef(AstDataDecl* decl, AstCtList* ctorArgs = NULL);
  virtual ~AstDataDef();
  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;
  virtual AstDataDecl& decl() const { return *m_decl; }
  AstCtList& ctorArgs() const { return *m_ctorArgs; }
  virtual AstValue& initValue() const;

private:
  /** We're the owner. Is garanteed to be non-null */
  AstDataDecl* const m_decl;
  /** We're the owner. Is garanteed to be non-null. */
  AstCtList* const m_ctorArgs;
  /** We're the owner. Is _NOT_ guaranteed to  be non-null. */
  AstValue* m_implicitInitializer;
};

/** Literal number */
class AstNumber : public AstValue {
public:
  AstNumber(GeneralValue value, ObjTypeFunda* objType = NULL);
  AstNumber(GeneralValue value, ObjTypeFunda::EType eType,
    ObjTypeFunda::Qualifiers qualifiers = ObjTypeFunda::eNoQualifier);

 	const ObjTypeFunda& objType() const;

  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;
  GeneralValue value() const { return m_value; }
  virtual bool isCTConst() const { return true; }
private:
  const GeneralValue m_value;
};

class AstSymbol : public AstValue {
public:
  AstSymbol(const std::string& name, Access access = eRead) :
    AstValue(access), m_name(name) { }
  virtual ~AstSymbol() {};
  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;
  const std::string& name() const { return m_name; }

private:
  const std::string m_name;
};

class AstFunCall : public AstValue {
public:
  AstFunCall(AstValue* address, AstCtList* args = NULL);
  virtual ~AstFunCall();
  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;
  virtual AstValue& address () const { return *m_address; }
  AstCtList& args () const { return *m_args; }

  void createAndSetStEntryUsingRetObjType();

private:
  /** We're the owner. Is garanteed to be non-null */
  AstValue* const m_address;
  /** We're the owner. Is garanteed to be non-null */
  AstCtList* const m_args;
};

class AstOperator : public AstValue {
public:
  enum EOperation {
    eAssign = '=',
    eAdd = '+',
    eSub = '-',
    eMul = '*', // '*' is ambigous. can also mean eDeref
    eDiv = '/',
    eNot = '!',
    eSeq = ';',
    eAddrOf = '&',
    eAnd = 128,
    eOr,
    eEqualTo,
    eDotAssign,
    eDeref
  };
  enum EClass {
    eAssignment,
    eArithmetic,
    eLogical,
    eComparison,
    eMemberAccess,
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
  EOperation op() const { return m_op; }
  AstCtList& args() const { return *m_args; }
  EClass class_() const;
  static EClass classOf(AstOperator::EOperation op);
  /** In case of ambiguity, chooses the binary operator */
  static EOperation toEOperationPreferingBinary(const std::string& op);
  /** In case of ambiguity asserts */
  static EOperation toEOperation(const std::string& op);

private:
  friend std::basic_ostream<char>& operator<<(std::basic_ostream<char>&,
    AstOperator::EOperation);

  AstOperator(const AstOperator&);
  AstOperator& operator=(const AstOperator&);

  const EOperation m_op;
  /** We're the owner. Is garanteed to be non-null */
  AstCtList* const m_args;
  static const std::map<const std::string, const EOperation> m_opMap;
  static const std::map<const EOperation, const std::string> m_opReverseMap;
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

private:
  /** We're the owner. Is garanteed to be non-null */
  AstValue* const m_condition;
  /** We're the owner. Is garanteed to be non-null */
  AstValue* const m_action;
  /** We're the owner. Is NOT garanteed to be non-null */
  AstValue* const m_elseAction;
};

class AstLoop : public AstValue {
public:
  AstLoop(AstValue* cond, AstValue* body);
  virtual ~AstLoop();
  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;
  AstValue& condition() const { return *m_condition; }
  AstValue& body() const { return *m_body; }

private:
  /** We're the owner. Is garanteed to be non-null */
  AstValue* const m_condition;
  /** We're the owner. Is garanteed to be non-null */
  AstValue* const m_body;
};

class AstReturn : public AstValue {
public:
  AstReturn(AstValue* retVal);
  virtual ~AstReturn();
  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;

  AstValue& retVal() const;

private:
  /** Is garanteed to be non-null */
  const std::unique_ptr<AstValue> m_retVal;
};

class AstCtList : public AstNode {
public:
  AstCtList(std::list<AstValue*>* childs);
  AstCtList(AstValue* child1 = NULL);
  AstCtList(AstValue* child1, AstValue* child2, AstValue* child3 = NULL,
    AstValue* child4 = NULL, AstValue* child5 = NULL, AstValue* child6 = NULL);
  virtual ~AstCtList();
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
};
