#include "ast.h"
#include "llvm/IR/IRBuilder.h"
#include <stack>
namespace llvm {
  class Module;
  class ExecutionEngine;
}

class IrBuilderAst : public AstVisitor {
public:
  static void staticOneTimeInit();
  IrBuilderAst();
  virtual ~IrBuilderAst();
  int buildAndRunModule(const AstSeq& seq);

private:
  int execMain();

  void visit(const AstSeq& seq);
  void visit(const AstCtList& ctList) {};
  void visit(const AstOperator& op);
  void visit(const AstNumber& number);
  void visit(const AstFunCall& funCall) {};
  void visit(const AstFunDef& funDef) {};
  void visit(const AstFunDecl& funDecl) {};

  std::stack<llvm::Value*> m_valueStack;
  llvm::IRBuilder<> m_builder;
  /** m_executionEngine is the owner */
  llvm::Module* m_module;
  std::string m_errStr;
  /** We're the owner */
  llvm::ExecutionEngine* m_executionEngine;
  llvm::Function* m_mainFunction;
};
