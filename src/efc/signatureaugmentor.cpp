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

  // create object type describing the function
  const auto& argsObjType = new list<shared_ptr<const ObjType>>;
  for (const auto& astArg: funDef.args()) {
    argsObjType->push_back(astArg->declaredObjTypeAsSp());
  }
  auto&& objTypeFun = make_shared<const ObjTypeFun>(argsObjType,
    funDef.retObjTypeAsSp());

  // augment function object associated with funDef with it's signature, i.e
  // the above created object type
  funDef.object()->setObjType(move(objTypeFun));

  AstDefaultIterator::visit(funDef);
}
