#pragma once
#include "astdefaultiterator.h"

class Env;
class ErrorHandler;
class ObjType;

/** Each definition occuring in AST augments its associated entity with it's
signature, as defined in the AST. */
class SignatureAugmentor : private AstDefaultIterator {
public:
  SignatureAugmentor(Env& env, ErrorHandler& errorHandler);
  void augmentEntities(AstNode& root);

private:
  void visit(AstFunDef& funDef) override;
};
