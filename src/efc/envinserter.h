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

  Env& m_env;
  ErrorHandler& m_errorHandler;
};
