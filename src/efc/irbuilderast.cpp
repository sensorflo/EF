#include "irbuilderast.h"

int IrBuilderAst::buildAndRunModule(const AstSeq& seq) {
  seq.accept(*this);
  return m_lastValueInSeq;
}

void IrBuilderAst::visit(const AstSeq& /*seq*/) {
  //nop
}

void IrBuilderAst::visit(const AstNumber& number) {
  m_lastValueInSeq = number.value();
}
