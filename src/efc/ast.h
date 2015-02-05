#ifndef AST_H
#define AST_H
#include "objtype.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Function.h"
#include <string>
#include <list>
#include <iostream>
#include <map>

class AstNode;
class AstNumber;
class AstSymbol;
class AstFunCall;
class AstOperator;
class AstCast;
class AstCtList;
class AstFunDef;
class AstFunDecl;
class AstDataDecl;
class AstArgDecl;
class AstDataDef;
class AstIf;

class IrBuilderAst;

enum Access { eRead, eWrite };

class AstVisitor {
public:
  virtual ~AstVisitor() {};
  virtual void visit(const AstCast& cast) =0;
  virtual void visit(const AstCtList& ctList) =0;
  virtual void visit(const AstOperator& op) =0;
  virtual void visit(const AstNumber& number) =0;
  virtual void visit(const AstSymbol& symbol) =0;
  virtual void visit(const AstFunCall& funCall) =0;
  virtual void visit(const AstFunDef& funDef) =0;
  virtual void visit(const AstFunDecl& funDecl) =0;
  virtual void visit(const AstDataDecl& dataDecl) =0;
  virtual void visit(const AstDataDef& dataDef) =0;
  virtual void visit(const AstIf& if_) =0;
};

class AstNode {
public:
  virtual ~AstNode() {};
  virtual void accept(AstVisitor& visitor) const =0;
  virtual llvm::Value* accept(IrBuilderAst& visitor, Access access = eRead) const = 0;
  /** Similar to printTo; the resulting text is returned as string. */
  virtual std::string toStr() const;
  /** Print node recursively in its canonical form to given stream */
  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>&) const = 0;
};

class AstValue : public AstNode {
public:
  /** A workaround to have an early notion of an address of data objects or
  code objects. */
  virtual const std::string& address_as_id_hack() const; 
  virtual ObjType& objType() const;
};

class AstCast : public AstValue {
public:
  AstCast(AstValue* child, ObjType* objType);
  AstCast(AstValue* child, ObjTypeFunda::EType objType);
  ~AstCast();
  virtual void accept(AstVisitor& visitor) const { visitor.visit(*this); };
  virtual llvm::Value* accept(IrBuilderAst& visitor, Access access = eRead) const;
  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>& os) const; 

private:
  /** We're the owner. Is guaranteed to be non-null */
  const AstValue* m_child;
  /** We're the owner. Is garanteed to be non-null. */
  ObjType* m_objType;
};

class AstFunDef : public AstValue {
public:
  AstFunDef(AstFunDecl* decl, AstValue* body);
  virtual ~AstFunDef();
  virtual void accept(AstVisitor& visitor) const { visitor.visit(*this); }
  virtual llvm::Function* accept(IrBuilderAst& visitor, Access access = eRead) const;
  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>&) const;
  virtual const AstFunDecl& decl() const { return *m_decl; }
  virtual const AstValue& body() const { return *m_body; }
private:
  /** We're the owner. Is garanteed to be non-null */
  const AstFunDecl* const m_decl;
  /** We're the owner. Is garanteed to be non-null */
  const AstValue* const m_body;
};

class AstFunDecl : public AstValue {
public:
  AstFunDecl(const std::string& name, std::list<AstArgDecl*>* args = NULL);
  AstFunDecl(const std::string& name, AstArgDecl* arg1,
    AstArgDecl* arg2 = NULL, AstArgDecl* arg3 = NULL);
  ~AstFunDecl();
  virtual void accept(AstVisitor& visitor) const { visitor.visit(*this); };
  virtual llvm::Function* accept(IrBuilderAst& visitor, Access access = eRead) const;
  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>&) const;
  virtual const std::string& name() const { return m_name; }
  virtual const std::list<AstArgDecl*>& args() const { return *m_args; }
  static std::list<AstArgDecl*>* createArgs(AstArgDecl* arg1 = NULL,
    AstArgDecl* arg2 = NULL, AstArgDecl* arg3 = NULL);
private:
  const std::string m_name;
  /** We're the owner. Is garanteed to be non-null */
  const std::list<AstArgDecl*>* const m_args;
};

class AstDataDecl : public AstValue {
public:
  AstDataDecl(const std::string& name, ObjType* objType);
  ~AstDataDecl();
  virtual void accept(AstVisitor& visitor) const { visitor.visit(*this); };
  virtual llvm::Value* accept(IrBuilderAst& visitor, Access access = eRead) const;
  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>&) const;
  virtual const std::string& name() const { return m_name; }
  virtual ObjType& objType() const;
  virtual ObjType& objType(bool stealOwnership) const;
protected:
  enum Type { eNormal, eFunArg };
  AstDataDecl(const std::string& name, ObjType* objType, Type type);
  
private:
  const std::string m_name;
  /** If m_ownerOfObjType is true, we're the owner. Is garanteed to be
  non-null. */
  ObjType* m_objType;
  /** See m_objType */
  mutable bool m_ownerOfObjType;
  const Type m_type;
};

class AstArgDecl : public AstDataDecl {
public:
  AstArgDecl(const std::string& name, ObjType* objType) :
    AstDataDecl(name, &objType->addQualifier(ObjType::eMutable), eFunArg) {};
};

