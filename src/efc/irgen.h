#pragma once
#include "objtype.h"
#include "access.h"
#include "astvisitor.h"
#include "llvm/IR/IRBuilder.h"
#include <stack>
#include <string>
#include <memory>

#include "astforwards.h"
namespace llvm {
  class Module;
  class BasicBlock;
}
class Env;
class SymbolTableEntry;
class ErrorHandler;

/** IR Generator -- Generates LLVM intermediate representation from a given
AST. */
class IrGen : private AstVisitor  {
public:
  static void staticOneTimeInit();
  IrGen(ErrorHandler& errorHandler);

  std::unique_ptr<llvm::Module> genIr(AstNode& root);

private:
  friend class TestingIrGen;

  virtual void visit(AstNop& nop);
  virtual void visit(AstBlock& block);
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
  virtual void visit(AstLoop& loop);
  virtual void visit(AstReturn& return_);

  llvm::Value* callAcceptOn(AstValue&);

  llvm::AllocaInst* createAllocaInEntryBlock(llvm::Function* functionIr,
    const std::string& varName, llvm::Type* type);

  llvm::IRBuilder<> m_builder;
  /** Is non-null during execution of IrGen, that is practically 'always'
  from the view point of member functions. */
  std::unique_ptr<llvm::Module> m_module;
  std::stack<llvm::BasicBlock*> m_BasicBlockStack;
  ErrorHandler& m_errorHandler;
  /** We're not the owner, guaranteed to be non-null. */
  AstVisitor* m_enclosingVisitor;
  /** For abstract obj types like void or noreturn. Contrast this with nullptr
  which means '(accidentaly) not (yet) set)'. */
  static llvm::Value* const m_abstractObject;
};
