#pragma once
#include "astvisitor.h"

class Env;
class ErrorHandler;
class ObjType;

/** Each definition occuring in AST inserts a new entity into the
environment. Of each entity, only it's name and it's meta type is yet
defined. */
class EnvInserter : private AstVisitor {
public:
  EnvInserter(Env& env, ErrorHandler& errorHandler);
  void insertIntoEnv(AstNode& root);

private:
  virtual void visit(AstNop& nop);
  virtual void visit(AstBlock& block);
  virtual void visit(AstCast& cast);
  virtual void visit(AstCtList& ctList);
  virtual void visit(AstOperator& op);
  virtual void visit(AstSeq& seq);
  virtual void visit(AstNumber& number);
  virtual void visit(AstSymbol& symbol);
  virtual void visit(AstFunCall& funCall);
  virtual void visit(AstFunDef& funDef);
  virtual void visit(AstDataDef& dataDef);
  virtual void visit(AstIf& if_);
  virtual void visit(AstLoop& loop);
  virtual void visit(AstReturn& return_);
  virtual void visit(AstObjTypeSymbol& symbol);
  virtual void visit(AstObjTypeQuali& quali);
  virtual void visit(AstObjTypePtr& ptr);
  virtual void visit(AstClassDef& class_);

  Env& m_env;
  ErrorHandler& m_errorHandler;
};
