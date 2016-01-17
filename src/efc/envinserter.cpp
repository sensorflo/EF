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

void EnvInserter::visit(AstFunDef& funDef) {
  const auto isAtGlobalScope = m_env.isAtGlobalScope();

  std::shared_ptr<Entity>* entity{};
  Env::AutoScope scope(m_env, funDef.name(), entity,
    Env::AutoScope::insertScopeAndDescent);
  if (!entity) {
    Error::throwError(m_errorHandler, Error::eRedefinition, funDef.name());
  }

  // Insert name additionally also into root scope of environment. This is
  // because currently IrGen can't make a globally unique name out of a name
  // nested within the environment.
  if ( !isAtGlobalScope ) {
    auto entityAtGlobal = m_env.insertLeafAtGlobalScope(funDef.name());
    if ( !entityAtGlobal ) {
      Error::throwError(m_errorHandler, Error::eRedefinition, funDef.name());
    }
  }

  // 1) create an function object representing the function, 2) associate it
  // with env and 3) associate it with AST
  auto&& object = make_shared<Object>(StorageDuration::eStatic); // 1
  *entity = object; // 2
  funDef.setObject(move(object)); // 3

  AstDefaultIterator::visit(funDef);
}
