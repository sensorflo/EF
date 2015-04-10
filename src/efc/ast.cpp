#include "ast.h"
#include "astvisitor.h"
#include "astprinter.h"
#include "errorhandler.h"
#include "env.h"
#include <cassert>
#include <stdexcept>
using namespace std;

void AstNode::setAccess(Access access) {
  m_access = access;
}

string AstNode::toStr() const {
  return AstPrinter::toStr(*this);
}

/** Default implementation for AstValue's where there is a one-to-one
relationship between AstValue and object. */
bool AstValue::objectWasModifiedOrRevealedAddr() const {
  return m_access==eWrite || m_access==eTakeAddress;
}

const ObjType& AstNop::objType() const {
  static const ObjTypeFunda voidObjType{ObjTypeFunda::eVoid};
  return voidObjType;
}

void AstNop::setIrValue(llvm::Value* value) {
  assert(value);
  assert(!m_irValue); // it doesnt make sense to set it twice
  m_irValue = value;
}

AstBlock::AstBlock(AstValue* body) :
  m_body(body),
  m_irValue(nullptr) {
  assert(m_body);
}

const ObjType& AstBlock::objType() const {
  assert(m_objType.get());
  return *m_objType.get();
}

void AstBlock::setObjType(std::unique_ptr<ObjType> objType) {
  assert(objType);
  assert(!m_objType); // it doesnt make sense to set it twice
  m_objType = move(objType);
}

void AstBlock::setIrValue(llvm::Value* value) {
  assert(value);
  assert(!m_irValue); // it doesnt make sense to set it twice
  m_irValue = value;
}

AstCast::AstCast(ObjType* objType, AstValue* child) :
  m_objType(objType ? objType : new ObjTypeFunda(ObjTypeFunda::eInt)),
  m_child(child ? child : new AstNumber(0)),
  m_irValue(NULL) {
  assert(m_child);
  assert(m_objType);

  // only currently for simplicity; the cast, as most expression, creates a
  // temporary object, and temporary objects are immutable
  assert(!(m_objType->qualifiers() & ObjType::eMutable));
}

AstCast::AstCast(ObjTypeFunda::EType objType, AstValue* child) :
  m_objType(new ObjTypeFunda(objType)),
  m_child(child ? child : new AstNumber(0)),
  m_irValue(NULL) {
  assert(m_objType);
  assert(m_child);
}

AstCast::~AstCast() {
  delete m_objType;
  delete m_child;
}

void AstCast::setIrValue(llvm::Value* value) {
  assert(value);
  assert(!m_irValue); // it doesnt make sense to set it twice
  m_irValue = value;
}

AstFunDef::AstFunDef(AstFunDecl* decl, AstValue* body) :
  m_decl(decl ? decl : new AstFunDecl("<unknown_name>")),
  m_body(body ? body : new AstNumber(0)),
  m_irFunction(NULL) {
  assert(m_decl);
  assert(m_body);
}

AstFunDef::~AstFunDef() {
  delete m_body;
}

void AstFunDef::setIrValue(llvm::Value* value) {
  assert(value);
  assert(!m_irFunction); // it doesnt make sense to set it twice
  m_irFunction = dynamic_cast<llvm::Function*>(value);
}

void AstFunDef::setIrFunction(llvm::Function* function) {
  assert(function);
  assert(!m_irFunction); // it doesnt make sense to set it twice
  m_irFunction = function;
}

bool AstFunDef::objectWasModifiedOrRevealedAddr() const {
  return m_decl->stentry()->objectWasModifiedOrRevealedAddr();
}

const ObjType& AstFunDef::objType() const {
  return decl().objType();
}

AstFunDecl::AstFunDecl(const string& name,
  list<AstArgDecl*>* args,
  shared_ptr<const ObjType> ret,
  shared_ptr<SymbolTableEntry> stentry) :
  m_name(name),
  m_args(args ? args : new list<AstArgDecl*>()),
  m_ret(ret ? move(ret) : make_shared<ObjTypeFunda>(ObjTypeFunda::eInt)),
  m_stentry(move(stentry)),
  m_irFunction(NULL) {
}

