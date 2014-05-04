#include "ast.h"
#include <sstream>
using namespace std;

string AstNode::toStr() {
  ostringstream ss;
  printTo(ss);
  return ss.str();
}

basic_ostream<char>& AstNumber::printTo(basic_ostream<char>& os) {
  return os << m_value;
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

basic_ostream<char>& AstSeq::printTo(basic_ostream<char>& os) {
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
  visitor.visit(*this);
  for (list<AstNode*>::const_iterator i=m_childs.begin();
       i!=m_childs.end(); ++i) {
    (*i)->accept(visitor);
  }
}
