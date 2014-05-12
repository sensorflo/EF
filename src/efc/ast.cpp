#include "ast.h"
#include <sstream>
#include <cassert>
using namespace std;

string AstNode::toStr() const {
  ostringstream ss;
  printTo(ss);
  return ss.str();
}

basic_ostream<char>& AstNumber::printTo(basic_ostream<char>& os) const {
  return os << m_value;
}

AstSymbol::AstSymbol(const string* name, ValueCategory valueCategory) :
  m_name(name ? name : new string("<unknown_name>")),
  m_valueCategory(valueCategory) {
  assert(m_name);
}

AstSymbol::~AstSymbol() {
  delete m_name;
}

basic_ostream<char>& AstSymbol::printTo(basic_ostream<char>& os) const {
  return os << *m_name;
}

AstFunDef::AstFunDef(AstFunDecl* decl, AstSeq* body) :
  m_decl(decl ? decl : new AstFunDecl("<unknown_name>")),
  m_body(body ? body : new AstSeq()) {
  assert(m_decl);
  assert(m_body);
}

AstFunDef::~AstFunDef() {
  delete m_body;
}

void AstFunDef::accept(AstVisitor& visitor) const {
  visitor.visit(*this, AstVisitor::ePreOrder); 

  m_decl->accept(visitor);
  visitor.visit(*this, AstVisitor::eInOrder); 

  m_body->accept(visitor);

  visitor.visit(*this, AstVisitor::ePostOrder); 
}

basic_ostream<char>& AstFunDef::printTo(basic_ostream<char>& os) const {
  os << "fun(";
  m_decl->printTo(os);
  os << " ";
  m_body->printTo(os);
  os <<")"; 
  return os;
}

AstFunDecl::AstFunDecl(const string& name, list<string>* args) :
  m_name(name),
  m_args(args ? args : new list<string>()) {
  assert(m_args);
}

AstFunDecl::~AstFunDecl() {
  delete m_args;
}

basic_ostream<char>& AstFunDecl::printTo(basic_ostream<char>& os) const {
  os << "declfun(" << m_name << " (";
  for (list<string>::const_iterator i=m_args->begin();
       i!=m_args->end(); ++i) {
    if (i!=m_args->begin()) { os << " "; }
    os << *i;
  }
  os << "))";
  return os;
}

AstDataDecl::AstDataDecl(const string& name, EStorage storage) :
  m_name(name),
  m_storage(storage){
}

basic_ostream<char>& AstDataDecl::printTo(basic_ostream<char>& os) const {
  os << "decldata(" << m_name << (m_storage==eAlloca ? " mut" : "") << ")";
  return os;
}

AstDataDef::AstDataDef(AstDataDecl* decl, AstSeq* initValue) :
  m_decl(decl ? decl : new AstDataDecl("<unknown_name>")),
  m_initValue(initValue) {
  assert(m_decl);
}

AstDataDef::~AstDataDef() {
  delete m_decl;
  if ( m_initValue ) { delete m_initValue; }
}

basic_ostream<char>& AstDataDef::printTo(basic_ostream<char>& os) const {
  os << "data(";
  m_decl->printTo(os);
  if (m_initValue) {
    os << " ";
    m_initValue->printTo(os);
  }
  os << ")";
  return os;
}

void AstDataDef::accept(AstVisitor& visitor) const {
  if (m_initValue) {
    m_initValue->accept(visitor);
  }
  visitor.visit(*this);
}

AstOperator::AstOperator(char op, AstValue* lhs, AstValue* rhs) :
  m_op(op),
  m_lhs(lhs ? lhs : new AstNumber(0)),
  m_rhs(rhs ? rhs : new AstNumber(0)) {
  assert(m_lhs);
  assert(m_rhs);
}

AstOperator::~AstOperator() {
  delete m_lhs;
  delete m_rhs;
}

basic_ostream<char>& AstOperator::printTo(basic_ostream<char>& os) const {
  os << m_op << '(';
  m_lhs->printTo(os);
  os << ' ';
  m_rhs->printTo(os);
  os << ')';
  return os;
}

void AstOperator::accept(AstVisitor& visitor) const {
  m_lhs->accept(visitor);
  m_rhs->accept(visitor);
  visitor.visit(*this);
}

AstIf::AstIf(AstSeq* cond, AstSeq* then, AstSeq* else_) :
  m_cond(cond ? cond : new AstSeq()),
  m_then(then ? then : new AstSeq()),
  m_else(else_) {
  assert(m_cond);
  assert(m_then);
}
  
