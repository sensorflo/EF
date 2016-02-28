#include "signatureaugmentor.h"
#include "ast.h"
#include "env.h"
#include "errorhandler.h"
#include "astdefaultiterator.h"
using namespace std;

SignatureAugmentor::SignatureAugmentor(Env& env, ErrorHandler& errorHandler) :
  m_env(env) {
}

void SignatureAugmentor::augmentEntities(AstNode& root) {
  root.accept(*this);
}

void SignatureAugmentor::visit(AstBlock& block) {
  Env::AutoScope scope(m_env, block);
  AstDefaultIterator::visit(block);
}

void SignatureAugmentor::visit(AstDataDef& dataDef) {
  // Note that in case SemanticAnalizer _will_ create an AST Node for the
  // initializer, that future AST subtree obviously will not be visited here.
  AstDefaultIterator::visit(dataDef);
}

void SignatureAugmentor::visit(AstFunDef& funDef) {
  Env::AutoScope scope(m_env, funDef);
  AstDefaultIterator::visit(funDef);
}

void SignatureAugmentor::visit(AstObjTypeSymbol& symbol) {
  AstDefaultIterator::visit(symbol);
  symbol.createAndSetObjType();
}

void SignatureAugmentor::visit(AstObjTypeQuali& quali) {
  AstDefaultIterator::visit(quali);
  quali.createAndSetObjType();
}

void SignatureAugmentor::visit(AstObjTypePtr& ptr) {
  AstDefaultIterator::visit(ptr);
  ptr.createAndSetObjType();
}

