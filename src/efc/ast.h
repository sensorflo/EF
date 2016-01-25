#pragma once
// If you change this list of header files, you must also modify the
// analogous, redundant list in the Makefile
#include "objtype.h"
#include "access.h"
#include "generalvalue.h"
#include "storageduration.h"
#include "declutils.h"

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
class Object;
class ErrorHandler;

/** See also Entity */
class AstNode {
public:
  virtual ~AstNode() = default;

  virtual void accept(AstVisitor& visitor) =0;
  virtual void accept(AstConstVisitor& visitor) const =0;

  /** See AstObject::m_access. For non-AstObject types, access can only be
  eIgnore. It's a kludge that this method is a member of AstNode, the author
  hasn't found another way to implement SemanticAnalizer::visit(AstSeq& seq).*/
  virtual void setAccess(Access access) { assert(access==eIgnore); }
  /** For AstObject's, identical to objType().isNoreturn();, for all others it
  always returns false. It's a kludge that this method is a member of AstNode,
  see also setAccess. */
  virtual bool isObjTypeNoReturn() const { return false;}

  std::string toStr() const;

protected:
  AstNode() = default;

private:
  NEITHER_COPY_NOR_MOVEABLE(AstNode);
};

/** See also Object */
class AstObject : public AstNode {
public:
  virtual ~AstObject();

  virtual bool isCTConst() const { return false; }
  virtual Access access() const { return m_access; }
  virtual void setAccess(Access access) override;

  virtual bool isObjTypeNoReturn() const override { return objType().isNoreturn(); }

  // associated object
  /** After SemanticAnalizer guaranteed to return non-null */
 	Object* object() const { return m_object.get(); }
  std::shared_ptr<Object>& objectAsSp() { return m_object; }
 	void setObject(std::shared_ptr<Object> object);
  virtual const ObjType& objType() const;
  virtual std::shared_ptr<const ObjType> objTypeAsSp() const;
  bool objectIsModifiedOrRevealsAddr() const;
  
protected:
  AstObject(Access access, std::shared_ptr<Object> object);
  AstObject(Access access = eRead);
  AstObject(std::shared_ptr<Object> object);

  /** Access to this AST node. Contrast this with access to the object refered
  to by this AST node. */
  Access m_access;
  std::shared_ptr<Object> m_object;
};

class AstNop : public AstObject {
public:
  AstNop();
  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;
};

class AstBlock : public AstObject {
public:
  AstBlock(AstObject* body);
  virtual ~AstBlock();

  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;
  const std::string& name() const { return m_name; }
  AstObject& body() const { return *m_body.get(); }

private:
  static std::string makeUniqueInternalName();
  const std::string m_name;
  /** Guaranteed to be non-null */
  std::unique_ptr<AstObject> m_body;
};

class AstCast : public AstObject {
public:
  AstCast(AstObjType* specifiedNewAstObjType, AstObject* child);
  AstCast(ObjTypeFunda::EType specifiedNewOjType, AstObject* child);
  virtual ~AstCast();

  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;
  AstObject& child() const { return *m_child; }

  AstObjType& specifiedNewAstObjType() const;

private:
  /** Is guaranteed to be non-null */
  const std::unique_ptr<AstObjType> m_specifiedNewAstObjType;
  /** We're the owner. Is guaranteed to be non-null */
  AstObject* m_child;
};

class AstFunDef : public AstObject {
public:
  AstFunDef(const std::string& name,
    std::list<AstDataDef*>* args,
    AstObjType* ret,
    AstObject* body);
  virtual ~AstFunDef();

  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;
  virtual const std::string& name() const { return m_name; }
  virtual std::list<AstDataDef*>const& declaredArgs() const { return *m_args; }
  virtual AstObjType& declaredRetAstObjType() const;
  virtual AstObject& body() const { return *m_body; }
  static std::list<AstDataDef*>* createArgs(AstDataDef* arg1 = NULL,
    AstDataDef* arg2 = NULL, AstDataDef* arg3 = NULL);

  void assignDeclaredObjTypeToAssociatedObject();

private:
  void initObjType();

  /** Redundant to the key of Env's key-value pair pointing to object(). */
  const std::string m_name;
  /** We're the owner. Is garanteed to be non-null. The shared_ptr<ObjType>
  instances are redundant to dynamic_cast<ObjTypeFun>(objType()).args().  */
  std::list<AstDataDef*>* const m_args;
  /** Is garanteed to be non-null. Is redundant to
  dynamic_cast<ObjTypeFun>(objType()).ret().  */
  const std::unique_ptr<AstObjType> m_ret;
  /** We're the owner. Is garanteed to be non-null */
  AstObject* const m_body;
};

