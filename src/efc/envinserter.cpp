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
  AstDefaultIterator iter(*this);
  root.accept(iter);
}

void EnvInserter::visit(AstFunDef& funDef) {
  // Insert name into environment. Note that currently there is a flat
  // namespace regarding function names; i.e. also nested functions are
  // nevertheless in the flat global namespace.
  auto&& insertRet = m_env.insert(funDef.name(), nullptr);
  const auto& wasAlreadyInMap = !insertRet.second;
  if (wasAlreadyInMap) {
    Error::throwError(m_errorHandler, Error::eRedefinition);
  }
  auto& entityInEnvSp = insertRet.first->second;

  // 1) create an function object representing the function, 2) associate it
  // with env and 3) associate it with AST
  auto&& objectSp = make_shared<Object>(StorageDuration::eStatic); // 1
  entityInEnvSp = objectSp; // 2
  funDef.setObject(move(objectSp)); // 3
}
