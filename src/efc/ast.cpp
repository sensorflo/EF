#include "ast.h"
#include "irbuilderast.h"
#include <sstream>
#include <cassert>
#include <stdexcept>
using namespace std;

string AstNode::toStr() const {
  ostringstream ss;
  printTo(ss);
  return ss.str();
}

const std::string& AstValue::address_as_id_hack() const {
  throw runtime_error::runtime_error("not an id");
}

basic_ostream<char>& AstNumber::printTo(basic_ostream<char>& os) const {
  return os << m_value;
}

AstSymbol::AstSymbol(const string* name) :
  m_name(name ? name : new string("<unknown_name>")) {
  assert(m_name);
}

AstSymbol::~AstSymbol() {
  delete m_name;
}

llvm::Value* AstSymbol::accept(IrBuilderAst& visitor, Access access) const {
  return visitor.visit(*this, access);
}

basic_ostream<char>& AstSymbol::printTo(basic_ostream<char>& os) const {
  return os << *m_name;
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

llvm::Function* AstFunDef::accept(IrBuilderAst& visitor, Access) const {
  return visitor.visit(*this);
}

basic_ostream<char>& AstFunDef::printTo(basic_ostream<char>& os) const {
  os << "fun(";
  m_decl->printTo(os);
  os << " ";
  m_body->printTo(os);
  os <<")";
  return os;
}

AstFunDecl::AstFunDecl(const string& name, list<AstArgDecl*>* args) :
  m_name(name),
  m_args(args ? args : new list<AstArgDecl*>()) {
  assert(m_args);
}

/** Same as overloaded ctor for convenience; it's easier to pass short
argument lists. An empty string means 'no argument'. */
AstFunDecl::AstFunDecl(const string& name, AstArgDecl* arg1,
  AstArgDecl* arg2, AstArgDecl* arg3) :
  m_name(name),
  m_args(createArgs(arg1, arg2, arg3)) {
  assert(m_args);
}

AstFunDecl::~AstFunDecl() {
  for (list<AstArgDecl*>::const_iterator i=m_args->begin();
       i!=m_args->end(); ++i) {
    delete *i;
  }
  delete m_args;
}

llvm::Function* AstFunDecl::accept(IrBuilderAst& visitor, Access) const {
  return visitor.visit(*this);
}

basic_ostream<char>& AstFunDecl::printTo(basic_ostream<char>& os) const {
  os << "declfun(" << m_name << " (";
  for (list<AstArgDecl*>::const_iterator i=m_args->begin();
       i!=m_args->end(); ++i) {
    if (i!=m_args->begin()) { os << " "; }
    (*i)->printTo(os);
  }
  os << ") ";
  os << ObjTypeFunda(ObjTypeFunda::eInt);
  os << ")";
  return os;
}

list<AstArgDecl*>* AstFunDecl::createArgs(AstArgDecl* arg1,
  AstArgDecl* arg2, AstArgDecl* arg3) {
  list<AstArgDecl*>* args = new list<AstArgDecl*>;
  if (arg1) { args->push_back(arg1); }
  if (arg2) { args->push_back(arg2); }
  if (arg3) { args->push_back(arg3); }
  return args;
}

AstDataDecl::AstDataDecl(const string& name, ObjType* objType) :
  m_name(name),
  m_objType(objType ? objType : new ObjTypeFunda(ObjTypeFunda::eInt)),
  m_ownerOfObjType(true),
  m_type(eNormal) {
}

AstDataDecl::AstDataDecl(const string& name, ObjType* objType, Type type) :
  m_name(name),
  m_objType(objType ? objType : new ObjTypeFunda(ObjTypeFunda::eInt)),
  m_ownerOfObjType(true),
  m_type(type) {
}

AstDataDecl::~AstDataDecl() {
  if (m_ownerOfObjType) { delete m_objType; }
}

llvm::Value* AstDataDecl::accept(IrBuilderAst& visitor, Access access) const {
  return visitor.visit(*this, access);
}

basic_ostream<char>& AstDataDecl::printTo(basic_ostream<char>& os) const {
  if (eNormal==m_type) { os << "decldata"; }
  return os << "(" << m_name << " " << *m_objType << ")";
}

ObjType& AstDataDecl::objType(bool stealOwnership) const {
  if (stealOwnership) {
    assert(m_ownerOfObjType);
    m_ownerOfObjType = false;
  }
  return *m_objType;
}

AstDataDef::AstDataDef(AstDataDecl* decl, AstValue* initValue) :
  m_decl(decl ? decl : new AstDataDecl("<unknown_name>", new ObjTypeFunda(ObjTypeFunda::eInt))),
  m_ctorArgs(new AstCtList(initValue)) {
  assert(m_decl);
}

AstDataDef::AstDataDef(AstDataDecl* decl, AstCtList* ctorArgs) :
  m_decl(decl ? decl : new AstDataDecl("<unknown_name>", new ObjTypeFunda(ObjTypeFunda::eInt))),
  m_ctorArgs(ctorArgs) {
  assert(m_decl);
}

AstDataDef::~AstDataDef() {
  delete m_decl;
  if ( m_ctorArgs ) { delete m_ctorArgs; }
}

llvm::Value* AstDataDef::accept(IrBuilderAst& visitor, Access access) const {
  return visitor.visit(*this, access);
}

basic_ostream<char>& AstDataDef::printTo(basic_ostream<char>& os) const {
  os << "data(";
  m_decl->printTo(os);
  os << " (";
  const AstValue* iv = initValue();
  if (iv) {
    iv->printTo(os);
  }
  os << "))";
  return os;
}

const AstValue* AstDataDef::initValue() const {
  if (m_ctorArgs) {
    const std::list<AstValue*>& args = m_ctorArgs->childs();
    if (!args.empty()) {
      assert(args.size()==1); // more ctor arguments not yet supported
      return args.front();
    }
  }
  return NULL;
}

llvm::Value* AstNumber::accept(IrBuilderAst& visitor, Access access) const {
  return visitor.visit(*this, access);
}

map<string, AstOperator::EOperation> AstOperator::m_opMap;

AstOperator::AstOperator(char op, AstCtList* args) :
  m_op(static_cast<EOperation>(op)),
  m_args(args ? args : new AstCtList) {
  assert(m_args);
}

AstOperator::AstOperator(const string& op, AstCtList* args) :
  m_op(toEOperation(op)),
  m_args(args ? args : new AstCtList) {
  assert(m_args);
}

AstOperator::AstOperator(AstOperator::EOperation op, AstCtList* args) :
  m_op(op),
  m_args(args ? args : new AstCtList) {
  assert(m_args);
}

AstOperator::AstOperator(char op, AstValue* operand1, AstValue* operand2,
  AstValue* operand3) :
  m_op(static_cast<EOperation>(op)),
  m_args(new AstCtList(operand1, operand2, operand3)) {
  assert(m_args);
}

AstOperator::AstOperator(const string& op, AstValue* operand1, AstValue* operand2,
  AstValue* operand3) :
  m_op(toEOperation(op)),
  m_args(new AstCtList(operand1, operand2, operand3)) {
  assert(m_args);
}

AstOperator::AstOperator(AstOperator::EOperation op, AstValue* operand1, AstValue* operand2,
  AstValue* operand3) :
  m_op(op),
  m_args(new AstCtList(operand1, operand2, operand3)) {
  assert(m_args);
}

AstOperator::~AstOperator() {
  delete m_args;
}

llvm::Value* AstOperator::accept(IrBuilderAst& visitor, Access access) const {
  return visitor.visit(*this, access);
}

basic_ostream<char>& AstOperator::printTo(basic_ostream<char>& os) const {
  os << m_op << '(';
  m_args->printTo(os);
  os << ')';
  return os;
}

const list<AstValue*>& AstOperator::argschilds() const {
  return m_args->childs();
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
  m_elseAction(elseAction) {
  assert(m_conditionActionPairs);
  assert(!m_conditionActionPairs->empty());
  assert(m_conditionActionPairs->front().m_condition);
  assert(m_conditionActionPairs->front().m_action);
}

AstIf::AstIf(AstValue* cond, AstValue* action, AstValue* elseAction) :
  m_conditionActionPairs(makeConditionActionPairs(cond,action)),
  m_elseAction(elseAction) {
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

llvm::Value* AstIf::accept(IrBuilderAst& visitor, Access access) const {
  return visitor.visit(*this, access);
}

basic_ostream<char>& AstIf::printTo(basic_ostream<char>& os) const {
  os << "if(";
  list<ConditionActionPair>::const_iterator i = m_conditionActionPairs->begin();
  for ( /*nop*/; i!=m_conditionActionPairs->end(); ++i ) {
    if ( i!=m_conditionActionPairs->begin() ) { os << " "; }
    i->m_condition->printTo(os);
    os << " ";
    i->m_action->printTo(os);
  }
  if (m_elseAction) {
    os << " ";
    m_elseAction->printTo(os); }
  os << ")";
  return os;
}

AstFunCall::AstFunCall(const AstValue* address, AstCtList* args) :
  m_address(address ? address : new AstSymbol(NULL)),
  m_args(args ? args : new AstCtList()) {
  assert(m_args);
}

AstFunCall::~AstFunCall() {
  delete m_args;
  delete m_address;
}

llvm::Value* AstFunCall::accept(IrBuilderAst& visitor, Access access) const {
  return visitor.visit(*this, access);
}

basic_ostream<char>& AstFunCall::printTo(basic_ostream<char>& os) const {
  m_address->printTo(os);
  os << "(";
  m_args->printTo(os);
  os << ")";
  return os;
}

AstSeq::AstSeq(std::list<AstValue*>* childs) :
  m_childs(childs ? childs : new std::list<AstValue*>()) {
}

/** When child is NULL it is ignored */
AstSeq::AstSeq(AstValue* child) :
  m_childs(new std::list<AstValue*>()) {
  if (child) { m_childs->push_back(child); }
}

/** NULL childs are ignored.*/
AstSeq::AstSeq(AstValue* child1, AstValue* child2, AstValue* child3) :
  m_childs(new std::list<AstValue*>()) {
  if (child1) { m_childs->push_back(child1); }
  if (child2) { m_childs->push_back(child2); }
  if (child3) { m_childs->push_back(child3); }
}

AstSeq::~AstSeq() {
  for (list<AstValue*>::iterator i=m_childs->begin(); i!=m_childs->end(); ++i) {
    delete (*i);
  }
  delete m_childs;
}

/** When child is NULL it is ignored */
AstSeq* AstSeq::Add(AstValue* child) {
  if (child) { m_childs->push_back(child); }
  return this;
}

/** NULL childs are ignored. */
AstSeq* AstSeq::Add(AstValue* child1, AstValue* child2, AstValue* child3) {
  Add(child1);
  Add(child2);
  Add(child3);
  return this;
}

llvm::Value* AstSeq::accept(IrBuilderAst& visitor, Access access) const {
  return visitor.visit(*this, access);
}

basic_ostream<char>& AstSeq::printTo(basic_ostream<char>& os) const {
  os << "seq(";
  for (list<AstValue*>::const_iterator i=m_childs->begin();
       i!=m_childs->end(); ++i) {
    if (i!=m_childs->begin()) { os << " "; }
    (*i)->printTo(os);
  }
  os << ")";
  return os;
}

/** The list's elements must be non-null */
AstCtList::AstCtList(std::list<AstValue*>* childs) :
  m_childs(childs ? childs : new std::list<AstValue*>() ) {
  for (std::list<AstValue*>::iterator it = childs->begin();
       it != childs->end();
       ++it) {
    assert(*it);
  }
}

/** When child is NULL it is ignored */
AstCtList::AstCtList(AstValue* child) :
  m_childs(new std::list<AstValue*>()) {
  if (child) { m_childs->push_back(child); }
}

/** NULL childs are ignored.*/
AstCtList::AstCtList(AstValue* child1, AstValue* child2, AstValue* child3) :
  m_childs(new std::list<AstValue*>()) {
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

llvm::Value* AstCtList::accept(IrBuilderAst&, Access) const {
  return NULL;
}

basic_ostream<char>& AstCtList::printTo(basic_ostream<char>& os) const {
  for (list<AstValue*>::const_iterator i=m_childs->begin();
       i!=m_childs->end(); ++i) {
    if (i!=m_childs->begin()) { os << " "; }
    (*i)->printTo(os);
  }
  return os;
}

