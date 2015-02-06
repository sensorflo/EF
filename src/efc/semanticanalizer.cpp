#include "semanticanalizer.h"
#include "ast.h"
#include "astdefaultiterator.h"
#include <stdexcept>
using namespace std;

AstNode* SemanticAnalizer::transform(AstNode& root) {
  AstDefaultIterator iter(*this);
  root.accept(iter);
  return &root;
}

void SemanticAnalizer::visit(AstCast&) {}

void SemanticAnalizer::visit(AstCtList&) {}

void SemanticAnalizer::visit(AstOperator& op) {
  const list<AstValue*>& argschilds = op.args().childs();
  switch (op.op()) {
  case AstOperator::eSeq:
    if (argschilds.empty()) {
      throw runtime_error::runtime_error("Empty sequence not allowed (yet)");
    }
    break;
  default:
    break;
  }
}

void SemanticAnalizer::visit(AstNumber&) {}

void SemanticAnalizer::visit(AstSymbol&) {}

void SemanticAnalizer::visit(AstFunCall&) {}

void SemanticAnalizer::visit(AstFunDef&) {}

void SemanticAnalizer::visit(AstFunDecl&) {}

void SemanticAnalizer::visit(AstDataDecl&) {}

void SemanticAnalizer::visit(AstArgDecl&) {}

void SemanticAnalizer::visit(AstDataDef& dataDef) {
  // initializer must be of same type. Currently there are no implicit conversions
  if ( ObjType::eNoMatch ==
    dataDef.decl().objType().match(dataDef.initValue().objType()) ) {
    throw runtime_error::runtime_error("Object type missmatch");
  }
}

void SemanticAnalizer::visit(AstIf&) {}