/** Same as overloaded ctor for convenience; it's easier to pass short
argument lists. An empty string means 'no argument'. */
AstFunDecl::AstFunDecl(const string& name, AstArgDecl* arg1,
  AstArgDecl* arg2, AstArgDecl* arg3) :
  AstFunDecl(name, createArgs(arg1, arg2, arg3)) {}

AstFunDecl::~AstFunDecl() {
  for (list<AstArgDecl*>::const_iterator i=m_args->begin();
       i!=m_args->end(); ++i) {
    delete *i;
  }
  delete m_args;
}

const ObjType& AstFunDecl::retObjType() const {
  assert(m_ret);
  return *m_ret;
}

const ObjType& AstFunDecl::objType() const {
  assert(m_stentry);
  return m_stentry->objType();
}

list<AstArgDecl*>* AstFunDecl::createArgs(AstArgDecl* arg1,
  AstArgDecl* arg2, AstArgDecl* arg3) {
  list<AstArgDecl*>* args = new list<AstArgDecl*>;
  if (arg1) { args->push_back(arg1); }
  if (arg2) { args->push_back(arg2); }
  if (arg3) { args->push_back(arg3); }
  return args;
}

bool AstFunDecl::objectWasModifiedOrRevealedAddr() const {
  assert(m_stentry);
  return m_stentry->objectWasModifiedOrRevealedAddr();
}

void AstFunDecl::setIrValue(llvm::Value* value) {
  assert(value);
  assert(!m_irFunction);  // it doesnt make sense to set it twice
  m_irFunction = dynamic_cast<llvm::Function*>(value);
}

void AstFunDecl::setIrFunction(llvm::Function* function) {
  assert(function);
  assert(!m_irFunction);  // it doesnt make sense to set it twice
  m_irFunction = function;
}

AstDataDecl::AstDataDecl(const string& name, ObjType* objType,
  shared_ptr<SymbolTableEntry> stentry) :
  m_name(name),
  m_objType(objType ? shared_ptr<ObjType>(objType) :
    make_shared<ObjTypeFunda>(ObjTypeFunda::eInt)),
  m_stentry(move(stentry)),
  m_irValue(NULL) {
}

const ObjType& AstDataDecl::objType() const {
  return *m_objType.get();
}

shared_ptr<const ObjType>& AstDataDecl::objTypeShareOwnership() {
  return m_objType;
}

void AstDataDecl::setStentry(shared_ptr<SymbolTableEntry> stentry) {
  assert(stentry.get());
  assert(!m_stentry); // it makes no sense to set it twice
  m_stentry = move(stentry);
}

bool AstDataDecl::objectWasModifiedOrRevealedAddr() const {
  assert(m_stentry);
  return m_stentry->objectWasModifiedOrRevealedAddr();
}

void AstDataDecl::setIrValue(llvm::Value* value) {
  assert(value);
  assert(!m_irValue); // it doesnt make sense to set it twice
  m_irValue = value;
}

AstDataDef::AstDataDef(AstDataDecl* decl, AstValue* initValue) :
  m_decl(decl ? decl : new AstDataDecl("<unknown_name>", new ObjTypeFunda(ObjTypeFunda::eInt))),
  m_ctorArgs(initValue ? new AstCtList(initValue) : new AstCtList()),
  m_implicitInitializer(initValue ? NULL : m_decl->objType().createDefaultAstValue()),
  m_irValue(NULL) {
  assert(m_decl);
  assert(m_ctorArgs);
}

