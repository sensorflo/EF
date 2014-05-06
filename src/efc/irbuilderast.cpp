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
  m_mainFunction(NULL) {

  assert(m_executionEngine);

  vector<Type*> argsIr(0);
  Type* retTypeIr = Type::getInt32Ty(getGlobalContext());
  FunctionType* functionTypeIr = FunctionType::get( retTypeIr, argsIr, false);
  m_mainFunction = Function::Create( functionTypeIr,
    Function::ExternalLinkage, "main", m_module );

  BasicBlock* bb = BasicBlock::Create(getGlobalContext(), "entry",
    m_mainFunction);
  m_builder.SetInsertPoint(bb);
}

IrBuilderAst::~IrBuilderAst() {
  if (m_executionEngine) { delete m_executionEngine; }
}

int IrBuilderAst::buildAndRunModule(const AstSeq& seq) {
  seq.accept(*this);

  assert(!m_valueStack.empty());
  Value* retValIr = m_valueStack.top();
  stack<Value*> empty; swap( m_valueStack, empty ); // clears the m_valueStack

  m_builder.CreateRet(retValIr);
  verifyFunction(*m_mainFunction);

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

void IrBuilderAst::visit(const AstSeq& /*seq*/) {
  //nop
}

void IrBuilderAst::visit(const AstOperator& op) {
  Value* result = NULL;
  // order of visit childs = order of push: lhs, rhs
  // ie top=rhs, top-1=lhs
  assert(!m_valueStack.empty());
  Value* rhs = m_valueStack.top(); m_valueStack.pop();
  assert(!m_valueStack.empty());
  Value* lhs = m_valueStack.top(); m_valueStack.pop();
  switch (op.op()) {
  case '-': result = m_builder.CreateSub(lhs, rhs, "subtmp"); break;
  case '+': result = m_builder.CreateAdd(lhs, rhs, "addtmp"); break;
  case '*': result = m_builder.CreateMul(lhs, rhs, "multmp"); break;
  case '/': result = m_builder.CreateSDiv(lhs, rhs, "divtmp"); break;
  default: break;
  }
  m_valueStack.push(result);
}

void IrBuilderAst::visit(const AstNumber& number) {
  llvm::Value* valueIr = ConstantInt::get( getGlobalContext(),
    APInt(32, number.value()));
  m_valueStack.push(valueIr);
}
