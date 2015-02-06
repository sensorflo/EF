#include "astdefaultiterator.h"
#include "ast.h"
using namespace std;

void AstDefaultIterator::visit(AstCast& cast) {
  cast.child().accept(*this);
}

void AstDefaultIterator::visit(AstCtList& ctList) {
  m_visitor.visit(ctList);
  list<AstValue*>& childs = ctList.childs();
  for (list<AstValue*>::const_iterator i=childs.begin();
       i!=childs.end(); ++i) {
    (*i)->accept(*this);
  }
}

void AstDefaultIterator::visit(AstOperator& op) {
  m_visitor.visit(op);
  op.args().accept(*this);
}

void AstDefaultIterator::visit(AstNumber& number) {
  m_visitor.visit(number);
}

void AstDefaultIterator::visit(AstSymbol& symbol) {
  m_visitor.visit(symbol);
}

void AstDefaultIterator::visit(AstFunCall& funCall) {
  m_visitor.visit(funCall);
  funCall.address().accept(*this);
  funCall.args().accept(*this);
}

void AstDefaultIterator::visit(AstFunDef& funDef) {
  m_visitor.visit(funDef);
  funDef.decl().accept(*this);
  funDef.body().accept(*this);
}

void AstDefaultIterator::visit(AstFunDecl& funDecl) {
  m_visitor.visit(funDecl);
  for (list<AstArgDecl*>::iterator i=funDecl.args().begin();
       i!=funDecl.args().end(); ++i) {
    (*i)->accept(*this);
  }
}

void AstDefaultIterator::visit(AstDataDecl& dataDecl) {
  m_visitor.visit(dataDecl);
}

void AstDefaultIterator::visit(AstArgDecl& argDecl) {
  m_visitor.visit(argDecl);
}

void AstDefaultIterator::visit(AstDataDef& dataDef) {
  m_visitor.visit(dataDef);
  dataDef.decl().accept(*this);
  dataDef.ctorArgs().accept(*this);
}

void AstDefaultIterator::visit(AstIf& if_) {
  m_visitor.visit(if_);
  list<AstIf::ConditionActionPair>::iterator i = if_.conditionActionPairs().begin();
  for ( /*nop*/; i!=if_.conditionActionPairs().end(); ++i ) {
    i->m_condition->accept(*this);
    i->m_action->accept(*this);
  }
  if (if_.elseAction()) {
    if_.elseAction()->accept(*this);
  }
}

