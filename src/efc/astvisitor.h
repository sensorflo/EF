#pragma once
#include "astforwards.h"

class AstVisitor {
public:
  virtual ~AstVisitor() {};
  virtual void visit(AstNop& nop) =0;
  virtual void visit(AstBlock& block) =0;
  virtual void visit(AstCast& cast) =0;
  virtual void visit(AstCtList& ctList) =0;
  virtual void visit(AstOperator& op) =0;
  virtual void visit(AstNumber& number) =0;
  virtual void visit(AstSymbol& symbol) =0;
  virtual void visit(AstFunCall& funCall) =0;
  virtual void visit(AstFunDef& funDef) =0;
  virtual void visit(AstFunDecl& funDecl) =0;
  virtual void visit(AstDataDecl& dataDecl) =0;
  virtual void visit(AstArgDecl& argDecl) =0;
  virtual void visit(AstDataDef& dataDef) =0;
  virtual void visit(AstIf& if_) =0;
  virtual void visit(AstLoop& loop) =0;
  virtual void visit(AstReturn& return_) =0;
};

class AstConstVisitor {
public:
  virtual ~AstConstVisitor() {};
  virtual void visit(const AstNop& nop) =0;
  virtual void visit(const AstBlock& block) =0;
  virtual void visit(const AstCast& cast) =0;
  virtual void visit(const AstCtList& ctList) =0;
  virtual void visit(const AstOperator& op) =0;
  virtual void visit(const AstNumber& number) =0;
  virtual void visit(const AstSymbol& symbol) =0;
  virtual void visit(const AstFunCall& funCall) =0;
  virtual void visit(const AstFunDef& funDef) =0;
  virtual void visit(const AstFunDecl& funDecl) =0;
  virtual void visit(const AstDataDecl& dataDecl) =0;
  virtual void visit(const AstArgDecl& argDecl) =0;
  virtual void visit(const AstDataDef& dataDef) =0;
  virtual void visit(const AstIf& if_) =0;
  virtual void visit(const AstLoop& loop) =0;
  virtual void visit(const AstReturn& return_) =0;
};
