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
  static std::basic_ostream<char>& printTo(
    const AstNode& root, std::basic_ostream<char>& os);

private:
  void visit(const AstNop& nop) override;
  void visit(const AstBlock& block) override;
  void visit(const AstCast& cast) override;
  void visit(const AstCtList& ctList) override;
  void visit(const AstOperator& op) override;
  void visit(const AstSeq& seq) override;
  void visit(const AstNumber& number) override;
  void visit(const AstSymbol& symbol) override;
  void visit(const AstFunCall& funCall) override;
  void visit(const AstFunDef& funDef) override;
  void visit(const AstDataDef& dataDef) override;
  void visit(const AstIf& if_) override;
  void visit(const AstLoop& loop) override;
  void visit(const AstReturn& return_) override;
  void visit(const AstObjTypeSymbol& symbol) override;
  void visit(const AstObjTypeQuali& quali) override;
  void visit(const AstObjTypePtr& ptr) override;
  void visit(const AstClassDef& class_) override;

  std::basic_ostream<char>& m_os;
};
