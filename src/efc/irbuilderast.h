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
  void buildModuleNoImplicitMain(const AstNode& root);
  void buildModule(const AstValue& root);
  int runModule();
  int buildAndRunModule(const AstValue& root);
  
  llvm::Value*    visit(const AstCast& cast, Access access = eRead);
  llvm::Value*    visit(const AstOperator& op, Access access = eRead);
  llvm::Value*    visit(const AstNumber& number, Access access = eRead);
  llvm::Value*    visit(const AstSymbol& symbol, Access access = eRead);
  llvm::Value*    visit(const AstFunCall& funCall, Access access = eRead);
  llvm::Function* visit(const AstFunDef& funDef);
  llvm::Function* visit(const AstFunDecl& funDecl);
  llvm::Value*    visit(const AstDataDecl& dataDecl, Access access = eRead);
  llvm::Value*    visit(const AstDataDef& dataDef, Access access = eRead);
  llvm::Value*    visit(const AstIf& if_, Access access = eRead);

private:
  friend class TestingIrBuilderAst;
  
  int jitExecFunction(llvm::Function* function);
  int jitExecFunction1Arg(llvm::Function* function, int arg1);
  int jitExecFunction2Arg(llvm::Function* function, int arg1, int arg2);
  int jitExecFunction(const std::string& name);
  int jitExecFunction1Arg(const std::string& name, int arg1);
  int jitExecFunction2Arg(const std::string& name, int arg1, int arg2);

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
