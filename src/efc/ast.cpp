#include "ast.h"
#include "astvisitor.h"
#include "astprinter.h"
#include "errorhandler.h"
#include "object.h"
#include <cassert>
#include <stdexcept>
using namespace std;

string AstNode::toStr() const {
  return AstPrinter::toStr(*this);
}

AstObject::AstObject(Access access, std::shared_ptr<Object> object) :
  m_access(access),
  m_object(move(object)) {
}

AstObject::AstObject(Access access) :
  AstObject(access, nullptr) {
}

AstObject::AstObject(std::shared_ptr<Object> object) :
  AstObject(eRead, move(object)) {
}

AstObject::~AstObject() {
}

void AstObject::setAccess(Access access) {
  m_access = access;
}

const ObjType& AstObject::objType() const {
  assert(m_object);
  return m_object->objType();
}

shared_ptr<const ObjType> AstObject::objTypeAsSp() const {
  assert(m_object);
  return m_object->objTypeAsSp();
}

bool AstObject::objectIsModifiedOrRevealsAddr() const {
  assert(m_object);
  return m_object->isModifiedOrRevealsAddr();
}

void AstObject::setObject(shared_ptr<Object> object) {
  assert(object);
  assert(!m_object.get()); // it makes no sense to set it twice
  m_object = move(object);
}

AstNop::AstNop() :
  AstObject{
    make_shared<Object>(
      make_shared<ObjTypeFunda>(ObjTypeFunda::eVoid),
      StorageDuration::eLocal)} {
}

AstBlock::AstBlock(AstObject* body) :
  m_body(body) {
}

AstBlock::~AstBlock() {
}

AstCast::AstCast(ObjType* objType, AstObject* child) :
  AstObject{
    make_shared<Object>(
      shared_ptr<ObjType>{
        objType ? objType : new ObjTypeFunda(ObjTypeFunda::eInt)},
      StorageDuration::eLocal)},
  m_child(child ? child : new AstNumber(0)) {
  assert(m_child);

  // only currently for simplicity; the cast, as most expression, creates a
  // temporary object, and temporary objects are immutable
  assert(!(objType->qualifiers() & ObjType::eMutable));
}

AstCast::AstCast(ObjTypeFunda::EType objType, AstObject* child) :
  AstCast{new ObjTypeFunda(objType), child} {
}

AstCast::~AstCast() {
  delete m_child;
}

AstFunDef::AstFunDef(const string& name,
  shared_ptr<Object> object,
  std::list<AstDataDef*>* args,
  shared_ptr<const ObjType> ret,
  AstObject* body) :
  AstObject{move(object)},
  m_name(name),
  m_args(args),
  m_ret(move(ret)),
  m_body(body) {
  assert(m_args);
  assert(m_ret);
  assert(m_body);
}

AstFunDef::~AstFunDef() {
  for (list<AstDataDef*>::const_iterator i=m_args->begin();
       i!=m_args->end(); ++i) {
    delete *i;
  }
  delete m_args;
  delete m_body;
}

const ObjType& AstFunDef::retObjType() const {
  assert(m_ret);
  return *m_ret;
}

list<AstDataDef*>* AstFunDef::createArgs(AstDataDef* arg1,
  AstDataDef* arg2, AstDataDef* arg3) {
  list<AstDataDef*>* args = new list<AstDataDef*>;
  if (arg1) { args->push_back(arg1); }
  if (arg2) { args->push_back(arg2); }
  if (arg3) { args->push_back(arg3); }
  return args;
}

AstObject* const AstDataDef::noInit = reinterpret_cast<AstObject*>(1);

AstDataDef::AstDataDef(const std::string& name,
  shared_ptr<const ObjType> declaredObjType,
  StorageDuration declaredStorageDuration,  AstCtList* ctorArgs) :
  m_name(name),
  m_declaredObjType(declaredObjType ?
    move(declaredObjType) :
    make_shared<ObjTypeFunda>(ObjTypeFunda::eInt)),
  m_declaredStorageDuration(declaredStorageDuration),
  m_ctorArgs(mkCtorArgs(ctorArgs, *m_declaredObjType,
      m_ctorArgsWereImplicitelyDefined, m_doNotInit)) {
  assert(m_ctorArgs);
}

AstDataDef::AstDataDef(const std::string& name,
  shared_ptr<const ObjType> declaredObjType,
  StorageDuration declaredStorageDuration, AstObject* initObj) :
  AstDataDef(name, move(declaredObjType), declaredStorageDuration,
    initObj ? new AstCtList(initObj) : nullptr) {
}

AstDataDef::AstDataDef(const std::string& name,
  shared_ptr<const ObjType> declaredObjType, AstObject* initObj) :
  AstDataDef(name, move(declaredObjType), StorageDuration::eLocal, initObj) {
}

