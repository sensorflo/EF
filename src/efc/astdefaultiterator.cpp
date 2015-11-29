#include "astdefaultiterator.h"
#include "ast.h"
using namespace std;

void AstDefaultIterator::visit(AstNop& nop) {
  nop.accept(m_visitor);
}

void AstDefaultIterator::visit(AstBlock& block) {
  block.accept(m_visitor);
  block.body().accept(*this);
}

void AstDefaultIterator::visit(AstCast& cast) {
  cast.accept(m_visitor);
  cast.child().accept(*this);
}

void AstDefaultIterator::visit(AstCtList& ctList) {
  ctList.accept(m_visitor);
  list<AstObject*>& childs = ctList.childs();
  for (list<AstObject*>::const_iterator i=childs.begin();
       i!=childs.end(); ++i) {
    (*i)->accept(*this);
  }
}

void AstDefaultIterator::visit(AstSeq& seq) {
  seq.accept(m_visitor);
  for (const auto& op: seq.operands()) {
    op->accept(*this);
  }
}

void AstDefaultIterator::visit(AstOperator& op) {
  op.accept(m_visitor);
  op.args().accept(*this);
}

void AstDefaultIterator::visit(AstNumber& number) {
  number.accept(m_visitor);
}

void AstDefaultIterator::visit(AstSymbol& symbol) {
  symbol.accept(m_visitor);
}

void AstDefaultIterator::visit(AstFunCall& funCall) {
  funCall.accept(m_visitor);
  funCall.address().accept(*this);
  funCall.args().accept(*this);
}

void AstDefaultIterator::visit(AstFunDef& funDef) {
  funDef.accept(m_visitor);
  for (list<AstDataDef*>::const_iterator i=funDef.args().begin();
       i!=funDef.args().end(); ++i) {
    (*i)->accept(*this);
  }
  funDef.body().accept(*this);

}

void AstDefaultIterator::visit(AstDataDef& dataDef) {
  dataDef.accept(m_visitor);
  dataDef.ctorArgs().accept(*this);
}

void AstDefaultIterator::visit(AstIf& if_) {
  if_.accept(m_visitor);
  if_.condition().accept(*this);
  if_.action().accept(*this);
  if (if_.elseAction()) {
    if_.elseAction()->accept(*this);
  }
}

void AstDefaultIterator::visit(AstLoop& loop) {
  loop.accept(m_visitor);
  loop.condition().accept(*this);
  loop.body().accept(*this);
}

void AstDefaultIterator::visit(AstReturn& return_) {
  return_.accept(m_visitor);
  return_.retVal().accept(*this);
}

void AstDefaultIterator::visit(AstObjTypeSymbol& symbol) {
  symbol.accept(m_visitor);
}

void AstDefaultIterator::visit(AstObjTypeQuali& quali) {
  quali.accept(m_visitor);
  quali.targetType().accept(*this);
}

void AstDefaultIterator::visit(AstObjTypePtr& ptr) {
  ptr.accept(m_visitor);
  ptr.pointee().accept(*this);
}

void AstDefaultIterator::visit(AstClassDef& class_) {
  class_.accept(m_visitor);
  for (const auto& dataMember : class_.dataMembers()) {
    dataMember->accept(*this);
  }
}
