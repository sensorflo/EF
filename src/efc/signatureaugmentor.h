#pragma once
#include "astdefaultiterator.h"

class Env;
class ErrorHandler;
class ObjType;

/** 'Object type template instanciaton'. Currently that means that each
AstObjType node does the instanciation it represents.

An AST subtree representing a type expression, i.e. all nodes are of type
AstObjType, really denotes a template instantiation. E.g. "*int" (EF syntax) is
short for "raw_ptr<int>" (using C++ syntax, since EF syntax is not yet there) */
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
