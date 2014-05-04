#include "ast.h"
#include <sstream>
using namespace std;

string AstNode::toStr() {
  ostringstream ss;
  printTo(ss);
  return ss.str();
}

basic_ostream<char>& AstNumber::printTo(basic_ostream<char>& os) {
  return os << m_number;
}

AstSeq::AstSeq(AstNode* child) {
  if (child) { m_childs.push_back(child); }
}

AstSeq::~AstSeq() {
  for (list<AstNode*>::iterator i=m_childs.begin(); i!=m_childs.end(); ++i) {
    delete (*i);
  }
}

AstSeq* AstSeq::Add(AstNode* child) {
  if (child) { m_childs.push_back(child); }
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

