#pragma once
#include "astvisitor.h"

/** Traverses AST: at each node, call its accept method and pass the vistor
given in the ctor. Currently it's only pre order traversal. */
class AstDefaultIterator : public AstVisitor {
public:
  AstDefaultIterator(AstVisitor* visitor = nullptr) : m_visitor(visitor) {}

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

private:
  AstVisitor* m_visitor;
};
