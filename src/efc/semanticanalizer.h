#ifndef SEMANTICANALIZER_H
#define SEMANTICANALIZER_H
#include "astvisitor.h"
#include <cstddef>

class ErrorHandler;

/** Does semenatic analysis by inserting AST nodes where needed or reporting
errors via the ErrorHandler.

Is implemented via the visitor pattern.  Additionaly, after visiting each
node, forwards the visit call to the 'next visitor'.  That next visitor is
responsible to descent down to child nodes, i.e. call accept on child nodes.
For production, the next visitor is intended to be an IR generator. */
class SemanticAnalizer : public AstVisitor {
public:
  SemanticAnalizer(ErrorHandler& errorHandler, AstVisitor* nextVisitor = NULL);
  virtual ~SemanticAnalizer();

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
  friend class TestingSemanticAnalizer;

  ErrorHandler& m_errorHandler;
  AstVisitor& m_nextVisitor;
  bool m_ownsNextVisitor;
};

#endif
