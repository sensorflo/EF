#include "envinserter.h"

#include "ast.h"
#include "astdefaultiterator.h"
#include "env.h"
#include "errorhandler.h"

using namespace std;

EnvInserter::EnvInserter(Env& env, ErrorHandler& errorHandler)
  : m_env{env}, m_errorHandler{errorHandler} {
}

void EnvInserter::insertIntoEnv(AstNode& root) {
  root.accept(*this);
}

void EnvInserter::visit(AstBlock& block) {
  Env::AutoScope scope{m_env, block, Env::AutoScope::insertScopeAndDescent};

  AstDefaultIterator::visit(block);
}

void EnvInserter::visit(AstDataDef& dataDef) {
  if (dataDef.declaredStorageDuration() != StorageDuration::eMember) {
    const auto success = m_env.insertLeaf(dataDef);
    if (!success) {
      Error::throwError(
        m_errorHandler, Error::eRedefinition, dataDef.loc(), dataDef.name());
    }
  }

  AstDefaultIterator::visit(dataDef);
}

void EnvInserter::visit(AstFunDef& funDef) {
  auto success = false;
  Env::AutoScope scope{
    m_env, funDef, Env::AutoScope::insertScopeAndDescent, success};
  if (!success) {
    Error::throwError(
      m_errorHandler, Error::eRedefinition, funDef.loc(), funDef.name());
  }

  AstDefaultIterator::visit(funDef);
}