AstDataDef::AstDataDef(AstDataDecl* decl, AstCtList* ctorArgs) :
  m_decl(decl ? decl : new AstDataDecl("<unknown_name>", new ObjTypeFunda(ObjTypeFunda::eInt))),
  m_ctorArgs(ctorArgs ? ctorArgs : new AstCtList()),
  m_implicitInitializer(ctorArgs && !ctorArgs->childs().empty() ? NULL : m_decl->objType().createDefaultAstValue()),
  m_irValue(NULL) {
  assert(m_decl);
  assert(m_ctorArgs);
}

AstDataDef::~AstDataDef() {
  delete m_decl;
  delete m_ctorArgs;
  delete m_implicitInitializer;
}

AstValue& AstDataDef::initValue() const {
  const list<AstValue*>& args = m_ctorArgs->childs();
  if (!args.empty()) {
    assert(args.size()==1); // more ctor arguments not yet supported
    return *(args.front());
  } else {
    assert(m_implicitInitializer);
    return *m_implicitInitializer;
  }
}

const ObjType& AstDataDef::objType() const {
  return decl().objType();
}

bool AstDataDef::objectWasModifiedOrRevealedAddr() const {
  return m_decl->stentry()->objectWasModifiedOrRevealedAddr();
}

void AstDataDef::setIrValue(llvm::Value* value) {
  assert(value);
  assert(!m_irValue); // it doesnt make sense to set it twice
  m_irValue = value;
}

AstNumber::AstNumber(AstNumber::value_t value, ObjTypeFunda* objType) :
  m_value(value),
  m_objType(objType ? objType : new ObjTypeFunda(ObjTypeFunda::eInt)),
  m_irValue(NULL) {
  assert(m_objType);
  // A mutable literal makes no sense.
  assert(!(m_objType->qualifiers() & ObjType::eMutable));
}

AstNumber::AstNumber(AstNumber::value_t value, ObjTypeFunda::EType eType,
  ObjTypeFunda::Qualifiers qualifiers) :
  AstNumber(value, new ObjTypeFunda(eType, qualifiers)) {
}

AstNumber::~AstNumber() {
  delete m_objType;
}

void AstNumber::setIrValue(llvm::Value* value) {
  assert(value);
  assert(!m_irValue); // it doesnt make sense to set it twice
  m_irValue = value;
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
  {eDotAssign, ".="}};

AstOperator::AstOperator(char op, AstCtList* args) :
  AstOperator(static_cast<EOperation>(op), args) {};

AstOperator::AstOperator(const string& op, AstCtList* args) :
  AstOperator(toEOperation(op), args) {};

AstOperator::AstOperator(AstOperator::EOperation op, AstCtList* args) :
  m_op(op),
  m_args(args ? args : new AstCtList),
  m_irValue(NULL) {
  const size_t required_arity =
    (op==eNot || op==eAddrOf || op==eDeref ) ? 1 : 2;
  assert( args->childs().size() == required_arity );
}

AstOperator::AstOperator(char op, AstValue* operand1, AstValue* operand2) :
  AstOperator(static_cast<EOperation>(op), operand1, operand2) {};

AstOperator::AstOperator(const string& op, AstValue* operand1, AstValue* operand2) :
  AstOperator(toEOperation(op), operand1, operand2) {};

AstOperator::AstOperator(AstOperator::EOperation op, AstValue* operand1, AstValue* operand2) :
  AstOperator(op, new AstCtList(operand1, operand2)) {};

AstOperator::~AstOperator() {
  delete m_args;
}

AstOperator::EClass AstOperator::class_() const {
  return classOf(m_op);
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

  case eSeq:
    return eOther;
  }
  assert(false);
  return eOther;
}

AstOperator::EOperation AstOperator::toEOperation(const string& op) {
  if (op.size()==1) {
    return static_cast<EOperation>(op[0]);
  } else {
    auto i = m_opMap.find(op);
    assert( i != m_opMap.end() );
    return i->second;
  }
}

void AstOperator::setIrValue(llvm::Value* value) {
  assert(value);
  assert(!m_irValue); // it doesnt make sense to set it twice
  m_irValue = value;
}