AstDataDef::AstDataDef(const std::string& name, ObjTypeFunda::EType declaredObjType,
  AstObject* initObj) :
  AstDataDef(name, make_shared<ObjTypeFunda>(declaredObjType), initObj) {
}

AstDataDef::~AstDataDef() {
  delete m_ctorArgs;
}

AstCtList* AstDataDef::mkCtorArgs(AstCtList* ctorArgs,
  const ObjType& declaredObjType, bool& ctorArgsWereImplicitelyDefined,
  bool& doNotInit) {
  ctorArgsWereImplicitelyDefined = false; // assumptions
  doNotInit = false;
  if (not ctorArgs) {
    ctorArgs = new AstCtList();
  }
  if (not ctorArgs->childs().empty()) {
    if ( ctorArgs->childs().front()==noInit ) {
      doNotInit = true;
      assert(ctorArgs->childs().size()==1);
      ctorArgs->childs().clear();
    }
    return ctorArgs;
  }
  // Currrently zero arguments are not supported. When none args are given, a
  // default arg is used.
  ctorArgs->childs().push_back(declaredObjType.createDefaultAstObject());
  ctorArgsWereImplicitelyDefined = true;
  return ctorArgs;
}

const ObjType& AstDataDef::declaredObjType() const {
  return *m_declaredObjType.get();
}

shared_ptr<const ObjType>& AstDataDef::declaredObjTypeAsSp() {
  return m_declaredObjType;
}

StorageDuration AstDataDef::declaredStorageDuration() const {
  return m_declaredStorageDuration;
}

shared_ptr<Object>& AstDataDef::createAndSetObjectUsingDeclaredObjType() {
  setObject(make_shared<Object>(m_declaredObjType, m_declaredStorageDuration));
  return m_object;
}

AstNumber::AstNumber(GeneralValue value, std::shared_ptr<const ObjTypeFunda> objType) :
  AstObject{
    make_shared<Object>(
      objType ?
      move(objType) :
      make_shared<const ObjTypeFunda>(ObjTypeFunda::eInt),
      StorageDuration::eLocal)},
  m_value(value) {
}

AstNumber::AstNumber(GeneralValue value, ObjTypeFunda::EType eType) :
  AstNumber(value, make_shared<ObjTypeFunda>(eType)) {
}

const ObjTypeFunda& AstNumber::objType() const {
  return static_cast<const ObjTypeFunda&>(AstObject::objType());
}

const map<const string, const AstOperator::EOperation> AstOperator::m_opMap{
  {"and", eAnd},
  {"&&", eAnd},
  {"or", eOr},
  {"||", eOr},
  {"==", eEqualTo},
  {".=", eDotAssign},
  {"not", eNot}};

// in case of ambiguity, prefer one letters over symbols. Also note that
// single char operators are prefered over multi chars; the single chars are
// not even in the map.
const map<const AstOperator::EOperation, const std::string> AstOperator::m_opReverseMap{
  {eAnd, "and"},
  {eOr, "or"},
  {eEqualTo, "=="},
  {eDotAssign, ".="},
  {eDeref, "*"}};

AstOperator::AstOperator(char op, AstCtList* args) :
  AstOperator(static_cast<EOperation>(op), args) {};

AstOperator::AstOperator(const string& op, AstCtList* args) :
  AstOperator(toEOperation(op), args) {};

AstOperator::AstOperator(AstOperator::EOperation op, AstCtList* args) :
  m_op(op),
  m_args(args ? args : new AstCtList) {
  if ( '-' == m_op || '+' == m_op ) {
    const auto argCnt = args->childs().size();
    assert( argCnt==1 || argCnt==2 );
  } else {
    const size_t required_arity =
      (op==eNot || op==eAddrOf || op==eDeref ) ? 1 : 2;
    assert( args->childs().size() == required_arity );
  }
}

AstOperator::AstOperator(char op, AstObject* operand1, AstObject* operand2) :
  AstOperator(static_cast<EOperation>(op), operand1, operand2) {};

AstOperator::AstOperator(const string& op, AstObject* operand1, AstObject* operand2) :
  AstOperator(toEOperation(op), operand1, operand2) {};

AstOperator::AstOperator(AstOperator::EOperation op, AstObject* operand1, AstObject* operand2) :
  AstOperator(op, new AstCtList(operand1, operand2)) {};

AstOperator::~AstOperator() {
  delete m_args;
}

AstOperator::EClass AstOperator::class_() const {
  return classOf(m_op);
}

bool AstOperator::isBinaryLogicalShortCircuit() const {
  return m_op==eAnd || m_op==eOr;
}

