#pragma once
#include "astdefaultiterator.h"

class Env;
class ErrorHandler;
class ObjType;

/** Two responsibilities.
1. 'Type template instanciaton'. AstObjType s create their associated
   ObjType. EF does not yet have templates; but the builtin types raw pointer
   and raw array can be viewed as type templates.
2. Each definition occuring in AST augments its associated entity with it's
   signature. */
class SignatureAugmentor : private AstDefaultIterator {
public:
  SignatureAugmentor(Env& env, ErrorHandler& errorHandler);
  void augmentEntities(AstNode& root);

private:
  void visit(AstBlock& block) override;
  void visit(AstDataDef& dataDef) override;
  void visit(AstFunDef& funDef) override;
  void visit(AstObjTypeSymbol& symbol) override;
  void visit(AstObjTypeQuali& quali) override;
  void visit(AstObjTypePtr& ptr) override;
  void visit(AstClassDef& class_) override;

  Env& m_env;
};
