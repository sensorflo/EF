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

AstFunDef::AstFunDef(const std::string& name, AstSeq* body) :
  m_name(name),
  m_body(body ? body : new AstSeq()) {
  assert(m_body);
}

AstFunDef::~AstFunDef() {
  delete m_body;
}

basic_ostream<char>& AstFunDef::printTo(basic_ostream<char>& os) const {
  os << "fun(" << m_name << ",";
  m_body->printTo(os);
  os <<")"; 
  return os;
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
  os << ',';
  m_rhs->printTo(os);
  os << ')';
  return os;
}

void AstOperator::accept(AstVisitor& visitor) const {
  m_lhs->accept(visitor);
  m_rhs->accept(visitor);
  visitor.visit(*this);
}

AstFunCall::AstFunCall(const std::string& name, AstCtList* args) :
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
    if (i!=m_childs.begin()) { os << ","; }
    (*i)->printTo(os);
  }
  os << ")";
  return os;
}

void AstSeq::accept(AstVisitor& visitor) const {
  for (list<AstNode*>::const_iterator i=m_childs.begin();
       i!=m_childs.end(); ++i) {
    (*i)->accept(visitor);
  }
  visitor.visit(*this);
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
    if (i!=m_childs.begin()) { os << ","; }
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