/** Also used for data members of a class, which is not entirerly a nice
design since a member declaration is not yet an object, thus subtyping from
AstObject is wrong in this case. */
class AstDataDef : public AstObject {
public:
  static AstObject* const noInit;

  AstDataDef(const std::string& name, AstObjType* declaredAstObjType,
    StorageDuration declaredStorageDuration,
    AstCtList* ctorArgs = nullptr);
  AstDataDef(const std::string& name, AstObjType* declaredAstObjType,
    StorageDuration declaredStorageDuration,
    AstObject* initObj);
  AstDataDef(const std::string& name, AstObjType* declaredAstObjType,
    AstObject* initObj = nullptr);
  AstDataDef(const std::string& name, ObjTypeFunda::EType declaredObjType,
    AstObject* initObj = nullptr);
  virtual ~AstDataDef();

  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;

  virtual const ObjType& objType() const;
  virtual std::shared_ptr<const ObjType> objTypeAsSp() const;

  virtual const std::string& name() const { return m_name; }
  AstObjType& declaredAstObjType() const;
  StorageDuration declaredStorageDuration() const;
  AstCtList& ctorArgs() const { return *m_ctorArgs; }
  bool doNotInit() const { return m_doNotInit; }

  void assignDeclaredObjTypeToAssociatedObject();

private:
  friend class AstPrinter;

  static AstCtList* mkCtorArgs(AstCtList* ctorArgs,
    StorageDuration storageDuration, bool& doNotInit);

  const std::string m_name;
  std::unique_ptr<AstObjType> m_declaredAstObjType;
  /** Redundant with m_object->objType(). But since in the case of
  m_declaredStorageDuration being eMember, there is no m_object, and then we
  need to store it here.*/
  std::shared_ptr<const ObjType> m_objType;
  /** Guaranteed to be not eUnknown */
  const StorageDuration m_declaredStorageDuration;
  /** We're the owner. Is garanteed to be non-null. Currently, zero args means
  to default initialize: semantic analizer will add an initilizer*/
  AstCtList* const m_ctorArgs;
  bool m_doNotInit;
};

/** Literal 'scalar', where scalar means 'not an aggregation /
composition'. Obviously a well defined term is needed, and then the class
should be renamed. */
class AstNumber : public AstObject {
public:
  AstNumber(GeneralValue value, AstObjType* astObjType = nullptr);
  AstNumber(GeneralValue value, ObjTypeFunda::EType eType);
  ~AstNumber();

  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;

  GeneralValue value() const { return m_value; }
  AstObjType& declaredAstObjType() const;

  virtual bool isCTConst() const { return true; }

private:
  const GeneralValue m_value;
  /** Guaranteed to be non-null */
  const std::unique_ptr<AstObjType> m_declaredAstObjType;
};

/** Here symbol as an synonym to identifier */
class AstSymbol : public AstObject {
public:
  AstSymbol(const std::string& name, Access access = eRead) :
    AstObject(access), m_name(name) { }

  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;
  const std::string& name() const { return m_name; }

private:
  const std::string m_name;
};

class AstFunCall : public AstObject {
public:
  AstFunCall(AstObject* address, AstCtList* args = NULL);
  virtual ~AstFunCall();

  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;
  virtual AstObject& address () const { return *m_address; }
  AstCtList& args () const { return *m_args; }

  void createAndSetObjectUsingRetObjType();

private:
  /** We're the owner. Is garanteed to be non-null */
  AstObject* const m_address;
  /** We're the owner. Is garanteed to be non-null */
  AstCtList* const m_args;
};

class AstOperator : public AstObject {
public:
  enum EOperation {
    eAssign = '=',
    eAdd = '+',
    eSub = '-',
    eMul = '*', // '*' is ambigous. can also mean eDeref
    eDiv = '/',
    eNot = '!',
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
  AstOperator(char op, AstObject* operand1 = NULL, AstObject* operand2 = NULL);
  AstOperator(const std::string& op, AstObject* operand1 = NULL, AstObject* operand2 = NULL);
  AstOperator(EOperation op, AstObject* operand1 = NULL, AstObject* operand2 = NULL);
  virtual ~AstOperator();

  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;
  EOperation op() const { return m_op; }
  AstCtList& args() const { return *m_args; }
  EClass class_() const;
  bool isBinaryLogicalShortCircuit() const;
  static EClass classOf(AstOperator::EOperation op);
  /** In case of ambiguity, chooses the binary operator */
  static EOperation toEOperationPreferingBinary(const std::string& op);
  /** In case of ambiguity asserts */
  static EOperation toEOperation(const std::string& op);

private:
  friend std::basic_ostream<char>& operator<<(std::basic_ostream<char>&,
    AstOperator::EOperation);

