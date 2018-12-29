#pragma once
// If you change this list of header files, you must also modify the
// analogous, redundant list in the Makefile
#include "objtype.h"
#include "object.h"
#include "access.h"
#include "generalvalue.h"
#include "storageduration.h"
#include "declutils.h"
#include "envnode.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Function.h"
#include <string>
#include <vector>
#include <iostream>
#include <map>
#include <memory>

#include "astforwards.h"
class AstConstVisitor;
class AstVisitor;
class ErrorHandler;

class AstNode {
public:
  virtual ~AstNode() = default;

  virtual void accept(AstVisitor& visitor) = 0;
  virtual void accept(AstConstVisitor& visitor) const = 0;

  /** See AstObject::m_accessFromAstParent. For non-AstObject types, access
  can only be eIgnore. It's a kludge that this method is a member of AstNode,
  the author hasn't found another way to implement
  SemanticAnalizer::visit(AstSeq& seq).*/
  virtual void setAccessFromAstParent(Access access) = 0;

  /** For AstObject's, identical to object().objType().isNoreturn(); for all
  others it always returns false, i.e. also for
  AstObjTypeSymbol(ObjTypeFunda::eNoReturn). It's a kludge that this method is
  a member of AstNode, see also setAccessFromAstParent. */
  virtual bool isObjTypeNoReturn() const { return false; }

  std::string toStr() const;

protected:
  AstNode() = default;

private:
  NEITHER_COPY_NOR_MOVEABLE(AstNode);
};

class AstObject : public AstNode {
public:
  // -- overrides for AstNode
  virtual bool isObjTypeNoReturn() const;

  // -- new virtual methods
  virtual Access accessFromAstParent() const = 0;
  virtual bool isCTConst() const { return false; }
  virtual Object& object() = 0;
  virtual const Object& object() const = 0;
};

/** AstObject directly being an Object, as opposed to refering to an Object. */
class AstObjInstance : public AstObject, public Object {
public:
  AstObjInstance();

  // -- overrides for AstNode
  void setAccessFromAstParent(Access access) override;

  // -- overrides for AstObject
  Access accessFromAstParent() const override;

  // -- new virtual methods
  Object& object() override { return *this; }
  const Object& object() const override { return *this; }

private:
  /** How the parent AST node accesses the Object associate to this AST
  node. Note that other AST nodes, e.g. AstSymbol, migh access the same object
  differently, see also Object::isModifiedOrRevealsAddr. */
  Access m_accessFromAstParent;
};

class AstObjDef : public AstObjInstance, public EnvNode {
public:
  // -- overrides for EnvNode
  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>& os) const;

  // -- misc
  bool isInitialized() const { return m_isInitialized; }
  void notifyInitialized() {
    assert(!m_isInitialized);
    m_isInitialized = true;
  }

protected:
  AstObjDef(std::string name);

private:
  /** Relative to program order, which equals AST post-order traversal, childs
  left to right. The property 'is initialized' is on AstObjDef instead of
  AstObject mainly for simplicity reasons. There currently only is a value of
  this property for symbols refering to a defined object. Anonymous objects
  cannot be refered to prior to their 'definition' anyway. */
  bool m_isInitialized = false;
};

class AstNop : public AstObjInstance {
public:
  // -- overrides for AstNode
  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;

  // -- overrides for Object
  virtual const ObjType& objType() const;
  virtual std::shared_ptr<const ObjType> objTypeAsSp() const;
  virtual StorageDuration storageDuration() const;
};

class AstBlock : public AstObjInstance, public EnvNode {
public:
  AstBlock(AstObject* body);

  // -- overrides for AstNode
  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;

  // -- overrides for Object
  virtual const ObjType& objType() const;
  virtual std::shared_ptr<const ObjType> objTypeAsSp() const;
  virtual StorageDuration storageDuration() const;

  // -- childs of this node
  AstObject& body() const { return *m_body.get(); }

private:
  // -- childs of this node
  /** Guaranteed to be non-null */
  const std::unique_ptr<AstObject> m_body;
};

class AstCast : public AstObjInstance {
public:
  AstCast(AstObjType* specifiedNewAstObjType, AstObject* arg);
  AstCast(AstObjType* specifiedNewAstObjType, AstCtList* args);
  AstCast(ObjTypeFunda::EType specifiedNewOjType, AstObject* child);

