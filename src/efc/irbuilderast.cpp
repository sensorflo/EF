#include "irbuilderast.h"

int IrBuilderAst::buildAndRunModule(const AstSeq& seq) {
  return 42;
}

void IrBuilderAst::visit(const AstSeq& /*seq*/) {
  //nop
}

void IrBuilderAst::visit(const AstNumber& /*number*/) {
  //nop
}
