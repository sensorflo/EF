#include "irgenforwarddeclarator.h"
#include "object.h"
#include "llvm/IR/Module.h"
#include <vector>
using namespace std;
using namespace llvm;

IrGenForwardDeclarator::IrGenForwardDeclarator(ErrorHandler& errorHandler,
  llvm::Module& module) :
  m_module{module}  {
}

void IrGenForwardDeclarator::operator()(AstNode& root) {
  root.accept(*this);
}

void IrGenForwardDeclarator::visit(AstDataDef& dataDef) {
  AstDefaultIterator::visit(dataDef);

  const auto object = dataDef.object();
  assert(object);
  if ( object->storageDuration() == StorageDuration::eStatic ) {
    const auto addr = new GlobalVariable(m_module,
      object->objType().llvmType(),
      !(object->objType().qualifiers() & ObjType::eMutable),
      GlobalValue::InternalLinkage, nullptr, dataDef.fqName());
    object->setAddrOfIrObject(addr);
  }
}

void IrGenForwardDeclarator::visit(AstFunDef& funDef) {
  AstDefaultIterator::visit(funDef);

  // create IR function with given name and signature
  vector<Type*> llvmArgs;
  for ( const auto& astArg : funDef.declaredArgs() ) {
    llvmArgs.push_back(astArg->objType().llvmType());
  }
  auto llvmFunctionType = FunctionType::get(
    funDef.declaredRetAstObjType().objType().llvmType(), llvmArgs, false);
  auto functionIr = Function::Create( llvmFunctionType,
    Function::ExternalLinkage, funDef.fqName(), &m_module);
  assert(functionIr);

  // If the names differ that means a function with that name already existed,
  // so LLVM automaticaly chose a new name. That cannot be since above our
  // environment said the name is unique.
  assert(functionIr->getName() == funDef.fqName());

  funDef.object()->setAddrOfIrObject(functionIr);
}
