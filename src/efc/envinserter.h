#pragma once
#include "astdefaultiterator.h"

class Env;
class ErrorHandler;
class ObjType;

/** Each AST node being an definition inserts itself, also being an EnvNode,
into the environment. */
class EnvInserter : private AstDefaultIterator {
public:
  EnvInserter(Env& env, ErrorHandler& errorHandler);
  void insertIntoEnv(AstNode& root);

private:
  enum EScope { eGlobalScope, eCurrentScope };
  void visit(AstBlock& block) override;
  void visit(AstDataDef& dataDef) override;
  void visit(AstFunDef& funDef) override;

  Env& m_env;
  ErrorHandler& m_errorHandler;
};
