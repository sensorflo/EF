#pragma once
#include "nopastvisitor.h"

class Env;
class ErrorHandler;
class ObjType;

/** Each definition occuring in AST inserts a new entity into the
environment. Of each entity, only it's name and it's meta type is yet
defined. */
class EnvInserter : private NopAstVisitor {
public:
  EnvInserter(Env& env, ErrorHandler& errorHandler);
  void insertIntoEnv(AstNode& root);

private:
  void visit(AstFunDef& funDef) override;

  /** Inserts name into environment. The returned referenced shared pointer
  is null and shall be reseated to new entity. If name is already in env, an
  appropriate error is thrown. */
  std::shared_ptr<Entity>& insertIntoEnv(const std::string& name);

  Env& m_env;
  ErrorHandler& m_errorHandler;
};
