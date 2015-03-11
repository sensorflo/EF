#ifndef IR_GEN_H
#define IR_GEN_H
#include "objtype.h"
#include "access.h"
#include "astvisitor.h"
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


/** IR Generator -- Generates (aka build) LLVM's intermediate representation
from a given AST.*/
class IrGen : private AstVisitor  {
public:
  static void staticOneTimeInit();
  IrGen(ErrorHandler& errorHandler);
  virtual ~IrGen();

  void genIr(AstNode& root);
  void genIrInImplicitMain(AstValue& root);
  int jitExecMain();

private:
  friend class TestingIrGen;

  virtual void visit(AstCast& cast);
  virtual void visit(AstCtList& ctList);
  virtual void visit(AstOperator& op);
  virtual void visit(AstNumber& number);
  virtual void visit(AstSymbol& symbol);
  virtual void visit(AstFunCall& funCall);
  virtual void visit(AstFunDef& funDef);
  virtual void visit(AstFunDecl& funDecl);
  virtual void visit(AstDataDecl& dataDecl);
  virtual void visit(AstArgDecl& argDecl);
  virtual void visit(AstDataDef& dataDef);
  virtual void visit(AstIf& if_);

  int jitExecFunction(llvm::Function* function);
  int jitExecFunction1Arg(llvm::Function* function, int arg1);
  int jitExecFunction2Arg(llvm::Function* function, int arg1, int arg2);
  int jitExecFunction(const std::string& name);
  int jitExecFunction1Arg(const std::string& name, int arg1);
  int jitExecFunction2Arg(const std::string& name, int arg1, int arg2);

  llvm::Value* callAcceptOn(AstNode&);

  llvm::AllocaInst* createAllocaInEntryBlock(llvm::Function* functionIr,
    const std::string& varName);

  llvm::IRBuilder<> m_builder;
  /** m_executionEngine is the owner */
  llvm::Module* m_module;
  std::string m_errStr;
  /** We're the owner */
  llvm::ExecutionEngine* m_executionEngine;
  llvm::Function* m_mainFunction;
  std::stack<llvm::BasicBlock*> m_BasicBlockStack;
  ErrorHandler& m_errorHandler;
  /** We're not the owner, guaranteed to be non-null. */
  AstVisitor* m_enclosingVisitor;
};

#endif
