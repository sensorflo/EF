#include "signatureaugmentor.h"
#include "ast.h"
#include "env.h"
#include "errorhandler.h"
#include "astdefaultiterator.h"
#include "object.h"
using namespace std;

SignatureAugmentor::SignatureAugmentor(Env& env, ErrorHandler& errorHandler) :
  m_env(env) {
}

void SignatureAugmentor::augmentEntities(AstNode& root) {
  root.accept(*this);
}

void SignatureAugmentor::visit(AstBlock& block) {
  Env::AutoScope scope(m_env, block.name(), Env::AutoScope::descentScope);
  AstDefaultIterator::visit(block);
}

void SignatureAugmentor::visit(AstFunDef& funDef) {
  Env::AutoScope scope(m_env, funDef.name(), Env::AutoScope::descentScope);
  funDef.createAndSetObjType();
  AstDefaultIterator::visit(funDef);
}
