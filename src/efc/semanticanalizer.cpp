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

void SemanticAnalizer::visit(AstOperator&) {}

void SemanticAnalizer::visit(AstNumber&) {}

void SemanticAnalizer::visit(AstSymbol&) {}

void SemanticAnalizer::visit(AstFunCall&) {}

void SemanticAnalizer::visit(AstFunDef&) {}

void SemanticAnalizer::visit(AstFunDecl&) {}

void SemanticAnalizer::visit(AstDataDecl&) {}

void SemanticAnalizer::visit(AstArgDecl&) {}

void SemanticAnalizer::visit(AstDataDef&) {}

void SemanticAnalizer::visit(AstIf&) {}
