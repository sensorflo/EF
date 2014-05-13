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

void IrBuilderAst::visit(const AstSeq& seq) {
  const std::list<AstNode*>& childs = seq.childs();
  if (childs.empty()) {
    throw runtime_error::runtime_error("Empty sequence not allowed (yet)");
  }
  for (list<AstNode*>::const_iterator i = childs.begin();
       i!=childs.end(); ++i) {
    // Two consequtive childs are like the comma operator in C: the lhs is
    // evaluated, but its value is droped, and the value of the binary comma
    // operator expression is the evaluated rhs. lhs is the *i of the previous
    // iteration, and rhs is the *i of the current iteration.
    if (i!=childs.begin()) {
      assert(!m_values.empty());
      m_values.pop_back(); // drop lhs
    }
    // evaluate rhs (lhs in the first iteration). will internally push one
    // single value on the values stack
    (*i)->accept(*this);
  }
}

void IrBuilderAst::visit(const AstOperator& op) {
  op.lhs().accept(*this);
  Value* lhs = valuesBackAndPop();
  op.rhs().accept(*this);
  Value* rhs = valuesBackAndPop();
  Value* result = NULL;
  switch (op.op()) {
  case '=': result = m_builder.CreateStore(rhs, lhs); break;
  case '-': result = m_builder.CreateSub(lhs, rhs, "subtmp"); break;
  case '+': result = m_builder.CreateAdd(lhs, rhs, "addtmp"); break;
  case '*': result = m_builder.CreateMul(lhs, rhs, "multmp"); break;
  case '/': result = m_builder.CreateSDiv(lhs, rhs, "divtmp"); break;
  default: assert(false); break;
  }
  assert(result);
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

void IrBuilderAst::visit(const AstFunDef& funDef) {
  funDef.decl().accept(*this);
  Function* functionIr = valuesBackToFunction();

  m_symbolTable.clear();
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

  funDef.body().accept(*this);
  m_builder.CreateRet(valuesBackAndPop());

  verifyFunction(*functionIr);

  // Previous building block is again the insert point
  m_builder.SetInsertPoint(m_mainBasicBlock);

  // Don't push onto the m_values stack, since the Function* we would push
  // is already on the stack due to funDef.decl()
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
  Value* initValue = NULL;
  if (dataDef.initValue()) {
    dataDef.initValue()->accept(*this);
    initValue = valuesBack(); 
  } else {
    initValue = ConstantInt::get( getGlobalContext(), APInt(32, 0));
    m_values.push_back( initValue );
  }
  assert(initValue);
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

void IrBuilderAst::visit(const AstIf& if_) {
  // misc setup
  const list<AstIf::ConditionActionPair>& capairs = if_.conditionActionPairs();
  assert(!capairs.empty());
  const AstIf::ConditionActionPair& capair = capairs.front(); 

  // setup needed basic blocks
  Function* functionIr = m_builder.GetInsertBlock()->getParent();
  BasicBlock* ThenBB = BasicBlock::Create(getGlobalContext(), "then", functionIr);
  BasicBlock* ElseBB = BasicBlock::Create(getGlobalContext(), "else");
  BasicBlock* MergeBB = BasicBlock::Create(getGlobalContext(), "ifcont");

  // IR for evaluate condition
  capair.m_condition->accept(*this);
  Value* condIr = valuesBackAndPop();
  Value* condCmpIr = m_builder.CreateICmpNE(condIr,
    ConstantInt::get(getGlobalContext(), APInt(32, 0)), "ifcond");

  // IR for branch based on condition
  m_builder.CreateCondBr(condCmpIr, ThenBB, ElseBB);

  // IR for then clause
  m_builder.SetInsertPoint(ThenBB);
  capair.m_action->accept(*this);
  Value* thenValue = valuesBackAndPop();
  m_builder.CreateBr(MergeBB);
  ThenBB = m_builder.GetInsertBlock();

  // IR for else clause
  functionIr->getBasicBlockList().push_back(ElseBB);
  m_builder.SetInsertPoint(ElseBB);
  Value* elseValue = NULL;
  assert( if_.elseAction() ); // currently an else action is mandatory. else
                              // its not trivial to calculate phi below
  if_.elseAction()->accept(*this);
  elseValue = valuesBackAndPop();
  m_builder.CreateBr(MergeBB);
  ElseBB = m_builder.GetInsertBlock();

  // IR for merge of then/else clauses
  functionIr->getBasicBlockList().push_back(MergeBB);
  m_builder.SetInsertPoint(MergeBB);
  PHINode* phi = m_builder.CreatePHI(Type::getInt32Ty(getGlobalContext()), 2,
    "iftmp");
  assert(phi);
  phi->addIncoming(thenValue, ThenBB);
  phi->addIncoming(elseValue, ElseBB);
  m_values.push_back(phi);
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
