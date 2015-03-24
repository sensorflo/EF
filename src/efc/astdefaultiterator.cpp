#include "astdefaultiterator.h"
#include "ast.h"
using namespace std;

void AstDefaultIterator::visit(AstNop&) {
}

void AstDefaultIterator::visit(AstCast& cast) {
  cast.child().accept(m_visitor);
}

void AstDefaultIterator::visit(AstCtList& ctList) {
  list<AstValue*>& childs = ctList.childs();
  for (list<AstValue*>::const_iterator i=childs.begin();
       i!=childs.end(); ++i) {
    (*i)->accept(m_visitor);
  }
}

void AstDefaultIterator::visit(AstOperator& op) {
  op.args().accept(m_visitor);
}

void AstDefaultIterator::visit(AstNumber& number) {
}

void AstDefaultIterator::visit(AstSymbol& symbol) {
}

void AstDefaultIterator::visit(AstFunCall& funCall) {
  funCall.address().accept(m_visitor);
  funCall.args().accept(m_visitor);
}

void AstDefaultIterator::visit(AstFunDef& funDef) {
  funDef.decl().accept(m_visitor);
  funDef.body().accept(m_visitor);
}

void AstDefaultIterator::visit(AstFunDecl& funDecl) {
  for (list<AstArgDecl*>::const_iterator i=funDecl.args().begin();
       i!=funDecl.args().end(); ++i) {
    (*i)->accept(m_visitor);
  }
}

void AstDefaultIterator::visit(AstDataDecl& dataDecl) {
}

void AstDefaultIterator::visit(AstArgDecl& argDecl) {
}

void AstDefaultIterator::visit(AstDataDef& dataDef) {
  dataDef.decl().accept(m_visitor);
  dataDef.ctorArgs().accept(m_visitor);
}

void AstDefaultIterator::visit(AstIf& if_) {
  if_.condition().accept(m_visitor);
  if_.action().accept(m_visitor);
  if (if_.elseAction()) {
    if_.elseAction()->accept(m_visitor);
  }
}

void AstDefaultIterator::visit(AstReturn& return_) {
  return_.retVal().accept(m_visitor);
}