  // -- overrides for AstNode
  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;

  // -- overrides for Object
  virtual const ObjType& objType() const;
  virtual std::shared_ptr<const ObjType> objTypeAsSp() const;
  virtual StorageDuration storageDuration() const;

  // -- childs of this node
  AstObjType& specifiedNewAstObjType() const;
  AstCtList& args() const { return *m_args; }

private:
  // -- childs of this node
  /** Is guaranteed to be non-null */
  const std::unique_ptr<AstObjType> m_specifiedNewAstObjType;
  /** Is guaranteed to be non-null */
  const std::unique_ptr<AstCtList> m_args;
};

class AstFunDef : public AstObjDef {
public:
  AstFunDef(const std::string& name, std::vector<AstDataDef*>* args,
    AstObjType* ret, AstObject* body);

  // -- overrides for AstNode
  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;

  // -- overrides for Object
  virtual const ObjType& objType() const;
  virtual std::shared_ptr<const ObjType> objTypeAsSp() const;
  virtual StorageDuration storageDuration() const;

  // -- childs of this node
  // name() is in EnvNode
  virtual std::vector<std::unique_ptr<AstDataDef>> const& declaredArgs() const {
    return m_args;
  }
  virtual AstObjType& ret() const {
    assert(m_ret);
    return *m_ret;
  }
  virtual AstObject& body() const {
    assert(m_body);
    return *m_body;
  }

  // -- misc
  static std::vector<AstDataDef*>* createArgs(AstDataDef* arg1 = nullptr,
    AstDataDef* arg2 = nullptr, AstDataDef* arg3 = nullptr);
  void createAndSetObjType();

private:
  // -- associated object
  /** Redundant to m_args and m_ret */
  mutable std::shared_ptr<const ObjTypeFun> m_objType;

  // -- childs of this node
  // m_name is in EnvNode
  /** The members are guaranteed to be non-null */
  const std::vector<std::unique_ptr<AstDataDef>> m_args;
  /** Is garanteed to be non-null. */
  const std::unique_ptr<AstObjType> m_ret;
  /** Is garanteed to be non-null */
  const std::unique_ptr<AstObject> m_body;
};

/** Also used for data members of a class, which is not entirerly a nice
design since a member declaration is not yet an object, thus subtyping from
AstObject is wrong in this case. */
class AstDataDef : public AstObjDef {
public:
  AstDataDef(const std::string& name, AstObjType* declaredAstObjType,
    StorageDuration declaredStorageDuration, AstCtList* ctorArgs = nullptr);
  AstDataDef(const std::string& name, AstObjType* declaredAstObjType,
    StorageDuration declaredStorageDuration, AstObject* initObj);
  AstDataDef(const std::string& name, AstObjType* declaredAstObjType,
    AstObject* initObj = nullptr);
  AstDataDef(const std::string& name,
    ObjTypeFunda::EType declaredObjType = ObjTypeFunda::eInt,
    AstObject* initObj = nullptr);

  // -- overrides for AstNode
  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;

  // -- overrides for Object
  virtual const ObjType& objType() const;
  virtual std::shared_ptr<const ObjType> objTypeAsSp() const;
  // storageDuration is below within childs section

  // -- childs of this node
  // name() is in EnvNode
  AstObjType& declaredAstObjType() const;
  StorageDuration declaredStorageDuration() const;
  virtual StorageDuration storageDuration() const {
    return m_declaredStorageDuration;
  }
  AstCtList& ctorArgs() const { return *m_ctorArgs; }

  // -- misc
  bool doNotInit() const { return m_doNotInit; }

  static AstObject* const noInit;

private:
  friend class AstPrinter;

  static std::unique_ptr<AstCtList> mkCtorArgs(
    AstCtList* ctorArgs, StorageDuration storageDuration, bool& doNotInit);

  // -- childs of this node
  // m_name is in EnvNode
  /** Guaranteed to be non-null. */
  const std::unique_ptr<AstObjType> m_declaredAstObjType;
  /** Guaranteed to be not eYetUndefined */
  const StorageDuration m_declaredStorageDuration;
  /** Is garanteed to be non-null. Currently, zero args means to default
  initialize: semantic analizer will add an initilizer*/
  const std::unique_ptr<AstCtList> m_ctorArgs;

