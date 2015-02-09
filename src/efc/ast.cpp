#include "ast.h"
#include "astvisitor.h"
#include "astprinter.h"
#include "errorhandler.h"
#include <cassert>
#include <stdexcept>
using namespace std;

void AstNode::setAccess(Access access, ErrorHandler& errorHandler) {
  if ( access!=eRead ) {
    errorHandler.add(new Error(Error::eWriteToReadOnly));
  }
}

string AstNode::toStr() const {
  return AstPrinter::toStr(*this);
}

ObjType& AstValue::objType() const {
  // KLUDGE: Currently wrongly too much depends on that nearly all expressions
  // are of type int, so for now the default has to be int opposed to an
  // assertion, because here the type really isn't known.
  static ObjTypeFunda ret(ObjTypeFunda::eInt);
  return ret;
}

AstCast::AstCast(AstValue* child, ObjType* objType) :
  m_child(child ? child : new AstNumber(0)),
  m_objType(objType ? objType : new ObjTypeFunda(ObjTypeFunda::eInt)),
  m_irValue(NULL) {
  assert(m_child);
  assert(m_objType);
}

AstCast::AstCast(AstValue* child, ObjTypeFunda::EType objType) :
  m_child(child ? child : new AstNumber(0)),
  m_objType(new ObjTypeFunda(objType)),
  m_irValue(NULL) {
  assert(m_child);
  assert(m_objType);
}

