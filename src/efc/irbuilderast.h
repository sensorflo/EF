#include "ast.h"

class IrBuilderAst : public AstVisitor {
public:
  int buildAndRunModule(const AstSeq& seq);

private:
  void visit(const AstSeq& seq);
  void visit(const AstOperator& op);
  void visit(const AstNumber& number);
  int m_lastValueInSeq;
};
