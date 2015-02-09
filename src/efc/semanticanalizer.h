#ifndef SEMANTICANALIZER_H
#define SEMANTICANALIZER_H
#include "astvisitor.h"
#include <cstddef>

class ErrorHandler;

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
  //ErrorHandler& m_errorHandler;
  AstVisitor& m_nextVisitor;
  bool m_ownsNextVisitor;
};

#endif
