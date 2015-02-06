#ifndef ASTDEFAULTITERATOR_H
#define ASTDEFAULTITERATOR_H
#include "astvisitor.h"


/** Pre-order traverses AST, invoking the given visitor's visit method for
each node. */
class AstDefaultIterator : public AstVisitor {

public:
  AstDefaultIterator(AstVisitor& visitor) :
    m_visitor(visitor) {}

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

private:
  AstVisitor& m_visitor;
};

#endif