AstOperator::EClass AstOperator::classOf(AstOperator::EOperation op) {
  switch (op) {
  case eAssign:
  case eDotAssign:
    return eAssignment;

  case eAdd:
  case eSub:
  case eMul:
  case eDiv:
    return eArithmetic;

  case eNot:
  case eAnd:
  case eOr:
    return eLogical;

  case eEqualTo:
    return eComparison;

  case eAddrOf:
  case eDeref:
    return eMemberAccess;
  }
  assert(false);
  return eOther;
}

AstOperator::EOperation AstOperator::toEOperationPreferingBinary(const string& op) {
  if ( op=="*" ) {
    return eMul;
  } else {
    return toEOperation(op);
  }
}

AstOperator::EOperation AstOperator::toEOperation(const string& op) {
  if (op.size()==1) {
    assert(op[0]!='*'); // '*' is ambigous: either eMul or eDeref
    assert(op[0]!=';'); // ';' is not really an operator, use AstSeq
    return static_cast<EOperation>(op[0]);
  } else {
    auto i = m_opMap.find(op);
    assert( i != m_opMap.end() );
    return i->second;
  }
}

basic_ostream<char>& operator<<(basic_ostream<char>& os,
  AstOperator::EOperation op) {
  if (static_cast<int>(op)<128) {
    return os << static_cast<char>(op);
  } else {
    auto i = AstOperator::m_opReverseMap.find(op);
    assert( i != AstOperator::m_opReverseMap.end() );
    return os << i->second;
  }
}

AstSeq::AstSeq(std::vector<AstNode*>* operands) :
  m_operands(operands) {
  assert(m_operands);
  assert(!m_operands->empty());
}

AstSeq::AstSeq(AstNode* op1, AstNode* op2, AstNode* op3, AstNode* op4,
  AstNode* op5) :
  m_operands(std::make_unique<std::vector<AstNode*>>()) {
  if ( op1 ) {
    m_operands->push_back(op1);
  }
  if ( op2 ) {
    m_operands->push_back(op2);
  }
  if ( op3 ) {
    m_operands->push_back(op3);
  }
  if ( op4 ) {
    m_operands->push_back(op4);
  }
  if ( op5 ) {
    m_operands->push_back(op5);
  }
  assert(!m_operands->empty());
}

AstObject& AstSeq::lastOperand(ErrorHandler& errorHandler) const {
  auto obj = dynamic_cast<AstObject*>((*m_operands).back());
  if ( !obj ) {
    Error::throwError(errorHandler, Error::eObjectExpected);
  }
  return *obj;
}

AstIf::AstIf(AstObject* cond, AstObject* action, AstObject* elseAction) :
  m_condition(cond),
  m_action(action),
  m_elseAction(elseAction) {
  assert(m_condition);
  assert(m_action);
}

AstIf::~AstIf() {
  delete m_condition;
  delete m_action;
  delete m_elseAction;
}

AstLoop::AstLoop(AstObject* cond, AstObject* body) :
  AstObject{
    make_shared<Object>(
      make_shared<ObjTypeFunda>(ObjTypeFunda::eVoid),
      StorageDuration::eLocal)},
  m_condition(cond),
  m_body(body) {
  assert(m_condition);
  assert(m_body);
}

AstLoop::~AstLoop() {
  delete m_condition;
  delete m_body;
}

AstReturn::AstReturn(AstObject* retVal) :
  AstObject{
    make_shared<Object>(
      make_shared<ObjTypeFunda>(ObjTypeFunda::eNoreturn),
      StorageDuration::eLocal)},
  m_retVal(retVal) {
  assert(m_retVal);
}

AstReturn::~AstReturn() {
}

AstObject& AstReturn::retVal() const {
  assert(m_retVal);
  return *m_retVal.get();
}

AstFunCall::AstFunCall(AstObject* address, AstCtList* args) :
  m_address(address ? address : new AstSymbol("")),
  m_args(args ? args : new AstCtList()) {
  assert(m_address);
  assert(m_args);
}

AstFunCall::~AstFunCall() {
  delete m_args;
  delete m_address;
}

void AstFunCall::createAndSetObjectUsingRetObjType() {
  const auto& objType = m_address->objType();
  assert(typeid(objType)==typeid(ObjTypeFun));
  const auto& objTypeFun = static_cast<const ObjTypeFun&>(objType);
  setObject(make_shared<Object>(
      objTypeFun.ret().unqualifiedObjType(),
      StorageDuration::eLocal));
}

AstClassDef::AstClassDef(const std::string& name, std::vector<AstNode*>* dataMembers) :
  m_name(name),
  m_dataMembers(dataMembers) {
  assert(m_dataMembers);
  for (const auto& dataMember: *m_dataMembers) {
    assert(dataMember);
  }
}

AstClassDef::AstClassDef(const std::string& name, AstNode* m1, AstNode* m2,
  AstNode* m3) :
  m_name(name),
  m_dataMembers(make_unique<std::vector<AstNode*>>()) {
  if (m1) {
    m_dataMembers->push_back(m1);  
  }
  if (m2) {
    m_dataMembers->push_back(m2);  
  }
  if (m3) {
    m_dataMembers->push_back(m3);  
  }
}

