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

AstValue::AstValue(Access access, std::shared_ptr<SymbolTableEntry> stentry) :
  AstNode(access),
  m_stentry(move(stentry)) {
}

AstValue::AstValue(Access access) :
  AstValue(access, nullptr) {
}

AstValue::AstValue(std::shared_ptr<SymbolTableEntry> stentry) :
  AstValue(eRead, move(stentry)) {
}

AstValue::~AstValue() {
}

const ObjType& AstValue::objType() const {
  assert(m_stentry);
  return m_stentry->objType();
}

bool AstValue::objectWasModifiedOrRevealedAddr() const {
  assert(m_stentry);
  return m_stentry->objectWasModifiedOrRevealedAddr();
}

void AstValue::setStentry(shared_ptr<SymbolTableEntry> stentry) {
  assert(stentry);
  assert(!m_stentry.get()); // it makes no sense to set it twice
  m_stentry = move(stentry);
}

AstNop::AstNop() :
  AstValue{
    make_shared<SymbolTableEntry>(
      make_shared<ObjTypeFunda>(ObjTypeFunda::eVoid))} {
}

AstBlock::AstBlock(AstValue* body) :
  m_body(body) {
}

AstBlock::~AstBlock() {
}

AstCast::AstCast(ObjType* objType, AstValue* child) :
  AstValue{
    make_shared<SymbolTableEntry>(
      shared_ptr<ObjType>{
        objType ? objType : new ObjTypeFunda(ObjTypeFunda::eInt)})},
  m_child(child ? child : new AstNumber(0)) {
  assert(m_child);

  // only currently for simplicity; the cast, as most expression, creates a
  // temporary object, and temporary objects are immutable
  assert(!(objType->qualifiers() & ObjType::eMutable));
}

AstCast::AstCast(ObjTypeFunda::EType objType, AstValue* child) :
  AstCast{new ObjTypeFunda(objType), child} {
}

AstCast::~AstCast() {
  delete m_child;
}

AstFunDef::AstFunDef(AstFunDecl* decl, AstValue* body) :
  m_decl(decl ? decl : new AstFunDecl("<unknown_name>")),
  m_body(body ? body : new AstNumber(0)) {
  assert(m_decl);
  assert(m_body);
}

AstFunDef::~AstFunDef() {
  delete m_body;
}

AstFunDecl::AstFunDecl(const string& name,
  list<AstArgDecl*>* args,
  shared_ptr<const ObjType> ret,
  shared_ptr<SymbolTableEntry> stentry) :
  AstValue{move(stentry)},
  m_name(name),
  m_args(args ? args : new list<AstArgDecl*>()),
  m_ret(ret ? move(ret) : make_shared<ObjTypeFunda>(ObjTypeFunda::eInt)) {
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

list<AstArgDecl*>* AstFunDecl::createArgs(AstArgDecl* arg1,
  AstArgDecl* arg2, AstArgDecl* arg3) {
  list<AstArgDecl*>* args = new list<AstArgDecl*>;
  if (arg1) { args->push_back(arg1); }
  if (arg2) { args->push_back(arg2); }
  if (arg3) { args->push_back(arg3); }
  return args;
}

AstDataDecl::AstDataDecl(const string& name, const ObjType* declaredObjType) :
  m_name(name),
  m_declaredObjType(declaredObjType ?
    shared_ptr<const ObjType>(declaredObjType) :
    make_shared<ObjTypeFunda>(ObjTypeFunda::eInt)) {
}

const ObjType& AstDataDecl::declaredObjType() const {
  return *m_declaredObjType.get();
}

shared_ptr<const ObjType>& AstDataDecl::declaredObjTypeAsSp() {
  return m_declaredObjType;
}

shared_ptr<SymbolTableEntry>& AstDataDecl::createAndSetStEntryUsingDeclaredObjType() {
  setStentry(make_shared<SymbolTableEntry>(m_declaredObjType));
  return m_stentry;
}

AstDataDef::AstDataDef(AstDataDecl* decl, AstValue* initValue) :
  m_decl(decl ? decl : new AstDataDecl("<unknown_name>", new ObjTypeFunda(ObjTypeFunda::eInt))),
  m_ctorArgs(initValue ? new AstCtList(initValue) : new AstCtList()),
  m_implicitInitializer(initValue ? NULL : m_decl->declaredObjType().createDefaultAstValue()) {
  assert(m_decl);
  assert(m_ctorArgs);
}

AstDataDef::AstDataDef(AstDataDecl* decl, AstCtList* ctorArgs) :
  m_decl(decl ? decl : new AstDataDecl("<unknown_name>", new ObjTypeFunda(ObjTypeFunda::eInt))),
  m_ctorArgs(ctorArgs ? ctorArgs : new AstCtList()),
  m_implicitInitializer(ctorArgs && !ctorArgs->childs().empty() ? NULL : m_decl->declaredObjType().createDefaultAstValue()) {
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

AstNumber::AstNumber(GeneralValue value, ObjTypeFunda* objType) :
  AstValue{
    make_shared<SymbolTableEntry>(
      objType ?
      shared_ptr<const ObjType>{objType} :
      make_shared<const ObjTypeFunda>(ObjTypeFunda::eInt))},
  m_value(value) {
  // A mutable literal makes no sense.
  assert(!(AstValue::objType().qualifiers() & ObjType::eMutable));
}

AstNumber::AstNumber(GeneralValue value, ObjTypeFunda::EType eType,
  ObjTypeFunda::Qualifiers qualifiers) :
  AstNumber(value, new ObjTypeFunda(eType, qualifiers)) {
}

const ObjTypeFunda& AstNumber::objType() const {
  return static_cast<const ObjTypeFunda&>(AstValue::objType());
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
  m_elseAction(elseAction) {
  assert(m_condition);
  assert(m_action);
}

AstIf::~AstIf() {
  delete m_condition;
  delete m_action;
  delete m_elseAction;
}

AstLoop::AstLoop(AstValue* cond, AstValue* body) :
  AstValue{
    make_shared<SymbolTableEntry>(
      make_shared<ObjTypeFunda>(ObjTypeFunda::eVoid))},
  m_condition(cond),
  m_body(body) {
  assert(m_condition);
  assert(m_body);
}

AstLoop::~AstLoop() {
  delete m_condition;
  delete m_body;
}

AstReturn::AstReturn(AstValue* retVal) :
  AstValue{
    make_shared<SymbolTableEntry>(
      make_shared<ObjTypeFunda>(ObjTypeFunda::eNoreturn))},
  m_retVal(retVal) {
  assert(m_retVal);
}

AstReturn::~AstReturn() {
}

AstValue& AstReturn::retVal() const {
  assert(m_retVal);
  return *m_retVal.get();
}

AstFunCall::AstFunCall(AstValue* address, AstCtList* args) :
  m_address(address ? address : new AstSymbol("")),
  m_args(args ? args : new AstCtList()) {
  assert(m_address);
  assert(m_args);
}

AstFunCall::~AstFunCall() {
  delete m_args;
  delete m_address;
}

void AstFunCall::createAndSetStEntryUsingRetObjType() {
  const auto& objType = m_address->objType();
  assert(typeid(objType)==typeid(ObjTypeFun));
  const auto& objTypeFun = static_cast<const ObjTypeFun&>(objType);
  auto objTypeRet = objTypeFun.ret().clone();
  objTypeRet->removeQualifiers(ObjType::eMutable);
  setStentry(make_shared<SymbolTableEntry>(unique_ptr<const ObjType>{objTypeRet}));
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
