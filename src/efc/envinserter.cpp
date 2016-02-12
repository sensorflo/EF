#include "envinserter.h"
#include "ast.h"
#include "env.h"
#include "errorhandler.h"
#include "astdefaultiterator.h"
#include "object.h"
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
    std::shared_ptr<EnvNode>* node = m_env.insertLeaf(dataDef.name(),
      dataDef.fqNameProvider());
    if (nullptr==node) {
      Error::throwError(m_errorHandler, Error::eRedefinition, dataDef.name());
    }

    // 1) create an data object, 2) associate it with env and 3) associate it
    // with AST
    auto&& object = make_shared<Object>(dataDef.declaredStorageDuration()); // 1
    *node = object; // 2
    dataDef.setObject(move(object)); // 3
  }

  AstDefaultIterator::visit(dataDef);
}

void EnvInserter::visit(AstFunDef& funDef) {
  std::shared_ptr<EnvNode>* node{};
  Env::AutoScope scope(m_env, funDef.name(), funDef.fqNameProvider(), node,
    Env::AutoScope::insertScopeAndDescent);
  if (!node) {
    Error::throwError(m_errorHandler, Error::eRedefinition, funDef.name());
  }

  // 1) create an function object representing the function, 2) associate it
  // with env and 3) associate it with AST
  auto&& object = make_shared<Object>(StorageDuration::eStatic); // 1
  *node = object; // 2
  funDef.setObject(move(object)); // 3

  AstDefaultIterator::visit(funDef);
}
