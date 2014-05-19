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

int IrBuilderAst::jitExecFunction(const string& name) {
  return jitExecFunction(m_module->getFunction(name));
}

int IrBuilderAst::jitExecFunction1Arg(const string& name, int arg1) {
  return jitExecFunction1Arg(m_module->getFunction(name), arg1);
}

int IrBuilderAst::jitExecFunction2Arg(const string& name, int arg1, int arg2) {
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
  const list<AstNode*>& childs = seq.childs();
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
  const list<AstNode*>& argschilds = op.argschilds();
  list<AstNode*>::const_iterator iter = argschilds.begin();
  Value* result = NULL;

  // Determine initial value of chain. Some operators start with constants,
  // other already need to read in one or two operands.
  switch (op.op()) {
  case AstOperator::eSub:
    if (argschilds.size()<=1) {
      result = ConstantInt::get( getGlobalContext(), APInt(32, 0));
    } else {
      const AstNode& lhsNode = **(iter++);
      lhsNode.accept(*this);
      result /*aka lhs*/ = valuesBackAndPop();
    }
    break;
  case AstOperator::eAdd:
    result = ConstantInt::get( getGlobalContext(), APInt(32, 0));
    break;
  case AstOperator::eMul:
    result = ConstantInt::get( getGlobalContext(), APInt(32, 1));
    break;
  case AstOperator::eAssign: // fallthrough
  case AstOperator::eDiv: {
    if ( op.op()==AstOperator::eAssign ) { assert(argschilds.size()==2); }
    if ( op.op()==AstOperator::eDiv )    { assert(argschilds.size()>=2); }
    const AstNode& lhsNode = **(iter++);
    lhsNode.accept(*this);
    result /*aka lhs*/ = valuesBackAndPop();
    break;
  }
  default:
    assert(false);
  }

  // Iterate trough all operands
  for (; iter!=argschilds.end(); ++iter) {
    const AstNode& operandNode = **iter;
    operandNode.accept(*this);
    Value* operand = valuesBackAndPop();
    switch (op.op()) {
    case AstOperator::eAssign   : m_builder.CreateStore(operand, result);
                                  result = operand; break;
    case AstOperator::eSub      : result = m_builder.CreateSub(result, operand, "subtmp"); break;
    case AstOperator::eAdd      : result = m_builder.CreateAdd(result, operand, "addtmp"); break;
    case AstOperator::eMul      : result = m_builder.CreateMul(result, operand, "multmp"); break;
    case AstOperator::eDiv      : result = m_builder.CreateSDiv(result, operand, "divtmp"); break;
    default: assert(false); break;
    }
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
  SymbolTableIter stentryi = m_symbolTable.find(symbol.name());
  if (stentryi==m_symbolTable.end()) {
    throw runtime_error::runtime_error("Symbol '" + symbol.name() +
      "' not declared");
  }
  const SymbolTableEntry& stentry = stentryi->second;
  assert( stentry.m_value );
  m_values.push_back(
    symbol.valueCategory()==AstSymbol::eLValue || stentry.m_storage==eValue ?
    stentry.m_value :
    m_builder.CreateLoad(stentry.m_value, symbol.name().c_str()));
}

void IrBuilderAst::visit(const AstFunDef& funDef) {
  Function* functionIr = NULL;
  visit(funDef.decl(), functionIr);

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
  Function* dummy;
  visit(funDecl, dummy);
}

void IrBuilderAst::visit(const AstFunDecl& funDecl, Function*& functionIr) {
  // currently rettype and type of args is always int
  vector<Type*> argsIr(funDecl.args().size(),
    Type::getInt32Ty(getGlobalContext()));
  Type* retTypeIr = Type::getInt32Ty(getGlobalContext());
  FunctionType* functionTypeIr = FunctionType::get(retTypeIr, argsIr, false);
  assert(functionTypeIr);
  functionIr = Function::Create( functionTypeIr,
    Function::ExternalLinkage, funDecl.name(), m_module );
  assert(functionIr);
  if ( functionIr->getName() != funDecl.name() ) {
    functionIr->eraseFromParent();
    functionIr = m_module->getFunction(funDecl.name());
    assert(functionIr->arg_size() == funDecl.args().size());
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
  if (!callee) {
    throw runtime_error::runtime_error("Function not defined");
  }

  const list<AstNode*>& argsAst = funCall.args().childs();

  if (callee->arg_size() != argsAst.size()) {
    throw runtime_error::runtime_error("Number of arguments do not match");
  }

  vector<Value*> argsIr;
  for ( list<AstNode*>::const_iterator i = argsAst.begin();
        i != argsAst.end(); ++i ) {
    (*i)->accept(*this);
    argsIr.push_back(valuesBackAndPop());
  }

  Value* callsValueIr = m_builder.CreateCall(callee, argsIr, "calltmp");
  m_values.push_back( callsValueIr );
}

void IrBuilderAst::visit(const AstDataDecl& dataDecl) {
  SymbolTableEntry* dummy;
  visit(dataDecl, dummy);

  // dummy, since each visit meth _must_ push _one_ element on the values
  // stack
  m_values.push_back( ConstantInt::get( getGlobalContext(), APInt(32, 0)) );
}

void IrBuilderAst::visit(const AstDataDecl& dataDecl,
  SymbolTableEntry*& stentry) {

  SymbolTableEntry newStEnry(NULL, static_cast<EStorage>(dataDecl.storage()));
  SymbolTableInsertResult stir = m_symbolTable.insert(
    make_pair(dataDecl.name(), newStEnry));
  bool wasAlreadyInMap = !stir.second;
  SymbolTableIter sti = stir.first;
  stentry = &sti->second;
  if (wasAlreadyInMap) {
    if ( stentry->m_storage != static_cast<EStorage>(dataDecl.storage()) ) {
      throw runtime_error::runtime_error("Idenifier '" + dataDecl.name() +
        "' declared or defined again with a different type.");
    }
  }
}

void IrBuilderAst::visit(const AstDataDef& dataDef) {
  // process data declaration. That ensures an entry in the symbol table
  SymbolTableEntry* stentry = NULL;
  visit(dataDef.decl(), stentry);
  assert(stentry);

  // Calculate value to initialzie new data object with. Also already push
  // that value on the values stack, since that is the result of the data
  // object declaration expression.
  Value* initValue = NULL;
  if (dataDef.initValue()) {
    dataDef.initValue()->accept(*this);
    initValue = valuesBack(); 
  } else {
    initValue = ConstantInt::get( getGlobalContext(), APInt(32, 0));
    m_values.push_back( initValue );
  }
  assert(initValue);

  // define m_value (type Value*) of symbol table entry. For values that is
  // trivial. For variables aka allocas first an alloca has to be created.
  if ( stentry->m_storage==eValue ) {
    stentry->m_value = initValue;
  }
  else if ( stentry->m_storage==eAlloca ) {
    Function* functionIr = m_builder.GetInsertBlock()->getParent();
    assert(functionIr);
    AllocaInst* alloca =
      createEntryBlockAlloca(functionIr, dataDef.decl().name());
    assert(alloca);
    m_builder.CreateStore(initValue, alloca);
    stentry->m_value = alloca;
  }
  else { assert(false); }

  // Nothing to push on the values stack, that was already done earlier in
  // this method.
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
  if ( if_.elseAction() ) {
    if_.elseAction()->accept(*this);
    elseValue = valuesBackAndPop();
  } else {
    elseValue = ConstantInt::get( getGlobalContext(), APInt(32, 0));
  }
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
