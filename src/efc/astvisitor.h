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


class AstConstVisitor {
public:
  virtual ~AstConstVisitor() {};
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
};

#endif
