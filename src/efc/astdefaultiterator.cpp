#include "astdefaultiterator.h"
#include "ast.h"
using namespace std;

void AstDefaultIterator::visit(AstNop& nop) {
  if ( m_visitor ) {
    nop.accept(*m_visitor);
  }
}

void AstDefaultIterator::visit(AstBlock& block) {
  if ( m_visitor ) {
    block.accept(*m_visitor);
  }
  block.body().accept(*this);
}

void AstDefaultIterator::visit(AstCast& cast) {
  if ( m_visitor ) {
    cast.accept(*m_visitor);
  }
  cast.child().accept(*this);
  cast.specifiedNewAstObjType().accept(*this);
}

void AstDefaultIterator::visit(AstCtList& ctList) {
  if ( m_visitor ) {
    ctList.accept(*m_visitor);
  }
  auto& childs = ctList.childs();
  for (auto i=childs.begin(); i!=childs.end(); ++i) {
    (*i)->accept(*this);
  }
}

void AstDefaultIterator::visit(AstSeq& seq) {
  if ( m_visitor ) {
    seq.accept(*m_visitor);
  }
  for (const auto& op: seq.operands()) {
    op->accept(*this);
  }
}

void AstDefaultIterator::visit(AstOperator& op) {
  if ( m_visitor ) {
    op.accept(*m_visitor);
  }
  op.args().accept(*this);
}

void AstDefaultIterator::visit(AstNumber& number) {
  if ( m_visitor ) {
    number.accept(*m_visitor);
  }
  number.declaredAstObjType().accept(*this);
}

void AstDefaultIterator::visit(AstSymbol& symbol) {
  if ( m_visitor ) {
    symbol.accept(*m_visitor);
  }
}

void AstDefaultIterator::visit(AstFunCall& funCall) {
  if ( m_visitor ) {
    funCall.accept(*m_visitor);
  }
  funCall.address().accept(*this);
  funCall.args().accept(*this);
}

void AstDefaultIterator::visit(AstFunDef& funDef) {
  if ( m_visitor ) {
    funDef.accept(*m_visitor);
  }
  for (auto i=funDef.declaredArgs().begin(); i!=funDef.declaredArgs().end(); ++i) {
    (*i)->accept(*this);
  }
  funDef.ret().accept(*this);
  funDef.body().accept(*this);
}

void AstDefaultIterator::visit(AstDataDef& dataDef) {
  if ( m_visitor ) {
    dataDef.accept(*m_visitor);
  }
  dataDef.declaredAstObjType().accept(*this);
  dataDef.ctorArgs().accept(*this);
}

void AstDefaultIterator::visit(AstIf& if_) {
  if ( m_visitor ) {
    if_.accept(*m_visitor);
  }
  if_.condition().accept(*this);
  if_.action().accept(*this);
  if (if_.elseAction()) {
    if_.elseAction()->accept(*this);
  }
}

void AstDefaultIterator::visit(AstLoop& loop) {
  if ( m_visitor ) {
    loop.accept(*m_visitor);
  }
  loop.condition().accept(*this);
  loop.body().accept(*this);
}

void AstDefaultIterator::visit(AstReturn& return_) {
  if ( m_visitor ) {
    return_.accept(*m_visitor);
  }
  return_.retVal().accept(*this);
}

void AstDefaultIterator::visit(AstObjTypeSymbol& symbol) {
  if ( m_visitor ) {
    symbol.accept(*m_visitor);
  }
}

void AstDefaultIterator::visit(AstObjTypeQuali& quali) {
  if ( m_visitor ) {
    quali.accept(*m_visitor);
  }
  quali.targetType().accept(*this);
}

void AstDefaultIterator::visit(AstObjTypePtr& ptr) {
  if ( m_visitor ) {
    ptr.accept(*m_visitor);
  }
  ptr.pointee().accept(*this);
}

