#include "envinserter.h"
#include "ast.h"
#include "env.h"
#include "errorhandler.h"
#include "astdefaultiterator.h"
using namespace std;

EnvInserter::EnvInserter(Env& env, ErrorHandler& errorHandler) :
  m_env(env),
  m_errorHandler(errorHandler) {
}

void EnvInserter::insertIntoEnv(AstNode& root) {
  root.accept(*this);
}

void EnvInserter::visit(AstBlock& block) {
  Env::AutoScope scope(m_env, block.name(),
    Env::AutoScope::insertScopeAndDescent);

  AstDefaultIterator::visit(block);
}

void EnvInserter::visit(AstDataDef& dataDef) {
  if (dataDef.declaredStorageDuration() != StorageDuration::eMember) {
    const auto node = m_env.insertLeaf(dataDef.name(),      dataDef.fqNameProvider());
    if (nullptr==node) {
      Error::throwError(m_errorHandler, Error::eRedefinition, dataDef.name());
    }

    *node = &dataDef;
  }

  AstDefaultIterator::visit(dataDef);
}

void EnvInserter::visit(AstFunDef& funDef) {
  EnvNode** node{};
  Env::AutoScope scope(m_env, funDef.name(), funDef.fqNameProvider(), node,
    Env::AutoScope::insertScopeAndDescent);
  if (!node) {
    Error::throwError(m_errorHandler, Error::eRedefinition, funDef.name());
  }

  *node = &funDef;

  AstDefaultIterator::visit(funDef);
}