  // -- misc
  bool m_doNotInit;
};

/** Literal 'scalar', where scalar means 'not an aggregation /
composition'. Obviously a well defined term is needed, and then the class
should be renamed. */
class AstNumber : public AstObjInstance {
public:
  AstNumber(GeneralValue value, AstObjType* astObjType = nullptr);
  AstNumber(GeneralValue value, ObjTypeFunda::EType eType);

  // -- overrides for AstNode
  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;

  // -- overrides for Object
  virtual bool isCTConst() const { return true; }
  virtual const ObjType& objType() const;
  virtual std::shared_ptr<const ObjType> objTypeAsSp() const;
  virtual StorageDuration storageDuration() const;

  // -- childs of this node
  GeneralValue value() const { return m_value; }
  AstObjType& declaredAstObjType() const;

private:
  // -- childs of this node
  const GeneralValue m_value;
  /** Guaranteed to be non-null */
  const std::unique_ptr<AstObjType> m_declaredAstObjType;
};

/** Here symbol as an synonym to identifier */
class AstSymbol : public AstObject {
public:
  AstSymbol(const std::string& name)
    : m_referencedAstObj{}
    , m_accessFromAstParent{Access::eYetUndefined}
    , m_name(name) {}

  // -- overrides for AstNode
  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;
  virtual void setAccessFromAstParent(Access access);

  // -- overrides for AstObject
  virtual Access accessFromAstParent() const;
  virtual Object& object();
  virtual const Object& object() const;

  // -- overrides for Object
  virtual const ObjType& objType() const;
  virtual std::shared_ptr<const ObjType> objTypeAsSp() const;
  virtual StorageDuration storageDuration() const;

  // -- childs of this node
  const std::string& name() const { return m_name; }

  // -- misc
  void setreferencedAstObjAndPropagateAccess(AstObjDef&);
  bool isInitialized() const;

private:
  friend class TestingAstSymbol;

  // -- associated object
  AstObjDef* m_referencedAstObj;
  /** See AstObjInstance::m_accessFromAstParent */
  Access m_accessFromAstParent;

  // -- childs of this node
  const std::string m_name;
};

class AstFunCall : public AstObjInstance {
public:
  AstFunCall(AstObject* address, AstCtList* args = nullptr);

  // -- overrides for AstNode
  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;

  // -- overrides for Object
  virtual const ObjType& objType() const;
  virtual std::shared_ptr<const ObjType> objTypeAsSp() const;
  virtual StorageDuration storageDuration() const;

  // -- childs of this node
  virtual AstObject& address() const { return *m_address; }
  AstCtList& args() const { return *m_args; }

private:
  // -- childs of this node
  /** Is garanteed to be non-null */
  const std::unique_ptr<AstObject> m_address;
  /** Is garanteed to be non-null */
  const std::unique_ptr<AstCtList> m_args;
};

/** KLUDGE: In case of operators returning 'by reference' (eAssign and
eDeref), the inheritance from AstObjInstance is not correct; in those cases
we should derive from AstObject. */
class AstOperator : public AstObject, private Object {
public:
  enum EOperation {
    eVoidAssign = '=',
    eAdd = '+',
    eSub = '-',
    eMul = '*', // '*' is ambigous. can also mean eDeref
    eDiv = '/',
    eNot = '!',
    eAddrOf = '&',
    eAnd = 128,
    eOr,
    eEqualTo,
    eAssign,
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
  AstOperator(
    char op, AstObject* operand1 = nullptr, AstObject* operand2 = nullptr);
  AstOperator(const std::string& op, AstObject* operand1 = nullptr,
    AstObject* operand2 = nullptr);
  AstOperator(EOperation op, AstObject* operand1 = nullptr,
    AstObject* operand2 = nullptr);

  // -- overrides for AstNode
  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;
  virtual void setAccessFromAstParent(Access access);

  // -- overrides for AstObject
  virtual Access accessFromAstParent() const;
  virtual Object& object();
  virtual const Object& object() const;

  // -- childs of this node
  EOperation op() const { return m_op; }
  AstCtList& args() const { return *m_args; }