AstIf::~AstIf() {
  delete m_cond;
  delete m_then;
  if (m_else) { delete m_else; }
}

basic_ostream<char>& AstIf::printTo(basic_ostream<char>& os) const {
  os << "if(";
  m_cond->printTo(os);
  os << " ";
  m_then->printTo(os);
  if (m_else) {
    os << " ";
    m_else->printTo(os); }
  os << ")";
  return os;
}

AstFunCall::AstFunCall(const string& name, AstCtList* args) :
  m_name(name),
  m_args(args ? args : new AstCtList()) {
  assert(m_args);
}

basic_ostream<char>& AstFunCall::printTo(basic_ostream<char>& os) const {
  os << m_name << "(";
  m_args->printTo(os);
  os << ")";
  return os;
}

/** When child is NULL it is ignored */
AstSeq::AstSeq(AstNode* child) {
  if (child) { m_childs.push_back(child); }
}

/** NULL childs are ignored.*/
AstSeq::AstSeq(AstNode* child1, AstNode* child2, AstNode* child3) {
  if (child1) { m_childs.push_back(child1); }
  if (child2) { m_childs.push_back(child2); }
  if (child3) { m_childs.push_back(child3); }
}

AstSeq::~AstSeq() {
  for (list<AstNode*>::iterator i=m_childs.begin(); i!=m_childs.end(); ++i) {
    delete (*i);
  }
}

/** When child is NULL it is ignored */
AstSeq* AstSeq::Add(AstNode* child) {
  if (child) { m_childs.push_back(child); }
  return this;
}

/** NULL childs are ignored. */
AstSeq* AstSeq::Add(AstNode* child1, AstNode* child2, AstNode* child3) {
  Add(child1);
  Add(child2);
  Add(child3);
  return this;
}

basic_ostream<char>& AstSeq::printTo(basic_ostream<char>& os) const {
  os << "seq("; 
  for (list<AstNode*>::const_iterator i=m_childs.begin();
       i!=m_childs.end(); ++i) {
    if (i!=m_childs.begin()) { os << " "; }
    (*i)->printTo(os);
  }
  os << ")";
  return os;
}

void AstSeq::accept(AstVisitor& visitor) const {
  visitor.visit(*this, AstVisitor::ePreOrder, 0);
  int childNo = 0;
  for (list<AstNode*>::const_iterator i=m_childs.begin();
       i!=m_childs.end(); ++i, ++childNo) {
    if (i!=m_childs.begin()) {
      // visit with eInOrder _after_ accept, but not after accecpt of last
      // child
      visitor.visit(*this, AstVisitor::eInOrder, childNo-1);
    }
    (*i)->accept(visitor);
  }
  visitor.visit(*this, AstVisitor::ePostOrder, childNo);
}

/** When child is NULL it is ignored */
AstCtList::AstCtList(AstNode* child) {
  if (child) { m_childs.push_back(child); }
}

/** NULL childs are ignored.*/
AstCtList::AstCtList(AstNode* child1, AstNode* child2, AstNode* child3) {
  if (child1) { m_childs.push_back(child1); }
  if (child2) { m_childs.push_back(child2); }
  if (child3) { m_childs.push_back(child3); }
}

AstCtList::~AstCtList() {
  for (list<AstNode*>::iterator i=m_childs.begin(); i!=m_childs.end(); ++i) {
    delete (*i);
  }
}

/** When child is NULL it is ignored */
AstCtList* AstCtList::Add(AstNode* child) {
  if (child) { m_childs.push_back(child); }
  return this;
}

/** NULL childs are ignored. */
AstCtList* AstCtList::Add(AstNode* child1, AstNode* child2, AstNode* child3) {
  Add(child1);
  Add(child2);
  Add(child3);
  return this;
}

basic_ostream<char>& AstCtList::printTo(basic_ostream<char>& os) const {
  for (list<AstNode*>::const_iterator i=m_childs.begin();
       i!=m_childs.end(); ++i) {
    if (i!=m_childs.begin()) { os << " "; }
    (*i)->printTo(os);
  }
  return os;
}

void AstCtList::accept(AstVisitor& visitor) const {
  for (list<AstNode*>::const_iterator i=m_childs.begin();
       i!=m_childs.end(); ++i) {
    (*i)->accept(visitor);
  }
  visitor.visit(*this);
}