  const EOperation m_op;
  /** We're the owner. Is garanteed to be non-null */
  AstCtList* const m_args;
  static const std::map<const std::string, const EOperation> m_opMap;
  static const std::map<const EOperation, const std::string> m_opReverseMap;
};

std::basic_ostream<char>& operator<<(std::basic_ostream<char>& os,
  AstOperator::EOperation op);

/** Has two responsibilities: 1) Be an ordered sequence of AstNode s 2) cast
the last AstNode to an AstObject and report an error if that is not
possible. */
class AstSeq : public AstObject {
public:
  AstSeq(std::vector<AstNode*>* operands);
  AstSeq(AstNode* op1 = nullptr, AstNode* op2 = nullptr,
    AstNode* op3 = nullptr, AstNode* op4 = nullptr, AstNode* op5 = nullptr);

  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;

  const std::vector<AstNode*>& operands() const { return *m_operands; }
  AstObject& lastOperand(ErrorHandler& errorHandler) const;

private:
  /** Pointers are garanteed to be non null. Garanteed to have at least one
  element. */
  std::unique_ptr<std::vector<AstNode*>> m_operands;
};

/* If flow control expression */
class AstIf : public AstObject {
public:
  AstIf(AstObject* cond, AstObject* action, AstObject* elseAction = NULL);
  virtual ~AstIf();

  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;
  AstObject& condition() const { return *m_condition; }
  AstObject& action() const { return *m_action; }
  AstObject* elseAction() const { return m_elseAction; }

private:
  /** We're the owner. Is garanteed to be non-null */
  AstObject* const m_condition;
  /** We're the owner. Is garanteed to be non-null */
  AstObject* const m_action;
  /** We're the owner. Is NOT garanteed to be non-null */
  AstObject* const m_elseAction;
};

class AstLoop : public AstObject {
public:
  AstLoop(AstObject* cond, AstObject* body);
  virtual ~AstLoop();

  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;
  AstObject& condition() const { return *m_condition; }
  AstObject& body() const { return *m_body; }

private:
  /** We're the owner. Is garanteed to be non-null */
  AstObject* const m_condition;
  /** We're the owner. Is garanteed to be non-null */
  AstObject* const m_body;
};

class AstReturn : public AstObject {
public:
  AstReturn(AstObject* retVal = nullptr);
  virtual ~AstReturn();

  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;

  AstObject& retVal() const;

private:
  /** Is garanteed to be non-null */
  const std::unique_ptr<AstObject> m_retVal;
};

/** See also ObjType */
class AstObjType : public AstNode {
public:
  /** Can only be called after this AstObjType has been visited by the
  SemanticAnalizer. */
  virtual const ObjType& objType() const =0; 
  virtual std::shared_ptr<const ObjType> objTypeAsSp() const =0; 
  virtual void createAndSetObjType() =0;
  virtual void printValueTo(std::ostream& os, GeneralValue value) const =0;
  virtual bool isValueInRange(GeneralValue value) const =0;
  /** The objType of the created AstObject is immutable, since the AstObject
  has the semantics of a temporary. Asserts in case of there is no default
  AstObject. The returned AstObject is set up as if it had passed all passes
  prior to SemanticAnalizer, but not yet passed SemanticAnalizer's pass. */
  virtual AstObject* createDefaultAstObjectForSemanticAnalizer() = 0;

  // decorations for IrGen
  virtual llvm::Value* createLlvmValueFrom(GeneralValue value) const =0;
};

class AstObjTypeSymbol : public AstObjType {
public:
  AstObjTypeSymbol(const std::string name);
  AstObjTypeSymbol(ObjTypeFunda::EType fundaType);

  void accept(AstVisitor& visitor) override;
  void accept(AstConstVisitor& visitor) const override;
  void printValueTo(std::ostream& os, GeneralValue value) const override;
  bool isValueInRange(GeneralValue value) const override;
  AstObject* createDefaultAstObjectForSemanticAnalizer() override;

  const ObjType& objType() const override; 
  std::shared_ptr<const ObjType> objTypeAsSp() const override; 

  const std::string name() const { return m_name; }

  void createAndSetObjType() override;

private:
  friend class TestingAstObjTypeSymbol;

  static std::string toName(ObjTypeFunda::EType fundaType);
  static ObjTypeFunda::EType toType(const std::string& name);
  static void initMap();
  static std::array<std::string, ObjTypeFunda::eTypeCnt> m_typeToName;
  static bool m_isMapInitialzied;

