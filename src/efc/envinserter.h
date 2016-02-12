#pragma once
#include "astdefaultiterator.h"

class Env;
class ErrorHandler;
class ObjType;

/** Each definition occuring in AST inserts a new node into the environment,
and associate the AST node being the definition and the new node. Of each
node, only it's name and it's meta type is yet defined. */
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
  void visit(AstDataDef& dataDef) override;
  void visit(AstFunDef& funDef) override;

  Env& m_env;
  ErrorHandler& m_errorHandler;
};