const ObjType& AstOperator::objType() const {
  assert(m_objType);
  return *m_objType.get();
}

void AstOperator::setObjType(unique_ptr<ObjType> objType) {
  assert(!m_objType); // it doesn't make sense to set it twice
  m_objType = move(objType);
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

AstIf::AstIf(AstValue* cond, AstValue* action, AstValue* elseAction) :
  m_condition(cond),
  m_action(action),
  m_elseAction(elseAction),
  m_irValue(NULL) {
  assert(m_condition);
  assert(m_action);
}

AstIf::~AstIf() {
  delete m_condition;
  delete m_action;
  delete m_elseAction;
}

void AstIf::setIrValue(llvm::Value* value) {
  assert(value);
  assert(!m_irValue); // it doesnt make sense to set it twice
  m_irValue = value;
}

const ObjType& AstIf::objType() const {
  assert(m_objType);
  return *m_objType.get();
}

void AstIf::setObjType(unique_ptr<ObjType> objType) {
  assert(!m_objType); // it doesn't make sense to set it twice
  m_objType = move(objType);
}

AstLoop::AstLoop(AstValue* cond, AstValue* body) :
  m_condition(cond),
  m_body(body),
  m_irValue(NULL) {
  assert(m_condition);
  assert(m_body);
}

AstLoop::~AstLoop() {
  delete m_condition;
  delete m_body;
}

const ObjType& AstLoop::objType() const {
  static const ObjTypeFunda void_(ObjTypeFunda::eVoid);
  return void_;
}

void AstLoop::setIrValue(llvm::Value* value) {
  assert(value);
  assert(!m_irValue); // it doesnt make sense to set it twice
  m_irValue = value;
}

AstReturn::AstReturn(AstValue* retVal) :
  m_retVal(retVal),
  m_irValue(NULL) {
  assert(m_retVal);
}

const ObjType& AstReturn::objType() const {
  static const ObjTypeFunda noretObjType(ObjTypeFunda::eNoreturn);
  return noretObjType;
}

AstValue& AstReturn::retVal() const {
  assert(m_retVal);
  return *m_retVal.get();
}

void AstReturn::setIrValue(llvm::Value* value) {
  assert(value);
  assert(!m_irValue); // it doesnt make sense to set it twice
  m_irValue = value;
}

void AstSymbol::setIrValue(llvm::Value* value) {
  assert(value);
  assert(!m_irValue); // it doesnt make sense to set it twice
  m_irValue = value;
}

const ObjType& AstSymbol::objType() const {
  assert(m_stentry);
  return m_stentry->objType();
}

void AstSymbol::setStentry(shared_ptr<SymbolTableEntry> stentry) {
  assert(stentry);
  assert(!m_stentry.get()); // it makes no sense to set it twice
  m_stentry = move(stentry);
}

bool AstSymbol::objectWasModifiedOrRevealedAddr() const {
  assert(m_stentry);
  return m_stentry->objectWasModifiedOrRevealedAddr();
}

AstFunCall::AstFunCall(AstValue* address, AstCtList* args) :
  m_address(address ? address : new AstSymbol("")),
  m_args(args ? args : new AstCtList()),
  m_irValue(NULL) {
  assert(m_address);
  assert(m_args);
}

AstFunCall::~AstFunCall() {
  delete m_args;
  delete m_address;
}

void AstFunCall::setIrValue(llvm::Value* value) {
  assert(value);
  assert(!m_irValue); // it doesnt make sense to set it twice
  m_irValue = value;
}

const ObjType& AstFunCall::objType() const {
  if ( !m_ret ) {
    const auto& objType = m_address->objType();
    assert(typeid(objType)==typeid(ObjTypeFun));
    const auto& objTypeFun = static_cast<const ObjTypeFun&>(objType);
    auto objTypeRet = objTypeFun.ret().clone();
    objTypeRet->removeQualifiers(ObjType::eMutable);
    m_ret = unique_ptr<const ObjType>(objTypeRet);
  }
  return *m_ret;
}

/** The list's elements must be non-null */
AstCtList::AstCtList(list<AstValue*>* childs) :
  m_childs(childs ? childs : new list<AstValue*>() ) {
  assert(m_childs);
  for (list<AstValue*>::iterator it = m_childs->begin();
       it != m_childs->end();
       ++it) {
    assert(*it);
  }
}

/** When child is NULL it is ignored */
AstCtList::AstCtList(AstValue* child) :
  m_childs(new list<AstValue*>()) {
  assert(m_childs);
  if (child) { m_childs->push_back(child); }
}

/** NULL childs are ignored.*/
AstCtList::AstCtList(AstValue* child1, AstValue* child2, AstValue* child3,
  AstValue* child4, AstValue* child5, AstValue* child6) :
  m_childs(new list<AstValue*>()) {
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
    for (list<AstValue*>::iterator i=m_childs->begin(); i!=m_childs->end(); ++i) {
      delete (*i);
    }
  }
  delete m_childs;
}

