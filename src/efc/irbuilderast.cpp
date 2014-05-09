#include "irbuilderast.h"
#include "llvm/ExecutionEngine/JIT.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include <algorithm>
#include <cstdlib>
using namespace std;
using namespace llvm;

void IrBuilderAst::staticOneTimeInit() {
  InitializeNativeTarget();
}

IrBuilderAst::IrBuilderAst() :
  m_builder(getGlobalContext()),
  m_module(new Module("Main", getGlobalContext())),
  m_executionEngine(EngineBuilder(m_module).setErrorStr(&m_errStr).create()),
  m_mainFunction(NULL),
  m_mainBasicBlock(NULL) {

  assert(m_executionEngine);

  vector<Type*> argsIr(0);
  Type* retTypeIr = Type::getInt32Ty(getGlobalContext());
  FunctionType* functionTypeIr = FunctionType::get( retTypeIr, argsIr, false);
  m_mainFunction = Function::Create( functionTypeIr,
    Function::ExternalLinkage, "main", m_module );

  m_mainBasicBlock = BasicBlock::Create(getGlobalContext(), "entry",
    m_mainFunction);
  assert(m_mainBasicBlock);
  m_builder.SetInsertPoint(m_mainBasicBlock);
}

IrBuilderAst::~IrBuilderAst() {
  if (m_executionEngine) { delete m_executionEngine; }
}

void IrBuilderAst::buildModule(const AstSeq& seq) {
  // this kicks off ast traversal
  seq.accept(*this);

  // The single last value remaining in m_values is the return value of the
  // implicit main method.
  assert(!m_values.empty());
  m_builder.CreateRet(m_values.back());
  m_values.pop_back();
  assert(m_values.empty()); // Ensure that it was really the last value

  verifyFunction(*m_mainFunction);
}

int IrBuilderAst::buildAndRunModule(const AstSeq& seq) {
  buildModule(seq);
  return execMain();
}

int IrBuilderAst::execMain() {
  void* mainFunctionVoidPtr =
    m_executionEngine->getPointerToFunction(m_mainFunction);
  assert(mainFunctionVoidPtr);
  int (*mainFunction)() = (int (*)())(intptr_t)mainFunctionVoidPtr;
  assert(mainFunction);
  return mainFunction();
}

void IrBuilderAst::visit(const AstSeq&, Place place, int childNo) {
  if (place==ePreOrder || childNo<2 ) return;
  // remove 2nd last value
  std::list<llvm::Value*>::iterator it = m_values.end();
  std::advance(it, -2);
  m_values.erase( it );
}

void IrBuilderAst::visit(const AstOperator& op) {
  Value* result = NULL;
  // order of visit childs = order of push: lhs, rhs
  // ie top=rhs, top-1=lhs
  assert(!m_values.empty());
  Value* rhs = m_values.back(); m_values.pop_back();
  assert(!m_values.empty());
  Value* lhs = m_values.back(); m_values.pop_back();
  switch (op.op()) {
  case '-': result = m_builder.CreateSub(lhs, rhs, "subtmp"); break;
  case '+': result = m_builder.CreateAdd(lhs, rhs, "addtmp"); break;
  case '*': result = m_builder.CreateMul(lhs, rhs, "multmp"); break;
  case '/': result = m_builder.CreateSDiv(lhs, rhs, "divtmp"); break;
  default: break;
  }
  m_values.push_back(result);
}

void IrBuilderAst::visit(const AstNumber& number) {
  Value* valueIr = ConstantInt::get( getGlobalContext(),
    APInt(32, number.value()));
  m_values.push_back(valueIr);
}

void IrBuilderAst::visit(const AstFunDecl& funDecl) {
  // currently always returns type int
  Type* retTypeIr = Type::getInt32Ty(getGlobalContext());
  // currently always zero parameters
  FunctionType* functionTypeIr = FunctionType::get(retTypeIr, false);
  assert(functionTypeIr);
  Function* functionIr = Function::Create( functionTypeIr,
    Function::ExternalLinkage, funDecl.name(), m_module );
  assert(functionIr);
  if ( functionIr->getName() != funDecl.name() ) {
    functionIr->eraseFromParent();
    functionIr = m_module->getFunction(funDecl.name());
    assert(functionIr->arg_size() == 0);
  }

  // push dummy value to m_values stack, so sequence operator has something to
  // remove. This dummy push can hopefully be removed as soon as metatypes are
  // introduced.
  m_values.push_back(ConstantInt::get( getGlobalContext(), APInt(32, 0)));
}