  const std::string m_name;
  std::shared_ptr<const ObjType> m_objType;

  // decorations for IrGen
public:
  virtual llvm::Value* createLlvmValueFrom(GeneralValue value) const override;
};

class AstObjTypeQuali : public AstObjType {
public:
  AstObjTypeQuali(ObjType::Qualifiers qualifiers, AstObjType* targetType);

  void accept(AstVisitor& visitor) override;
  void accept(AstConstVisitor& visitor) const override;
  void printValueTo(std::ostream& os, GeneralValue value) const override;
  bool isValueInRange(GeneralValue value) const override;
  AstObject* createDefaultAstObjectForSemanticAnalizer() override;

  const ObjType& objType() const override;
  std::shared_ptr<const ObjType> objTypeAsSp() const override; 

  ObjType::Qualifiers qualifiers() const { return m_qualifiers; }
  AstObjType& targetType() const { return *m_targetType; }

  void createAndSetObjType() override;

private:
  ObjType::Qualifiers m_qualifiers;
  std::unique_ptr<AstObjType> m_targetType;
  std::shared_ptr<const ObjType> m_objType;

  // decorations for IrGen
public:
  virtual llvm::Value* createLlvmValueFrom(GeneralValue value) const override;
};

class AstObjTypePtr : public AstObjType {
public:
  AstObjTypePtr(AstObjType* pointee);

  void accept(AstVisitor& visitor) override;
  void accept(AstConstVisitor& visitor) const override;
  void printValueTo(std::ostream& os, GeneralValue value) const override;
  bool isValueInRange(GeneralValue value) const override;
  AstObject* createDefaultAstObjectForSemanticAnalizer() override;

  const ObjTypePtr& objType() const override;
  std::shared_ptr<const ObjType> objTypeAsSp() const override; 

  AstObjType& pointee() const { return *m_pointee; }

  void createAndSetObjType() override;

private:
  std::unique_ptr<AstObjType> m_pointee;
  std::shared_ptr<const ObjTypePtr> m_objType;

  // decorations for IrGen
public:
  virtual llvm::Value* createLlvmValueFrom(GeneralValue value) const override;
};

/** Definition of a class. See also ObjTypeClass */
class AstClassDef : public AstObjType {
public:
  AstClassDef(const std::string& name, std::vector<AstDataDef*>* dataMembers);
  AstClassDef(const std::string& name, AstDataDef* m1 = nullptr,
    AstDataDef* m2 = nullptr, AstDataDef* m3 = nullptr);

  void accept(AstVisitor& visitor) override;
  void accept(AstConstVisitor& visitor) const override;
  void printValueTo(std::ostream& os, GeneralValue value) const override;
  bool isValueInRange(GeneralValue value) const override;
  AstObject* createDefaultAstObjectForSemanticAnalizer() override;

  const ObjTypeClass& objType() const override;
  std::shared_ptr<const ObjType> objTypeAsSp() const override; 

  const std::string& name() const { return m_name; }
  const std::vector<AstDataDef*>& dataMembers() const { return *m_dataMembers; }

  void createAndSetObjType() override;

private:
  const std::string m_name;
  /** We're the owner of the list and of the pointees. Pointers are garanteed
  to be non null, also the pointer to the vector.*/
  const std::unique_ptr<std::vector<AstDataDef*>> m_dataMembers;
  std::shared_ptr<const ObjTypeClass> m_objType;

  // decorations for IrGen
public:
  virtual llvm::Value* createLlvmValueFrom(GeneralValue value) const override;
};

/** Maybe it should be an independent type, that is not derive from AstNode */
class AstCtList : public AstNode {
public:
  AstCtList(std::list<AstObject*>* childs);
  AstCtList(AstObject* child1 = NULL);
  AstCtList(AstObject* child1, AstObject* child2, AstObject* child3 = NULL,
    AstObject* child4 = NULL, AstObject* child5 = NULL, AstObject* child6 = NULL);
  virtual ~AstCtList();

  void releaseOwnership();
  virtual void accept(AstVisitor& visitor) override;
  virtual void accept(AstConstVisitor& visitor) const override;
  AstCtList* Add(AstObject* child);
  AstCtList* Add(AstObject* child1, AstObject* child2, AstObject* child3 = NULL);
  /** The elements are guaranteed to be non-null */
  std::list<AstObject*>& childs() const { return *m_childs; }
  virtual const ObjType& objType() const;

private:
  /** We're the owner of the list and of the pointees. Pointers are garanteed
  to be non null*/
  std::list<AstObject*>*const m_childs;
  bool m_owner = true;
};