  // -- misc
  EClass class_() const;
  bool isBinaryLogicalShortCircuit() const;
  /** See m_referencedAstObj */
  void setReferencedObjAndPropagateAccess(std::unique_ptr<Object>);
  void setReferencedObjAndPropagateAccess(Object&);
  void setObjType(std::shared_ptr<const ObjType>);
  void setStorageDuration(StorageDuration);
  static EClass classOf(AstOperator::EOperation op);
  /** In case of ambiguity, chooses the binary operator */
  static EOperation toEOperationPreferingBinary(const std::string& op);
  /** In case of ambiguity asserts */
  static EOperation toEOperation(const std::string& op);

private:
  // -- overrides for Object
  // !see class comment! Private so they are not called accidentaly, callers
  // must go via object()
  virtual const ObjType& objType() const;
  virtual std::shared_ptr<const ObjType> objTypeAsSp() const;
  virtual StorageDuration storageDuration() const;

  friend std::basic_ostream<char>& operator<<(
    std::basic_ostream<char>&, AstOperator::EOperation);

  // -- associated object
  /** For operands which return by reference: Currently only
  AstOperator::eAssign */
  Object* m_referencedObj;
  /** See AstObjInstance::m_accessFromAstParent */
  Access m_accessFromAstParent;
  /** For the case we must be the owner of m_referencedObj's pointee */
  std::unique_ptr<Object> m_dummy;
  /** In case m_referencedObj is non-nullptr, redundant to
  m_referencedObj->objTypeAsSp*/
  std::shared_ptr<const ObjType> m_objType;
  /** In case m_referencedObj is non-nullptr, redundant to
  m_referencedObj->m_storageDuration */
  StorageDuration m_storageDuration;

  // -- childs of this node
  const EOperation m_op;
  /** Is garanteed to be non-null */
  const std::unique_ptr<AstCtList> m_args;

  // -- misc
  static const std::map<const std::string, const EOperation> m_opMap;
  static const std::map<const EOperation, const std::string> m_opReverseMap;
};

std::basic_ostream<char>& operator<<(
  std::basic_ostream<char>& os, AstOperator::EOperation op);

/** Has two responsibilities: 1) Be an ordered sequence of AstNode s 2) cast
the last AstNode to an AstObject and report an error if that is not
possible. */
class AstSeq : public AstObject {
public:
  AstSeq(std::vector<AstNode*>* operands);
  AstSeq(AstNode* op1 = nullptr, AstNode* op2 = nullptr, AstNode* op3 = nullptr,
    AstNode* op4 = nullptr, AstNode* op5 = nullptr);

  // -- overrides for AstNode
  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;
  virtual void setAccessFromAstParent(Access access);

  // -- overrides for AstObject
  virtual Access accessFromAstParent() const;
  virtual Object& object();
  virtual const Object& object() const;

  // -- childs of this node
  const std::vector<std::unique_ptr<AstNode>>& operands() const {
    return m_operands;
  }

  // -- misc
  bool lastOperandIsAnObject() const;

private:
  AstObject& lastOperand() const;

  // -- associated object
  /** See AstObjInstance::m_accessFromAstParent */
  Access m_accessFromAstParent;

  // -- childs of this node
  /** Pointers are garanteed to be non null. Garanteed to have at least one
  element. */
  const std::vector<std::unique_ptr<AstNode>> m_operands;
};

/* If flow control expression */
class AstIf : public AstObjInstance {
public:
  AstIf(AstObject* cond, AstObject* action, AstObject* elseAction = nullptr);

  // -- overrides for AstNode
  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;

  // -- overrides for Object
  virtual const ObjType& objType() const;
  virtual std::shared_ptr<const ObjType> objTypeAsSp() const;
  virtual StorageDuration storageDuration() const;

  // -- childs of this node
  AstObject& condition() const { return *m_condition; }
  AstObject& action() const { return *m_action; }
  AstObject* elseAction() const { return m_elseAction.get(); }

  // -- misc
  void setObjectType(std::shared_ptr<const ObjType>);

private:
  // -- associated object
  std::shared_ptr<const ObjType> m_objType;

  // -- childs of this node
  /** Is garanteed to be non-null */
  const std::unique_ptr<AstObject> m_condition;
  /** Is garanteed to be non-null */
  const std::unique_ptr<AstObject> m_action;
  /** Is NOT garanteed to be non-null */
  const std::unique_ptr<AstObject> m_elseAction;
};

class AstLoop : public AstObjInstance {
public:
  AstLoop(AstObject* cond, AstObject* body);

