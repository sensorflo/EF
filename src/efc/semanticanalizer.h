#pragma once
#include "access.h"
#include "astvisitor.h"

#include <cstddef>
#include <stack>

class Env;
class ErrorHandler;
class ObjType;

/** Does semenatic analysis by inserting AST nodes where needed or reporting
errors via the ErrorHandler.  Some of these responsibilities were already done
by GenParserExt, see there, and some responsibilities are delegated to others,
see analyze(AstNode& root). */
class SemanticAnalizer : private AstVisitor {
public:
  SemanticAnalizer(Env& env, ErrorHandler& errorHandler);
  void analyze(AstNode& root);

private:
  class FunBodyHelper {
  public:
    FunBodyHelper(std::stack<const AstObjType*>& funRetAstObjTypes,
      const AstObjType* objType);
    ~FunBodyHelper();

  private:
    std::stack<const AstObjType*>& m_funRetAstObjTypes;
  };

  void visit(AstNop& nop) override;
  void visit(AstBlock& block) override;
  void visit(AstCast& cast) override;
  void visit(AstCtList& ctList) override;
  void visit(AstOperator& op) override;
  void visit(AstSeq& seq) override;
  void visit(AstNumber& number) override;
  void visit(AstSymbol& symbol) override;
  void visit(AstFunCall& funCall) override;
  void visit(AstFunDef& funDef) override;
  void visit(AstDataDef& dataDef) override;
  void visit(AstIf& if_) override;
  void visit(AstLoop& loop) override;
  void visit(AstReturn& return_) override;
  void visit(AstObjTypeSymbol& symbol) override;
  void visit(AstObjTypeQuali& quali) override;
  void visit(AstObjTypePtr& ptr) override;
  void visit(AstClassDef& class_) override;

  void preConditionCheck(const AstObject& node);
  void preConditionCheck(const AstObjType& node);
  void postConditionCheck(const AstObject& node);
  void postConditionCheck(const AstObjType& node);

  void setAccessAndCallAcceptOn(AstNode& node, Access access);

  friend class TestingSemanticAnalizer;

  Env& m_env;
  ErrorHandler& m_errorHandler;
  std::stack<const AstObjType*> m_funRetAstObjTypes;
};
