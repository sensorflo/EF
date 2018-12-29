#pragma once
#include "astdefaultiterator.h"

#include "astforwards.h"
namespace llvm {
class Module;
}
class ErrorHandler;

/** For each AST node representing an non-local object (data or function)
definition, create an respective LLVM value, and set the IrAddr of the Object
associated to the AST node to the new entity.

  ASTNode ---->  Object --------> new LLVM entity
  being an                 IrAddr
  definition
*/
class IrGenForwardDeclarator : private AstDefaultIterator {
public:
  IrGenForwardDeclarator(ErrorHandler& errorHandler, llvm::Module& module);

  void operator()(AstNode& root);

private:
  virtual void visit(AstDataDef& dataDef);
  virtual void visit(AstFunDef& funDef);

  llvm::Module& m_module;
};