class AstDataDef : public AstValue {
public:
  AstDataDef(AstDataDecl* decl, AstValue* initValue);
  AstDataDef(AstDataDecl* decl, AstCtList* ctorArgs = NULL);
  ~AstDataDef();
  virtual void accept(AstVisitor& visitor) const { visitor.visit(*this); };
  virtual llvm::Value* accept(IrBuilderAst& visitor, Access access = eRead) const;
  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>&) const;
  virtual const AstDataDecl& decl() const { return *m_decl; }
  virtual const AstValue& initValue() const;
private:
  /** We're the owner. Is garanteed to be non-null */
  const AstDataDecl* const m_decl;
  /** We're the owner. Is garanteed to be non-null. */
  AstCtList* const m_ctorArgs;
  /** We're the owner. Is _NOT_ guaranteed to  be non-null. */
  AstValue* m_implicitInitializer;
};

/** Literal number */
class AstNumber : public AstValue {
public:
  AstNumber(int value, ObjType* objType = NULL);
  AstNumber(int value, ObjTypeFunda::EType eType,
    ObjTypeFunda::Qualifier qualifier = ObjTypeFunda::eNoQualifier);
  ~AstNumber();
  virtual void accept(AstVisitor& visitor) const { visitor.visit(*this); };
  virtual llvm::Value* accept(IrBuilderAst& visitor, Access access = eRead) const;
  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>& os) const; 
  int value() const { return m_value; }
  virtual ObjType& objType() const { return *m_objType; }
private:
  const int m_value;
  /** We're the owner. Is garanteed to be non-null. */
  ObjType* m_objType;
};

class AstSymbol : public AstValue {
public:
  AstSymbol(const std::string* name);
  virtual ~AstSymbol();
  virtual void accept(AstVisitor& visitor) const { visitor.visit(*this); };
  virtual llvm::Value* accept(IrBuilderAst& visitor, Access access = eRead) const;
  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>& os) const; 
  const std::string& name() const { return *m_name; }
  virtual const std::string& address_as_id_hack() const { return *m_name; }
private:
  /** We're the owner. Is garanteed to be non-null */
  const std::string* const m_name;
};

class AstFunCall : public AstValue {
public:
  AstFunCall(const AstValue* address, AstCtList* args = NULL);
  virtual ~AstFunCall();
  virtual void accept(AstVisitor& visitor) const { visitor.visit(*this); };
  virtual llvm::Value* accept(IrBuilderAst& visitor, Access access = eRead) const;
  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>& os) const; 
  virtual const AstValue& address () const { return *m_address; }
  virtual const AstCtList& args () const { return *m_args; }
private:
  /** We're the owner. Is garanteed to be non-null */
  const AstValue*const m_address;
  /** We're the owner. Is garanteed to be non-null */
  const AstCtList* const m_args;
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
  virtual void accept(AstVisitor& visitor) const { visitor.visit(*this); };
  virtual llvm::Value* accept(IrBuilderAst& visitor, Access access = eRead) const;
  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>& os) const; 
  EOperation op() const { return m_op; }
  const std::list<AstValue*>& argschilds() const;
private:
  static EOperation toEOperation(const std::string& op);
  AstOperator(const AstOperator&);
  AstOperator& operator=(const AstOperator&);
  const EOperation m_op;
  /** We're the owner. Is garanteed to be non-null */
  const AstCtList* const m_args;
  static std::map<std::string, EOperation> m_opMap;
};

std::basic_ostream<char>& operator<<(std::basic_ostream<char>& os,
  AstOperator::EOperation op);

class AstSeq : public AstOperator {
public:
  AstSeq(std::list<AstValue*>* childs = NULL);
  AstSeq(AstValue* child1, AstValue* child2 = NULL, AstValue* child3 = NULL);
};

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
  virtual void accept(AstVisitor& visitor) const { visitor.visit(*this); };
  virtual llvm::Value* accept(IrBuilderAst& visitor, Access access = eRead) const;
  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>& os) const; 
  const std::list<ConditionActionPair>& conditionActionPairs() const { return *m_conditionActionPairs; }
  const AstValue* elseAction() const { return m_elseAction; }
private:
  static std::list<ConditionActionPair>* makeDefaultConditionActionPairs();
  static std::list<ConditionActionPair>* makeConditionActionPairs(
    AstValue* cond, AstValue* action);
  /** We're the owner. Is garanteed to be non-null and size>=1*/
  const std::list<ConditionActionPair>* const m_conditionActionPairs;
  /** We're the owner. Is NOT garanteed to be non-null */
  const AstValue* const m_elseAction;
};

class AstCtList : public AstNode {
public:
  AstCtList(std::list<AstValue*>* childs);
  AstCtList(AstValue* child1 = NULL);
  AstCtList(AstValue* child1, AstValue* child2, AstValue* child3 = NULL);
  ~AstCtList();
  virtual void accept(AstVisitor& visitor) const { visitor.visit(*this); };
  virtual llvm::Value* accept(IrBuilderAst& visitor, Access access = eRead) const;
  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>&) const;
  AstCtList* Add(AstValue* child);
  AstCtList* Add(AstValue* child1, AstValue* child2, AstValue* child3 = NULL);
  /** The elements are guaranteed to be non-null */
  const std::list<AstValue*>& childs() const { return *m_childs; }
private:
  AstCtList(const AstCtList&);
  AstCtList& operator=(const AstCtList&);

  /** We're the owner of the list and of the pointees. Pointers are garanteed
  to be non null*/
  std::list<AstValue*>*const m_childs;
};

#endif
