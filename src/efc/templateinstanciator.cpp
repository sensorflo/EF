#include "templateinstanciator.h"

#include "ast.h"
#include "astdefaultiterator.h"
#include "env.h"
#include "errorhandler.h"

using namespace std;

TemplateInstanciator::TemplateInstanciator(
  Env& env, ErrorHandler& /*errorHandler*/)
  : m_env(env) {
}

void TemplateInstanciator::instanciateTemplates(AstNode& root) {
  root.accept(*this);
}

void TemplateInstanciator::visit(AstBlock& block) {
  Env::AutoScope scope(m_env, block);
  AstDefaultIterator::visit(block);
}

void TemplateInstanciator::visit(AstDataDef& dataDef) {
  // Note that in case SemanticAnalizer _will_ create an AST Node for the
  // initializer, that future AST subtree obviously will not be visited here.
  AstDefaultIterator::visit(dataDef);
}

void TemplateInstanciator::visit(AstFunDef& funDef) {
  Env::AutoScope scope(m_env, funDef);
  AstDefaultIterator::visit(funDef);
}

void TemplateInstanciator::visit(AstObjTypeSymbol& symbol) {
  AstDefaultIterator::visit(symbol);
  symbol.createAndSetObjType();
}

void TemplateInstanciator::visit(AstObjTypeQuali& quali) {
  AstDefaultIterator::visit(quali);
  quali.createAndSetObjType();
}

void TemplateInstanciator::visit(AstObjTypePtr& ptr) {
  AstDefaultIterator::visit(ptr);
  ptr.createAndSetObjType();
}

void TemplateInstanciator::visit(AstClassDef& class_) {
  AstDefaultIterator::visit(class_);
  class_.createAndSetObjType();
}