void AstCtList::releaseOwnership() {
  m_owner = false;
}

/** When child is NULL it is ignored */
AstCtList* AstCtList::Add(AstValue* child) {
  if (child) m_childs->push_back(child);
  return this;
}

/** NULL childs are ignored. */
AstCtList* AstCtList::Add(AstValue* child1, AstValue* child2, AstValue* child3) {
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
void AstFunDecl::accept(AstConstVisitor& visitor) const { visitor.visit(*this); }
void AstDataDecl::accept(AstConstVisitor& visitor) const { visitor.visit(*this); }
void AstArgDecl::accept(AstConstVisitor& visitor) const { visitor.visit(*this); }
void AstDataDef::accept(AstConstVisitor& visitor) const { visitor.visit(*this); }
void AstNumber::accept(AstConstVisitor& visitor) const { visitor.visit(*this); }
void AstSymbol::accept(AstConstVisitor& visitor) const { visitor.visit(*this); }
void AstFunCall::accept(AstConstVisitor& visitor) const { visitor.visit(*this); }
void AstOperator::accept(AstConstVisitor& visitor) const { visitor.visit(*this); }
void AstIf::accept(AstConstVisitor& visitor) const { visitor.visit(*this); }
void AstLoop::accept(AstConstVisitor& visitor) const { visitor.visit(*this); }
void AstReturn::accept(AstConstVisitor& visitor) const { visitor.visit(*this); }
void AstCtList::accept(AstConstVisitor& visitor) const { visitor.visit(*this); }

void AstNop::accept(AstVisitor& visitor) { visitor.visit(*this); }
void AstBlock::accept(AstVisitor& visitor) { visitor.visit(*this); }
void AstCast::accept(AstVisitor& visitor) { visitor.visit(*this); }
void AstFunDef::accept(AstVisitor& visitor) { visitor.visit(*this); }
void AstFunDecl::accept(AstVisitor& visitor) { visitor.visit(*this); }
void AstDataDecl::accept(AstVisitor& visitor) { visitor.visit(*this); }
void AstArgDecl::accept(AstVisitor& visitor) { visitor.visit(*this); }
void AstDataDef::accept(AstVisitor& visitor) { visitor.visit(*this); }
void AstNumber::accept(AstVisitor& visitor) { visitor.visit(*this); }
void AstSymbol::accept(AstVisitor& visitor) { visitor.visit(*this); }
void AstFunCall::accept(AstVisitor& visitor) { visitor.visit(*this); }
void AstOperator::accept(AstVisitor& visitor) { visitor.visit(*this); }
void AstIf::accept(AstVisitor& visitor) { visitor.visit(*this); }
void AstLoop::accept(AstVisitor& visitor) { visitor.visit(*this); }
void AstReturn::accept(AstVisitor& visitor) { visitor.visit(*this); }
void AstCtList::accept(AstVisitor& visitor) { visitor.visit(*this); }
