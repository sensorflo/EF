#ifndef ASTVISITOR_H
#define ASTVISITOR_H

class AstNode;
class AstNumber;
class AstSymbol;
class AstFunCall;
class AstOperator;
class AstCast;
class AstCtList;
class AstFunDef;
class AstFunDecl;
class AstDataDecl;
class AstArgDecl;
class AstDataDef;
class AstIf;


class AstVisitor {
public:
  virtual ~AstVisitor() {};
  virtual void visit(AstCast& cast) =0;
  virtual void visit(AstCtList& ctList) =0;
  virtual void visit(AstOperator& op) =0;
  virtual void visit(AstNumber& number) =0;
  virtual void visit(AstSymbol& symbol) =0;
  virtual void visit(AstFunCall& funCall) =0;
  virtual void visit(AstFunDef& funDef) =0;
  virtual void visit(AstFunDecl& funDecl) =0;
  virtual void visit(AstDataDecl& dataDecl) =0;
  virtual void visit(AstDataDef& dataDef) =0;
  virtual void visit(AstIf& if_) =0;
};

#endif
