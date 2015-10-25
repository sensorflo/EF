#pragma once
#include "astvisitor.h"
#include <iostream>

/** Prints the AST in its canonical form. Note purpose is to have an exact
representation of the AST. Thus no information can thus be dropped. For
example one could _mistakenly_ think that for the readers' convenience
AstBlock should only print its body but not itself, or that a AstSeq with only
one elements should only print that but not itself.*/
class AstPrinter : private AstConstVisitor {
public:
  AstPrinter(std::basic_ostream<char>& os);

  static std::string toStr(const AstNode& root);
  static std::basic_ostream<char>& printTo(const AstNode& root,
    std::basic_ostream<char>& os);

private:
  virtual void visit(const AstNop& nop);
  virtual void visit(const AstBlock& block);
  virtual void visit(const AstCast& cast);
  virtual void visit(const AstCtList& ctList);
  virtual void visit(const AstOperator& op);
  virtual void visit(const AstSeq& seq);
  virtual void visit(const AstNumber& number);
  virtual void visit(const AstSymbol& symbol);
  virtual void visit(const AstFunCall& funCall);
  virtual void visit(const AstFunDef& funDef);
  virtual void visit(const AstDataDef& dataDef);
  virtual void visit(const AstIf& if_);
  virtual void visit(const AstLoop& loop);
  virtual void visit(const AstReturn& return_);
  virtual void visit(const AstClass& class_);
  
  std::basic_ostream<char>& m_os;
};
