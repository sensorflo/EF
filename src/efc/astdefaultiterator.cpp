#include "astdefaultiterator.h"
#include "ast.h"
using namespace std;

void AstDefaultIterator::visit(AstNop&) {
}

void AstDefaultIterator::visit(AstBlock& block) {
  block.body().accept(*this);
}

void AstDefaultIterator::visit(AstCast& cast) {
  cast.child().accept(m_visitor);
}

void AstDefaultIterator::visit(AstCtList& ctList) {
  list<AstObject*>& childs = ctList.childs();
  for (list<AstObject*>::const_iterator i=childs.begin();
       i!=childs.end(); ++i) {
    (*i)->accept(m_visitor);
  }
}

void AstDefaultIterator::visit(AstSeq& seq) {
  for (const auto& op: seq.operands()) {
    op->accept(m_visitor);
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
  for (list<AstDataDef*>::const_iterator i=funDef.args().begin();
       i!=funDef.args().end(); ++i) {
    (*i)->accept(m_visitor);
  }
  funDef.body().accept(m_visitor);
}

void AstDefaultIterator::visit(AstDataDef& dataDef) {
  dataDef.ctorArgs().accept(m_visitor);
}

void AstDefaultIterator::visit(AstIf& if_) {
  if_.condition().accept(m_visitor);
  if_.action().accept(m_visitor);
  if (if_.elseAction()) {
    if_.elseAction()->accept(m_visitor);
  }
}

void AstDefaultIterator::visit(AstLoop& loop) {
  loop.condition().accept(m_visitor);
  loop.body().accept(m_visitor);
}

void AstDefaultIterator::visit(AstReturn& return_) {
  return_.retVal().accept(m_visitor);
}

