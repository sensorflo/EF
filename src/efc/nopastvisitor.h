#pragma once
#include "astvisitor.h"

/** AstVisitor doing nothing. Intended to be a conenience helper to derive
from when one only implements a few of the visit function overloads. */
class NopAstVisitor : public AstVisitor {
public:
  void visit(AstNop& nop) override{};
  void visit(AstBlock& block) override{};
  void visit(AstCast& cast) override{};
  void visit(AstCtList& ctList) override{};
  void visit(AstOperator& op) override{};
  void visit(AstSeq& seq) override{};
  void visit(AstNumber& number) override{};
  void visit(AstSymbol& symbol) override{};
  void visit(AstFunCall& funCall) override{};
  void visit(AstFunDef& funDef) override{};
  void visit(AstDataDef& dataDef) override{};
  void visit(AstIf& if_) override{};
  void visit(AstLoop& loop) override{};
  void visit(AstReturn& return_) override{};
  void visit(AstObjTypeSymbol& symbol) override{};
  void visit(AstObjTypeQuali& quali) override{};
  void visit(AstObjTypePtr& ptr) override{};
  void visit(AstClassDef& class_) override{};

protected:
  NopAstVisitor() = default;
};

/** Analogous to NopAstVisitor */
class NopAstConstVisitor : public AstConstVisitor {
public:
  void visit(const AstNop& nop) override{};
  void visit(const AstBlock& block) override{};
  void visit(const AstCast& cast) override{};
  void visit(const AstCtList& ctList) override{};
  void visit(const AstOperator& op) override{};
  void visit(const AstSeq& seq) override{};
  void visit(const AstNumber& number) override{};
  void visit(const AstSymbol& symbol) override{};
  void visit(const AstFunCall& funCall) override{};
  void visit(const AstFunDef& funDef) override{};
  void visit(const AstDataDef& dataDef) override{};
  void visit(const AstIf& if_) override{};
  void visit(const AstLoop& loop) override{};
  void visit(const AstReturn& return_) override{};
  void visit(const AstObjTypeSymbol& symbol) override{};
  void visit(const AstObjTypeQuali& quali) override{};
  void visit(const AstObjTypePtr& ptr) override{};
  void visit(const AstClassDef& class_) override{};

protected:
  NopAstConstVisitor() = default;
};
