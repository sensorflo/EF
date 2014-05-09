#include "ast.h"
#include "llvm/IR/IRBuilder.h"
#include <stack>
namespace llvm {
  class Module;
  class ExecutionEngine;
  class BasicBlock;
}

class IrBuilderAst : public AstVisitor {
public:
  static void staticOneTimeInit();
  IrBuilderAst();
  virtual ~IrBuilderAst();
  void buildModule(const AstSeq& seq);
  int buildAndRunModule(const AstSeq& seq);

private:
  friend class TestingIrBuilderAst;

  int execMain();

  void visit(const AstSeq& seq);
  void visit(const AstCtList& ctList) {};
  void visit(const AstOperator& op);
  void visit(const AstNumber& number);
  void visit(const AstFunCall& funCall) {};
  void visit(const AstFunDef& funDef, Place place) {};
  void visit(const AstFunDecl& funDecl);

  std::list<llvm::Value*> m_values;
  llvm::IRBuilder<> m_builder;
  /** m_executionEngine is the owner */
  llvm::Module* m_module;
  std::string m_errStr;
  /** We're the owner */
  llvm::ExecutionEngine* m_executionEngine;
  llvm::Function* m_mainFunction;
  llvm::BasicBlock* m_mainBasicBlock;
};
