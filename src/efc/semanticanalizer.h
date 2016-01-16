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
  class FunBodyHelper {
  public:
    FunBodyHelper(std::stack<const ObjType*>& funRetObjTypes,
      const ObjType* objType);
    ~FunBodyHelper();
  private:
    std::stack<const ObjType*>& m_funRetObjTypes;
  };

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

  void postConditionCheck(const AstObject& node);
  void postConditionCheck(const AstObjType& node);

  friend class TestingSemanticAnalizer;

  Env& m_env;
  ErrorHandler& m_errorHandler;
  std::stack<const ObjType*> m_funRetObjTypes;
};
