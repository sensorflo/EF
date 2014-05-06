#include "irbuilderast.h"
#include <algorithm>
using namespace std;

int IrBuilderAst::buildAndRunModule(const AstSeq& seq) {
  seq.accept(*this);
  int retVal = valueStack.top();
  stack<int> empty; swap( valueStack, empty ); // clears the valueStack
  return retVal;
}

void IrBuilderAst::visit(const AstSeq& /*seq*/) {
  //nop
}

void IrBuilderAst::visit(const AstOperator& /*op*/) {
  //nop
}

void IrBuilderAst::visit(const AstNumber& number) {
  valueStack.push(number.value());
}
