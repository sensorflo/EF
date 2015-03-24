#ifndef ASTDEFAULTITERATOR_H
#define ASTDEFAULTITERATOR_H
#include "astvisitor.h"


/** Traverses AST: at each node, call its accept method and pass the vistor
given in the ctor. */
class AstDefaultIterator : public AstVisitor {

public:
  AstDefaultIterator(AstVisitor& visitor) :
    m_visitor(visitor) {}

  virtual void visit(AstNop& nop);
  virtual void visit(AstCast& cast);
  virtual void visit(AstCtList& ctList);
  virtual void visit(AstOperator& op);
  virtual void visit(AstNumber& number);
  virtual void visit(AstSymbol& symbol);
  virtual void visit(AstFunCall& funCall);
  virtual void visit(AstFunDef& funDef);
  virtual void visit(AstFunDecl& funDecl);
  virtual void visit(AstDataDecl& dataDecl);
  virtual void visit(AstArgDecl& argDecl);
  virtual void visit(AstDataDef& dataDef);
  virtual void visit(AstIf& if_);
  virtual void visit(AstReturn& return_);

private:
  AstVisitor& m_visitor;
};

#endif
