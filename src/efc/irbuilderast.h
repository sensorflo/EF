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

  int jitExecFunction(llvm::Function* function);
  int jitExecFunction1Arg(llvm::Function* function, int arg1);
  int jitExecFunction2Arg(llvm::Function* function, int arg1, int arg2);
  int jitExecFunction(const std::string& name);
  int jitExecFunction1Arg(const std::string& name, int arg1);
  int jitExecFunction2Arg(const std::string& name, int arg1, int arg2);

  void visit(const AstSeq& seq, Place place, int childNo);
  void visit(const AstCtList& ctList) {};
  void visit(const AstOperator& op);
  void visit(const AstNumber& number);
  void visit(const AstSymbol& symbol);
  void visit(const AstFunCall& funCall);
  void visit(const AstFunDef& funDef, Place place);
  void visit(const AstFunDecl& funDecl);
  void visit(const AstDataDecl& dataDecl) {};
  void visit(const AstDataDef& dataDef) {};

  llvm::Value* valuesBackAndPop();
  llvm::Function* valuesBackToFunction();
  llvm::AllocaInst* createEntryBlockAlloca(llvm::Function* functionIr,
    const std::string& varName);

  std::list<llvm::Value*> m_values;
  llvm::IRBuilder<> m_builder;
  /** m_executionEngine is the owner */
  llvm::Module* m_module;
  std::string m_errStr;
  /** We're the owner */
  llvm::ExecutionEngine* m_executionEngine;
  llvm::Function* m_mainFunction;
  llvm::BasicBlock* m_mainBasicBlock;
  std::map<std::string, llvm::Value*> m_symbolTable;
};
