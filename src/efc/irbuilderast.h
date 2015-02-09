#ifndef IR_BUILDER_AST_H
#define IR_BUILDER_AST_H
#include "objtype.h"
#include "access.h"
#include "llvm/IR/IRBuilder.h"
#include <stack>

#include "astforwards.h"
namespace llvm {
  class Module;
  class ExecutionEngine;
  class BasicBlock;
}
class Env;
class SymbolTableEntry;
class ErrorHandler;


class IrBuilderAst {
public:
  static void staticOneTimeInit();
  IrBuilderAst(Env& env, ErrorHandler& errorHandler);
  virtual ~IrBuilderAst();
  void buildModuleNoImplicitMain(AstNode& root);
  void buildModule(AstValue& root);
  int runModule();
  int buildAndRunModule(AstValue& root);
  
  void visit(AstCast& cast);
  void visit(AstOperator& op);
  void visit(AstNumber& number);
  void visit(AstSymbol& symbol);
  void visit(AstFunCall& funCall);
  void visit(AstFunDef& funDef);
  void visit(AstFunDecl& funDecl);
  void visit(AstDataDecl& dataDecl);
  void visit(AstDataDef& dataDef);
  void visit(AstIf& if_);

private:
  friend class TestingIrBuilderAst;
  
  int jitExecFunction(llvm::Function* function);
  int jitExecFunction1Arg(llvm::Function* function, int arg1);
  int jitExecFunction2Arg(llvm::Function* function, int arg1, int arg2);
  int jitExecFunction(const std::string& name);
  int jitExecFunction1Arg(const std::string& name, int arg1);
  int jitExecFunction2Arg(const std::string& name, int arg1, int arg2);

  llvm::Value* callAcceptOn(AstNode&);

  llvm::AllocaInst* createEntryBlockAlloca(llvm::Function* functionIr,
    const std::string& varName);

  llvm::IRBuilder<> m_builder;
  /** m_executionEngine is the owner */
  llvm::Module* m_module;
  std::string m_errStr;
  /** We're the owner */
  llvm::ExecutionEngine* m_executionEngine;
  llvm::Function* m_mainFunction;
  std::stack<llvm::BasicBlock*> m_BasicBlockStack;
  Env& m_env;
  ErrorHandler& m_errorHandler;
};

#endif
