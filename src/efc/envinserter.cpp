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
  Env::AutoScope scope(m_env);
  AstDefaultIterator::visit(block);
}

void EnvInserter::visit(AstFunDef& funDef) {
  Env::AutoScope scope(m_env);

  // Insert name into environment. Note that currently there is a flat
  // namespace regarding function names; i.e. also nested functions are
  // nevertheless in the flat global namespace.
  auto& entityInEnvSp = insertIntoEnv(funDef.name(), eGlobalScope);

  // 1) create an function object representing the function, 2) associate it
  // with env and 3) associate it with AST
  auto&& objectSp = make_shared<Object>(StorageDuration::eStatic); // 1
  entityInEnvSp = objectSp; // 2
  funDef.setObject(move(objectSp)); // 3

  AstDefaultIterator::visit(funDef);
}

std::shared_ptr<Entity>& EnvInserter::insertIntoEnv(const std::string& name,
  EScope scope) {
  auto&& insertRet = (scope==eCurrentScope) ?
    m_env.insert(name) :
    m_env.insertAtGlobalScope(name);
  const auto& wasAlreadyInMap = !insertRet.second;
  if (wasAlreadyInMap) {
    Error::throwError(m_errorHandler, Error::eRedefinition, name);
  }
  return insertRet.first->second;
}
