#pragma once
#include "astdefaultiterator.h"

class Env;
class ErrorHandler;
class ObjType;

/** Each definition occuring in AST inserts a new entity into the
environment. Of each entity, only it's name and it's meta type is yet
defined. */
class EnvInserter : private AstDefaultIterator {
public:
  EnvInserter(Env& env, ErrorHandler& errorHandler);
  void insertIntoEnv(AstNode& root);

private:
  enum EScope {
    eGlobalScope,
    eCurrentScope
  };
  void visit(AstBlock& block) override;
  void visit(AstFunDef& funDef) override;

  /** Inserts name into environment. The returned referenced shared pointer
  is null and shall be reseated to new entity. If name is already in env, an
  appropriate error is thrown. */
  std::shared_ptr<Entity>& insertIntoEnv(const std::string& name,
    EScope scope = eCurrentScope);

  Env& m_env;
  ErrorHandler& m_errorHandler;
};
