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

void IrBuilderAst::visit(const AstOperator& op) {
  int result = 0;
  // order of visit childs = order of push: lhs, rhs
  // ie top=rhs, top-1=lhs
  int rhs = valueStack.top(); valueStack.pop();
  int lhs = valueStack.top(); valueStack.pop();
  switch (op.op()) {
  case '-': result = lhs - rhs; break;
  case '+': result = lhs + rhs; break;
  case '*': result = lhs * rhs; break;
  case '/': result = lhs / rhs; break;
  default: break;
  }
  valueStack.push(result);
}

void IrBuilderAst::visit(const AstNumber& number) {
  valueStack.push(number.value());
}