AstCast::~AstCast() {
  delete m_child;
  delete m_objType;
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

AstFunDecl::AstFunDecl(const string& name, list<AstArgDecl*>* args,
  SymbolTableEntry* stentry) :
  m_name(name),
  m_args(args ? args : new list<AstArgDecl*>()),
  m_objType(NULL),
  m_ownerOfObjType(true),
  m_stentry(stentry),
  m_irFunction(NULL) {
  assert(m_args);
  initObjType();
}

/** Same as overloaded ctor for convenience; it's easier to pass short
argument lists. An empty string means 'no argument'. */
AstFunDecl::AstFunDecl(const string& name, AstArgDecl* arg1,
  AstArgDecl* arg2, AstArgDecl* arg3) :
  m_name(name),
  m_args(createArgs(arg1, arg2, arg3)),
  m_objType(NULL),
  m_ownerOfObjType(true),
  m_stentry(NULL),
  m_irFunction(NULL) {
  assert(m_args);
  initObjType();
}

AstFunDecl::~AstFunDecl() {
  for (list<AstArgDecl*>::const_iterator i=m_args->begin();
       i!=m_args->end(); ++i) {
    delete *i;
  }
  delete m_args;
  if (m_ownerOfObjType) { delete m_objType; }
}

void AstFunDecl::initObjType() {
  list<ObjType*>* argtypes = new list<ObjType*>;
  for (list<AstArgDecl*>::iterator i = m_args->begin(); i!=m_args->end(); ++i) {
    // for simplicity, currently only args of type ObjTypeFunda are allowed
    ObjTypeFunda* argtype_src = dynamic_cast<ObjTypeFunda*>(&(*i)->objType());
    assert(argtype_src);
    argtypes->push_back(new ObjTypeFunda(*argtype_src));
  }
  m_objType = new ObjTypeFun(argtypes, new ObjTypeFunda(ObjTypeFunda::eInt));
}

ObjType& AstFunDecl::objType() const {
  return objType(false);
}

ObjType& AstFunDecl::objType(bool stealOwnership) const {
  if (stealOwnership) {
    assert(m_ownerOfObjType);
    m_ownerOfObjType = false;
  }
  return *m_objType;
}

list<AstArgDecl*>* AstFunDecl::createArgs(AstArgDecl* arg1,
  AstArgDecl* arg2, AstArgDecl* arg3) {
  list<AstArgDecl*>* args = new list<AstArgDecl*>;
  if (arg1) { args->push_back(arg1); }
  if (arg2) { args->push_back(arg2); }
  if (arg3) { args->push_back(arg3); }
  return args;
}

void AstFunDecl::setStentry(SymbolTableEntry* stentry) {
  assert(stentry);
  assert(!m_stentry); // it makes no sense to set it twice
  m_stentry = stentry;
}

AstDataDecl::AstDataDecl(const string& name, ObjType* objType,
  SymbolTableEntry* stentry) :
  m_name(name),
  m_objType(objType ? objType : new ObjTypeFunda(ObjTypeFunda::eInt)),
  m_ownerOfObjType(true),
  m_stentry(stentry),
  m_access(eRead),
  m_irValue(NULL) {
}

AstDataDecl::~AstDataDecl() {
  if (m_ownerOfObjType) { delete m_objType; }
}

ObjType& AstDataDecl::objType() const {
  return objType(false);
}

ObjType& AstDataDecl::objType(bool stealOwnership) const {
  if (stealOwnership) {
    assert(m_ownerOfObjType);
    m_ownerOfObjType = false;
  }
  return *m_objType;
}

void AstDataDecl::setStentry(SymbolTableEntry* stentry) {
  assert(stentry);
  assert(!m_stentry); // it makes no sense to set it twice
  m_stentry = stentry;
}

AstDataDef::AstDataDef(AstDataDecl* decl, AstValue* initValue) :
  m_decl(decl ? decl : new AstDataDecl("<unknown_name>", new ObjTypeFunda(ObjTypeFunda::eInt))),
  m_ctorArgs(initValue ? new AstCtList(initValue) : new AstCtList()),
  m_implicitInitializer(initValue ? NULL : m_decl->objType().createDefaultAstValue()),
  m_access(eRead),
  m_irValue(NULL) {
  assert(m_decl);
  assert(m_ctorArgs);
}

AstDataDef::AstDataDef(AstDataDecl* decl, AstCtList* ctorArgs) :
  m_decl(decl ? decl : new AstDataDecl("<unknown_name>", new ObjTypeFunda(ObjTypeFunda::eInt))),
  m_ctorArgs(ctorArgs ? ctorArgs : new AstCtList()),
  m_implicitInitializer(ctorArgs && !ctorArgs->childs().empty() ? NULL : m_decl->objType().createDefaultAstValue()),
  m_access(eRead),
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

AstNumber::AstNumber(int value, ObjType* objType) :
  m_value(value),
  m_objType(objType ? objType : new ObjTypeFunda(ObjTypeFunda::eInt)),
  m_irValue(NULL) {
  assert(m_objType);
}

AstNumber::AstNumber(int value, ObjTypeFunda::EType eType,
  ObjTypeFunda::Qualifier qualifier) :
  m_value(value),
  m_objType(new ObjTypeFunda(eType, qualifier)),
  m_irValue(NULL) {
  assert(m_objType);
}

AstNumber::~AstNumber() {
  delete m_objType;
}

map<string, AstOperator::EOperation> AstOperator::m_opMap;

AstOperator::AstOperator(char op, AstCtList* args) :
  m_op(static_cast<EOperation>(op)),
  m_args(args ? args : new AstCtList),
  m_irValue(NULL) {
  assert(m_args);
}

AstOperator::AstOperator(const string& op, AstCtList* args) :
  m_op(toEOperation(op)),
  m_args(args ? args : new AstCtList),
  m_irValue(NULL) {
  assert(m_args);
}

AstOperator::AstOperator(AstOperator::EOperation op, AstCtList* args) :
  m_op(op),
  m_args(args ? args : new AstCtList),
  m_irValue(NULL) {
  assert(m_args);
}

AstOperator::AstOperator(char op, AstValue* operand1, AstValue* operand2,
  AstValue* operand3) :
  m_op(static_cast<EOperation>(op)),
  m_args(new AstCtList(operand1, operand2, operand3)),
  m_irValue(NULL) {
  assert(m_args);
}

AstOperator::AstOperator(const string& op, AstValue* operand1, AstValue* operand2,
  AstValue* operand3) :
  m_op(toEOperation(op)),
  m_args(new AstCtList(operand1, operand2, operand3)),
  m_irValue(NULL) {
  assert(m_args);
}

AstOperator::AstOperator(AstOperator::EOperation op, AstValue* operand1, AstValue* operand2,
  AstValue* operand3) :
  m_op(op),
  m_args(new AstCtList(operand1, operand2, operand3)),
  m_irValue(NULL) {
  assert(m_args);
}

AstOperator::~AstOperator() {
  delete m_args;
}

AstOperator::EOperation AstOperator::toEOperation(const string& op) {
  if (op.size()==1) {
    return static_cast<EOperation>(op[0]);
  } else {
    if (m_opMap.empty()) {
      m_opMap.insert(make_pair("and", eAnd));
      m_opMap.insert(make_pair("&&", eAnd));
      m_opMap.insert(make_pair("or", eOr));
      m_opMap.insert(make_pair("||", eOr));
      m_opMap.insert(make_pair("==", eEqualTo));
      m_opMap.insert(make_pair("not", eNot));
    }
    map<string,EOperation>::iterator i = m_opMap.find(op);
    assert( i != m_opMap.end() );
    return i->second;
  }
}

basic_ostream<char>& operator<<(basic_ostream<char>& os,
  AstOperator::EOperation op) {
  switch (op) {
  case AstOperator::eAnd: return os << "and";
  case AstOperator::eOr: return os << "or";
  case AstOperator::eNot: return os << "not";
  case AstOperator::eEqualTo: return os << "==";
  default:
    if (static_cast<int>(op)<128) {
      return os << static_cast<char>(op);
    } else {
      assert(false);
    }
  }
}

AstIf::AstIf(list<AstIf::ConditionActionPair>* conditionActionPairs, AstValue* elseAction) :
  m_conditionActionPairs(
    conditionActionPairs ? conditionActionPairs : makeDefaultConditionActionPairs()),
  m_elseAction(elseAction),
  m_irValue(NULL) {
  assert(m_conditionActionPairs);
  assert(!m_conditionActionPairs->empty());
  assert(m_conditionActionPairs->front().m_condition);
  assert(m_conditionActionPairs->front().m_action);
}

AstIf::AstIf(AstValue* cond, AstValue* action, AstValue* elseAction) :
  m_conditionActionPairs(makeConditionActionPairs(cond,action)),
  m_elseAction(elseAction),
  m_irValue(NULL) {
  assert(m_conditionActionPairs);
  assert(!m_conditionActionPairs->empty());
  assert(m_conditionActionPairs->front().m_condition);
  assert(m_conditionActionPairs->front().m_action);
}

AstIf::~AstIf() {
  delete m_conditionActionPairs->front().m_condition;
  delete m_conditionActionPairs->front().m_action;
  delete m_conditionActionPairs;
  if (m_elseAction) { delete m_elseAction; }
}

list<AstIf::ConditionActionPair>* AstIf::makeDefaultConditionActionPairs() {
  list<ConditionActionPair>* tmp = new list<AstIf::ConditionActionPair>();
  tmp->push_back(ConditionActionPair(new AstNumber(0),new AstNumber(0)));
  return tmp;
}

list<AstIf::ConditionActionPair>* AstIf::makeConditionActionPairs(
  AstValue* cond, AstValue* action) {
  list<ConditionActionPair>* tmp = new list<AstIf::ConditionActionPair>();
  tmp->push_back(ConditionActionPair(cond, action));
  return tmp;
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
AstCtList::AstCtList(AstValue* child1, AstValue* child2, AstValue* child3) :
  m_childs(new list<AstValue*>()) {
  assert(m_childs);
  if (child1) { m_childs->push_back(child1); }
  if (child2) { m_childs->push_back(child2); }
  if (child3) { m_childs->push_back(child3); }
}

AstCtList::~AstCtList() {
  for (list<AstValue*>::iterator i=m_childs->begin(); i!=m_childs->end(); ++i) {
    delete (*i);
  }
  delete m_childs;
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
void AstCtList::accept(AstConstVisitor& visitor) const { visitor.visit(*this); }

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
void AstCtList::accept(AstVisitor& visitor) { visitor.visit(*this); }