/** The list's elements must be non-null */
AstCtList::AstCtList(list<AstObject*>* childs) :
  m_childs(childs ? childs : new list<AstObject*>() ) {
  assert(m_childs);
  for (list<AstObject*>::iterator it = m_childs->begin();
       it != m_childs->end();
       ++it) {
    assert(*it);
  }
}

/** When child is NULL it is ignored */
AstCtList::AstCtList(AstObject* child) :
  m_childs(new list<AstObject*>()) {
  assert(m_childs);
  if (child) { m_childs->push_back(child); }
}

/** NULL childs are ignored.*/
AstCtList::AstCtList(AstObject* child1, AstObject* child2, AstObject* child3,
  AstObject* child4, AstObject* child5, AstObject* child6) :
  m_childs(new list<AstObject*>()) {
  assert(m_childs);
  if (child1) { m_childs->push_back(child1); }
  if (child2) { m_childs->push_back(child2); }
  if (child3) { m_childs->push_back(child3); }
  if (child4) { m_childs->push_back(child4); }
  if (child5) { m_childs->push_back(child5); }
  if (child6) { m_childs->push_back(child6); }
}

AstCtList::~AstCtList() {
  if ( m_owner ) {
    for (list<AstObject*>::iterator i=m_childs->begin(); i!=m_childs->end(); ++i) {
      delete (*i);
    }
  }
  delete m_childs;
}

void AstCtList::releaseOwnership() {
  m_owner = false;
}

/** When child is NULL it is ignored */
AstCtList* AstCtList::Add(AstObject* child) {
  if (child) m_childs->push_back(child);
  return this;
}

/** NULL childs are ignored. */
AstCtList* AstCtList::Add(AstObject* child1, AstObject* child2, AstObject* child3) {
  Add(child1);
  Add(child2);
  Add(child3);
  return this;
}

const ObjType& AstCtList::objType() const {
  // AstCtList is never the operand of an expression, thus it doesn't has an
  // ObjType
  assert(false);
}


void AstNop::accept(AstConstVisitor& visitor) const { visitor.visit(*this); }
void AstBlock::accept(AstConstVisitor& visitor) const { visitor.visit(*this); }
void AstCast::accept(AstConstVisitor& visitor) const { visitor.visit(*this); }
void AstFunDef::accept(AstConstVisitor& visitor) const { visitor.visit(*this); }
void AstDataDef::accept(AstConstVisitor& visitor) const { visitor.visit(*this); }
void AstNumber::accept(AstConstVisitor& visitor) const { visitor.visit(*this); }
void AstSymbol::accept(AstConstVisitor& visitor) const { visitor.visit(*this); }
void AstFunCall::accept(AstConstVisitor& visitor) const { visitor.visit(*this); }
void AstOperator::accept(AstConstVisitor& visitor) const { visitor.visit(*this); }
void AstSeq::accept(AstConstVisitor& visitor) const { visitor.visit(*this); }
void AstIf::accept(AstConstVisitor& visitor) const { visitor.visit(*this); }
void AstClassDef::accept(AstConstVisitor& visitor) const { visitor.visit(*this); }
void AstLoop::accept(AstConstVisitor& visitor) const { visitor.visit(*this); }
void AstReturn::accept(AstConstVisitor& visitor) const { visitor.visit(*this); }
void AstCtList::accept(AstConstVisitor& visitor) const { visitor.visit(*this); }

void AstNop::accept(AstVisitor& visitor) { visitor.visit(*this); }
void AstBlock::accept(AstVisitor& visitor) { visitor.visit(*this); }
void AstCast::accept(AstVisitor& visitor) { visitor.visit(*this); }
void AstFunDef::accept(AstVisitor& visitor) { visitor.visit(*this); }
void AstDataDef::accept(AstVisitor& visitor) { visitor.visit(*this); }
void AstNumber::accept(AstVisitor& visitor) { visitor.visit(*this); }
void AstSymbol::accept(AstVisitor& visitor) { visitor.visit(*this); }
void AstFunCall::accept(AstVisitor& visitor) { visitor.visit(*this); }
void AstOperator::accept(AstVisitor& visitor) { visitor.visit(*this); }
void AstSeq::accept(AstVisitor& visitor) { visitor.visit(*this); }
void AstIf::accept(AstVisitor& visitor) { visitor.visit(*this); }
void AstClassDef::accept(AstVisitor& visitor) { visitor.visit(*this); }
void AstLoop::accept(AstVisitor& visitor) { visitor.visit(*this); }
void AstReturn::accept(AstVisitor& visitor) { visitor.visit(*this); }
void AstCtList::accept(AstVisitor& visitor) { visitor.visit(*this); }
