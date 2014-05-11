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
#include <stdexcept>
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

  assert(m_module);
  assert(m_executionEngine);

  vector<Type*> argsIr(0);
  Type* retTypeIr = Type::getInt32Ty(getGlobalContext());
  FunctionType* functionTypeIr = FunctionType::get( retTypeIr, argsIr, false);
  m_mainFunction = Function::Create( functionTypeIr,
    Function::ExternalLinkage, "main", m_module );
  assert(m_mainFunction);

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
  m_builder.CreateRet(valuesBackAndPop());
  assert(m_values.empty()); // Ensure that it was really the last value

  verifyFunction(*m_mainFunction);
}

int IrBuilderAst::buildAndRunModule(const AstSeq& seq) {
  buildModule(seq);
  return jitExecFunction(m_mainFunction);
}

int IrBuilderAst::jitExecFunction(const std::string& name) {
  return jitExecFunction(m_module->getFunction(name));
}

int IrBuilderAst::jitExecFunction1Arg(const std::string& name, int arg1) {
  return jitExecFunction1Arg(m_module->getFunction(name), arg1);
}

int IrBuilderAst::jitExecFunction2Arg(const std::string& name, int arg1, int arg2) {
  return jitExecFunction2Arg(m_module->getFunction(name), arg1, arg2);
}

int IrBuilderAst::jitExecFunction(llvm::Function* function) {
  void* functionVoidPtr =
    m_executionEngine->getPointerToFunction(function);
  assert(functionVoidPtr);
  int (*functionPtr)() = (int (*)())(intptr_t)functionVoidPtr;
  assert(functionPtr);
  return functionPtr();
}

int IrBuilderAst::jitExecFunction1Arg(llvm::Function* function, int arg1) {
  void* functionVoidPtr =
    m_executionEngine->getPointerToFunction(function);
  assert(functionVoidPtr);
  int (*functionPtr)(int) = (int (*)(int))(intptr_t)functionVoidPtr;
  assert(functionPtr);
  return functionPtr(arg1);
}

int IrBuilderAst::jitExecFunction2Arg(llvm::Function* function, int arg1, int arg2) {
  void* functionVoidPtr =
    m_executionEngine->getPointerToFunction(function);
  assert(functionVoidPtr);
  int (*functionPtr)(int, int) = (int (*)(int, int))(intptr_t)functionVoidPtr;
  assert(functionPtr);
  return functionPtr(arg1, arg2);
}

void IrBuilderAst::visit(const AstSeq&, Place place, int childNo) {
  if (place==ePostOrder && childNo==0) {
    throw runtime_error::runtime_error("Empty sequence not allowed (yet)");
  }
  if (place==ePreOrder || childNo<2) return;
  // remove 2nd last value
  Value* lastValue = valuesBackAndPop();
  m_values.pop_back();
  m_values.push_back(lastValue);
}