  // -- overrides for AstNode
  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;

  // -- overrides for Object
  virtual const ObjType& objType() const;
  virtual std::shared_ptr<const ObjType> objTypeAsSp() const;
  virtual StorageDuration storageDuration() const;

  // -- childs of this node
  AstObject& condition() const { return *m_condition; }
  AstObject& body() const { return *m_body; }

private:
  // -- childs of this node
  /** Is garanteed to be non-null */
  const std::unique_ptr<AstObject> m_condition;
  /** Is garanteed to be non-null */
  const std::unique_ptr<AstObject> m_body;
};

class AstReturn : public AstObjInstance {
public:
  AstReturn(AstObject* retVal = nullptr);
  AstReturn(AstCtList* ctorArgs);

  // -- overrides for AstNode
  virtual void accept(AstVisitor& visitor);
  virtual void accept(AstConstVisitor& visitor) const;

  // -- overrides for Object
  virtual const ObjType& objType() const;
  virtual std::shared_ptr<const ObjType> objTypeAsSp() const;
  virtual StorageDuration storageDuration() const;

  // -- childs of this node
  AstCtList& ctorArgs() const { return *m_ctorArgs; }

private:
  // -- childs of this node
  /** Is guaranteed to be non-null. Arguments to construct return value */
  const std::unique_ptr<AstCtList> m_ctorArgs;
};

/** See also ObjType */
class AstObjType : public AstNode {
public:
  virtual void setAccessFromAstParent(Access access) override {
    assert(access == Access::eIgnoreValueAndAddr);
  }

  virtual void printValueTo(std::ostream& os, GeneralValue value) const = 0;
  virtual bool isValueInRange(GeneralValue value) const = 0;
  /** The objType of the created AstObject is immutable, since the AstObject
  has the semantics of a temporary. Asserts in case of there is no default
  AstObject. The returned AstObject is set up as if it had passed all passes
  prior to SemanticAnalizer, but not yet passed SemanticAnalizer's pass. */
  virtual AstObject* createDefaultAstObjectForSemanticAnalizer() = 0;

  /** Can only be called after this AstObjType has been visited by the
  SemanticAnalizer. */
  virtual const ObjType& objType() const = 0;
  virtual std::shared_ptr<const ObjType> objTypeAsSp() const = 0;
  virtual void createAndSetObjType() = 0;

  // -- decorations for IrGen
  virtual llvm::Value* createLlvmValueFrom(GeneralValue value) const = 0;
};

class AstObjTypeSymbol : public AstObjType {
public:
  AstObjTypeSymbol(const std::string name);
  AstObjTypeSymbol(ObjTypeFunda::EType fundaType);

  // -- overrides for AstNode
  void accept(AstVisitor& visitor) override;
  void accept(AstConstVisitor& visitor) const override;

  // -- overrides for AstObjType
  void printValueTo(std::ostream& os, GeneralValue value) const override;
  bool isValueInRange(GeneralValue value) const override;
  AstObject* createDefaultAstObjectForSemanticAnalizer() override;

  const ObjType& objType() const override;
  std::shared_ptr<const ObjType> objTypeAsSp() const override;
  void createAndSetObjType() override;

  // -- childs of this node
  const std::string name() const { return m_name; }

private:
  friend class TestingAstObjTypeSymbol;

  static std::string toName(ObjTypeFunda::EType fundaType);
  static ObjTypeFunda::EType toType(const std::string& name);
  static void initMap();

  // -- to implement overrides
  std::shared_ptr<const ObjType> m_objType;

  // -- childs of this node
  const std::string m_name;

  // -- misc
  static std::array<std::string, ObjTypeFunda::eTypeCnt> m_typeToName;
  static bool m_isMapInitialzied;

  // decorations for IrGen
public:
  virtual llvm::Value* createLlvmValueFrom(GeneralValue value) const override;
};

class AstObjTypeQuali : public AstObjType {
public:
  AstObjTypeQuali(ObjType::Qualifiers qualifiers, AstObjType* targetType);

  // -- overrides for AstNode
  void accept(AstVisitor& visitor) override;
  void accept(AstConstVisitor& visitor) const override;

