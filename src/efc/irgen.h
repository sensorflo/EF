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
from a given AST.

Is implemented using the vistor pattern.  Processing an AST node is in general
an interleaved mixture between generating IR code for the current node and
recursively descending into all child nodes.  Descinding to a child node works
by calling accept (visitor pattern) on the child note and passing the
'enclosed visitor'.  The enclosed visitor can process the (child) node first
and is then obliged to forward the call to IrGen.  For production, the
enclosed visitor is intended to be the semantic analyzer. */
class IrGen : public AstVisitor  {
public:
  static void staticOneTimeInit();
  IrGen(Env& env, ErrorHandler& errorHandler);
  virtual ~IrGen();

  void genIr(AstNode& root, AstVisitor* enclosingVisitor = NULL);
  void genIrInImplicitMain(AstValue& root, AstVisitor* enclosingVisitor = NULL);
  int jitExecMain();

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

private:
  friend class TestingIrGen;
  
  llvm::Value* genIrInternal(AstNode& root, AstVisitor* enclosingVisitor = NULL);

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
  Env& m_env;
  ErrorHandler& m_errorHandler;
  /** We're not the owner, guaranteed to be non-null. */
  AstVisitor* m_enclosingVisitor;
};

#endif
