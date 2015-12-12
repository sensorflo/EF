#pragma once
#include "astvisitor.h"

class Env;
class ErrorHandler;
class ObjType;

/** Each definition occuring in AST augments its associated entity with it's
signature, as defined in the AST. */
class SignatureAugmentor : private AstVisitor {
public:
  SignatureAugmentor(Env& env, ErrorHandler& errorHandler);
  void augmentEntities(AstNode& root);

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
};