  // -- overrides for AstObjType
  void printValueTo(std::ostream& os, GeneralValue value) const override;
  bool isValueInRange(GeneralValue value) const override;
  AstObject* createDefaultAstObjectForSemanticAnalizer() override;

  const ObjType& objType() const override;
  std::shared_ptr<const ObjType> objTypeAsSp() const override;
  void createAndSetObjType() override;

  // -- childs of this node
  ObjType::Qualifiers qualifiers() const { return m_qualifiers; }
  AstObjType& targetType() const { return *m_targetType; }

private:
  // -- to implement overrides
  std::shared_ptr<const ObjType> m_objType;

  // -- childs of this node
  const ObjType::Qualifiers m_qualifiers;
  /** Guaranteed to be non-null */
  const std::unique_ptr<AstObjType> m_targetType;

  // decorations for IrGen
public:
  virtual llvm::Value* createLlvmValueFrom(GeneralValue value) const override;
};

class AstObjTypePtr : public AstObjType {
public:
  AstObjTypePtr(AstObjType* pointee);

  // -- overrides for AstNode
  void accept(AstVisitor& visitor) override;
  void accept(AstConstVisitor& visitor) const override;

  // -- overrides for AstObjType
  void printValueTo(std::ostream& os, GeneralValue value) const override;
  bool isValueInRange(GeneralValue value) const override;
  AstObject* createDefaultAstObjectForSemanticAnalizer() override;

  const ObjTypePtr& objType() const override;
  std::shared_ptr<const ObjType> objTypeAsSp() const override;
  void createAndSetObjType() override;

  // -- childs of this node
  AstObjType& pointee() const { return *m_pointee; }

private:
  // -- to implement overrides
  std::shared_ptr<const ObjTypePtr> m_objType;

  // -- childs of this node
  /** Guaranteed to be non-null */
  const std::unique_ptr<AstObjType> m_pointee;

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

  // -- overrides for AstNode
  void accept(AstVisitor& visitor) override;
  void accept(AstConstVisitor& visitor) const override;

  // -- overrides for AstObjType
  void printValueTo(std::ostream& os, GeneralValue value) const override;
  bool isValueInRange(GeneralValue value) const override;
  AstObject* createDefaultAstObjectForSemanticAnalizer() override;

  const ObjTypeClass& objType() const override;
  std::shared_ptr<const ObjType> objTypeAsSp() const override;
  void createAndSetObjType() override;

  // -- childs of this node
  const std::string& name() const { return m_name; }
  const std::vector<std::unique_ptr<AstDataDef>>& dataMembers() const {
    return m_dataMembers;
  }

private:
  // -- to implement overrides
  std::shared_ptr<const ObjTypeClass> m_objType;

  // -- childs of this node
  const std::string m_name;
  /** Pointers are garanteed to be non null.*/
  const std::vector<std::unique_ptr<AstDataDef>> m_dataMembers;

  // decorations for IrGen
public:
  virtual llvm::Value* createLlvmValueFrom(GeneralValue value) const override;
};

/** Maybe it should be an independent type, that is not derive from AstNode */
class AstCtList : public AstNode {
public:
  AstCtList(std::vector<AstObject*>* childs);
  AstCtList(AstObject* child1 = nullptr);
  AstCtList(AstObject* child1, AstObject* child2, AstObject* child3 = nullptr,
    AstObject* child4 = nullptr, AstObject* child5 = nullptr,
    AstObject* child6 = nullptr);
  virtual ~AstCtList();

  // -- overrides for AstNode
  virtual void accept(AstVisitor& visitor) override;
  virtual void accept(AstConstVisitor& visitor) const override;
  virtual void setAccessFromAstParent(Access access) override {
    assert(access == Access::eIgnoreValueAndAddr);
  }

  // -- childs of this node
  /** The elements are guaranteed to be non-null */
  std::vector<AstObject*>& childs() const { return *m_childs; }

  // -- misc
  void releaseOwnership();
  AstCtList* Add(AstObject* child);
  AstCtList* Add(
    AstObject* child1, AstObject* child2, AstObject* child3 = nullptr);

private:
  // -- childs of this node
  /** We're the owner of the pointees. Pointers are garanteed to be non null*/
  std::vector<AstObject*>* const m_childs;

  // -- misc
  bool m_owner = true;
};
