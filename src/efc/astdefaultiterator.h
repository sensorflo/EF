#pragma once
#include "astvisitor.h"

/** Traverses AST: at each node, call its accept method and pass the vistor
given in the ctor. Currently it's only pre order traversal. */
class AstDefaultIterator : public AstVisitor {
public:
  AstDefaultIterator(AstVisitor* visitor = nullptr) : m_visitor(visitor) {}

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

private:
  AstVisitor* m_visitor;
};
