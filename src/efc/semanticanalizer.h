#pragma once
#include "astvisitor.h"
#include <cstddef>
#include <stack>

class Env;
class ErrorHandler;
class ObjType;

/** Does semenatic analysis by inserting AST nodes where needed or reporting
errors via the ErrorHandler.  Some of these responsibilities were already done
by PerserExt, see there. */
class SemanticAnalizer : private AstVisitor {
public:
  SemanticAnalizer(Env& env, ErrorHandler& errorHandler);
  void analyze(AstNode& root);

private:
  virtual void visit(AstNop& nop);
  virtual void visit(AstBlock& block);
  virtual void visit(AstCast& cast);
  virtual void visit(AstCtList& ctList);
  virtual void visit(AstOperator& op);
  virtual void visit(AstNumber& number);
  virtual void visit(AstSymbol& symbol);
  virtual void visit(AstFunCall& funCall);
  virtual void visit(AstFunDef& funDef);
  virtual void visit(AstDataDecl& dataDecl);
  virtual void visit(AstArgDecl& argDecl);
  virtual void visit(AstDataDef& dataDef);
  virtual void visit(AstIf& if_);
  virtual void visit(AstLoop& loop);
  virtual void visit(AstReturn& return_);

  void callAcceptWithinNewScope(AstValue& node);
  void postConditionCheck(const AstValue& node);

  friend class TestingSemanticAnalizer;

  Env& m_env;
  ErrorHandler& m_errorHandler;
  std::stack<const ObjType*> m_funRetObjTypes;
};