void IrBuilderAst::visit(const AstOperator& op) {
  Value* result = NULL;
  // order of visit childs = order of push: lhs, rhs
  // ie top=rhs, top-1=lhs
  Value* rhs = valuesBackAndPop();
  Value* lhs = valuesBackAndPop();
  switch (op.op()) {
  case '=': result = m_builder.CreateStore(rhs, lhs); break;
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

void IrBuilderAst::visit(const AstSymbol& symbol) {
  const SymbolTableEntry& stentry = m_symbolTable[symbol.name()];
  assert(stentry && stentry.m_value);
  m_values.push_back(
    symbol.valueCategory()==AstSymbol::eLValue || stentry.m_storage==eValue ?
    stentry.m_value :
    m_builder.CreateLoad(stentry.m_value, symbol.name().c_str()));
}

void IrBuilderAst::visit(const AstFunDef& funDef, Place place) {
  // ePreOrder
  // ---------------
  if ( ePreOrder==place ) { /*nop*/ }

  // outer iterator makes funDef.decl() being visited. That pushes a Function*
  // onto m_values
  // ---------------

  // eInOrder
  // ---------------
  else if ( eInOrder==place ) {
    m_symbolTable.clear();

    Function* functionIr = valuesBackToFunction();
    m_builder.SetInsertPoint(
      BasicBlock::Create(getGlobalContext(), "entry", functionIr));

    // Add all arguments to the symbol table and create their allocas.
    Function::arg_iterator iterIr = functionIr->arg_begin();
    list<string>::const_iterator iterAst = funDef.decl().args().begin();
    for (/*nop*/; iterIr != functionIr->arg_end(); ++iterIr, ++iterAst) {
      AllocaInst *alloca = createEntryBlockAlloca(functionIr, *iterAst);
      m_builder.CreateStore(iterIr, alloca);
      m_symbolTable[*iterAst] = SymbolTableEntry( alloca, eAlloca);
    }

    // Don't pop the Function* from m_values, see below
  }

  // outer iterator makes funDef.body() being visited. 
  // ---------------

  // ePostOrder
  // ---------------
  else {
    m_builder.CreateRet(valuesBackAndPop());

    verifyFunction(*valuesBackToFunction());

    // Previous building block is again the insert point
    m_builder.SetInsertPoint(m_mainBasicBlock);

    // Don't push onto the m_values stack, since the Function* we would push
    // is already on the stack due to funDef.decl()
  }
}

void IrBuilderAst::visit(const AstFunDecl& funDecl) {
  // currently rettype and type of args is always int
  vector<Type*> argsIr(funDecl.args().size(),
    Type::getInt32Ty(getGlobalContext()));
  Type* retTypeIr = Type::getInt32Ty(getGlobalContext());
  FunctionType* functionTypeIr = FunctionType::get(retTypeIr, argsIr, false);
  assert(functionTypeIr);
  Function* functionIr = Function::Create( functionTypeIr,
    Function::ExternalLinkage, funDecl.name(), m_module );
  assert(functionIr);
  if ( functionIr->getName() != funDecl.name() ) {
    functionIr->eraseFromParent();
    functionIr = m_module->getFunction(funDecl.name());
    assert(functionIr->arg_size() == funDecl.args.size());
  }
  else {
    list<string>::const_iterator iterAst = funDecl.args().begin();
    Function::arg_iterator iterIr = functionIr->arg_begin();
    for (/*nop*/; iterAst!=funDecl.args().end(); ++iterAst, ++iterIr) {
      iterIr->setName(*iterAst);
    }
  }

  m_values.push_back(functionIr);
}

void IrBuilderAst::visit(const AstFunCall& funCall) {
  Function* callee = m_module->getFunction(funCall.name());
  assert(callee);
  Value* value = m_builder.CreateCall(callee, vector<Value*>(), "calltmp");
  m_values.push_back( value );
}

void IrBuilderAst::visit(const AstDataDef& dataDef) {
  Value* const initValue = valuesBack();
  SymbolTableEntry& stentry = m_symbolTable[dataDef.decl().name()];
  if ( dataDef.decl().storage()==AstDataDecl::eAlloca ) {
    Function* functionIr = m_builder.GetInsertBlock()->getParent();
    assert(functionIr);
    AllocaInst* alloca =
      createEntryBlockAlloca(functionIr, dataDef.decl().name());
    assert(alloca);
    m_builder.CreateStore(initValue, alloca);
    stentry = SymbolTableEntry(alloca, eAlloca);
  } else {
    stentry = SymbolTableEntry(initValue, eValue);  
  }
}

Value* IrBuilderAst::valuesBackAndPop() {
  assert(!m_values.empty());
  Value* value = m_values.back();
  m_values.pop_back();
  assert(value);
  return value; 
}

Value* IrBuilderAst::valuesBack() {
  assert(!m_values.empty());
  Value* value = m_values.back();
  assert(value);
  return value; 
}

Function* IrBuilderAst::valuesBackToFunction() {
  assert(!m_values.empty());
  Function* function = dynamic_cast<Function*>(m_values.back());
  assert(function);
  return function;
}

AllocaInst* IrBuilderAst::createEntryBlockAlloca(Function *functionIr,
  const string &varName) {
  IRBuilder<> irBuilder(&functionIr->getEntryBlock(),
    functionIr->getEntryBlock().begin());
  return m_builder.CreateAlloca(Type::getInt32Ty(getGlobalContext()), 0,
    varName.c_str());
}
