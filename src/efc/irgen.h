#pragma once
#include "access.h"
#include "astforwards.h"
#include "astvisitor.h"
#include "objtype.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"

#include <memory>
#include <stack>
#include <string>

namespace llvm {
class Module;
class BasicBlock;
}
class ErrorHandler;

/** IR Generator -- Generates LLVM intermediate representation from a given
AST. */
class IrGen : private AstVisitor {
public:
  static void staticOneTimeInit();
  IrGen(ErrorHandler& errorHandler);

  std::unique_ptr<llvm::Module> genIr(AstNode& root);

private:
  friend class TestingIrGen;

  void visit(AstNop& nop) override;
  void visit(AstBlock& block) override;
  void visit(AstCast& cast) override;
  void visit(AstCtList& ctList) override;
  void visit(AstOperator& op) override;
  void visit(AstSeq& seq) override;
  void visit(AstNumber& number) override;
  void visit(AstSymbol& symbol) override;
  void visit(AstFunCall& funCall) override;
  void visit(AstFunDef& funDef) override;
  void visit(AstDataDef& dataDef) override;
  void visit(AstIf& if_) override;
  void visit(AstLoop& loop) override;
  void visit(AstReturn& return_) override;
  void visit(AstObjTypeSymbol& symbol) override;
  void visit(AstObjTypeQuali& quali) override;
  void visit(AstObjTypePtr& ptr) override;
  void visit(AstClassDef& class_) override;

  llvm::Value* callAcceptOn(AstObject&);

  void allocateAndInitLocalIrObjectFor(AstObject& astObject,
    llvm::Value* irInitializer, const std::string& name = "");
  llvm::AllocaInst* createAllocaInEntryBlock(
    llvm::Function* functionIr, const std::string& varName, llvm::Type* type);

  llvm::IRBuilder<> m_builder;
  /** Is non-null during execution of IrGen, that is practically 'always'
  from the view point of member functions. */
  std::unique_ptr<llvm::Module> m_module;
  std::stack<llvm::BasicBlock*> m_BasicBlockStack;
  ErrorHandler& m_errorHandler;
  /** For abstract obj types like void or noreturn. Contrast this with nullptr
  which means '(accidentaly) not (yet) set)'. */
  static llvm::Value* const m_abstractObject;
};

extern llvm::LLVMContext llvmContext;
