#include "irbuilderast.h"
#include "llvm/ExecutionEngine/JIT.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include <algorithm>
#include <cstdlib>
#include <stdexcept>
using namespace std;
using namespace llvm;

void IrBuilderAst::staticOneTimeInit() {
  InitializeNativeTarget();
}

IrBuilderAst::IrBuilderAst(Env& env) :
  m_builder(getGlobalContext()),
  m_module(new Module("Main", getGlobalContext())),
  m_executionEngine(EngineBuilder(m_module).setErrorStr(&m_errStr).create()),
  m_mainFunction(NULL),
  m_mainBasicBlock(NULL),
  m_env(env) {

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
  m_builder.CreateRet(visit(seq));
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

Value* IrBuilderAst::visit(const AstSeq& seq, Access) {
  const list<AstNode*>& childs = seq.childs();
  if (childs.empty()) {
    throw runtime_error::runtime_error("Empty sequence not allowed (yet)");
  }
  // Evalue all nodes of the sequence. The value of the sequence is the value
  // of the last node.
  Value* ret = NULL;
  for (list<AstNode*>::const_iterator i = childs.begin();
       i!=childs.end(); ++i) {
    ret = (*i)->accept(*this);
  }
  return ret;
}

Value* IrBuilderAst::visit(const AstOperator& op, Access) {
  const list<AstValue*>& argschilds = op.argschilds();
  list<AstValue*>::const_iterator iter = argschilds.begin();
  Value* resultIr = NULL;
  AstOperator::EOperation op_ = op.op();

  // Determine initial value of chain. Some operators start with constants,
  // other already need to read in one or two operands.
  switch (op_) {
  case AstOperator::eNot:
    assert(argschilds.size()==1);
    break;
  case AstOperator::eSub:
    if (argschilds.size()<=1) {
      resultIr = ConstantInt::get( getGlobalContext(), APInt(32, 0));
    } else {
      const AstValue& lhsAst = **(iter++);
      resultIr = lhsAst.accept(*this);
    }
    break;
  case AstOperator::eOr:
    resultIr = ConstantInt::get( getGlobalContext(), APInt(1, 0));
    break;
  case AstOperator::eAnd:
    resultIr = ConstantInt::get( getGlobalContext(), APInt(1, 1));
    break;
  case AstOperator::eAdd:
    resultIr = ConstantInt::get( getGlobalContext(), APInt(32, 0));
    break;
  case AstOperator::eMul:
    resultIr = ConstantInt::get( getGlobalContext(), APInt(32, 1));
    break;
  case AstOperator::eAssign: // fallthrough
  case AstOperator::eDiv: {
    if ( op_==AstOperator::eAssign ) { assert(argschilds.size()==2); }
    if ( op_==AstOperator::eDiv )    { assert(argschilds.size()>=2); }
    const AstValue& lhsAst = **(iter++);
    resultIr = lhsAst.accept(*this, op_==AstOperator::eAssign ? eWrite : eRead);
    break;
  }
  default:
    assert(false);
  }

  // Iterate trough all operands
  for (; iter!=argschilds.end(); ++iter) {
    const AstValue& operandAst = **iter;
    Value* operandIr = operandAst.accept(*this, eRead);
    Value* operandIrBool = m_builder.CreateICmpNE(operandIr,
      ConstantInt::get( getGlobalContext(), APInt(32, 0)));

    switch (op_) {
    case AstOperator::eAssign   :            m_builder.CreateStore (operandIr,      resultIr                );
                                  resultIr = operandIr; break;
    case AstOperator::eNot      : resultIr = m_builder.CreateNot   (operandIrBool,                 "nottmp" ); break;
    case AstOperator::eAnd      : resultIr = m_builder.CreateAnd   (resultIr,       operandIrBool, "andtmp" ); break;
    case AstOperator::eOr       : resultIr = m_builder.CreateOr    (resultIr,       operandIrBool, "ortmp"  ); break;
    case AstOperator::eSub      : resultIr = m_builder.CreateSub   (resultIr,       operandIr,     "subtmp" ); break;
    case AstOperator::eAdd      : resultIr = m_builder.CreateAdd   (resultIr,       operandIr,     "addtmp" ); break;
    case AstOperator::eMul      : resultIr = m_builder.CreateMul   (resultIr,       operandIr,     "multmp" ); break;
    case AstOperator::eDiv      : resultIr = m_builder.CreateSDiv  (resultIr,       operandIr,     "divtmp" ); break;
    default: assert(false); break;
    }
  }

  assert(resultIr);
  if (op_==AstOperator::eNot || op_==AstOperator::eAnd || op_==AstOperator::eOr) {
    resultIr = m_builder.CreateZExt(resultIr, Type::getInt32Ty(getGlobalContext()));
  }
  return resultIr;
}

Value* IrBuilderAst::visit(const AstNumber& number, Access) {
  return ConstantInt::get( getGlobalContext(), APInt(32, number.value()));
}

Value* IrBuilderAst::visit(const AstSymbol& symbol, Access access) {
  SymbolTableEntry* stentry = m_env.find(symbol.name());
  if (NULL==stentry) {
    throw runtime_error::runtime_error("Symbol '" + symbol.name() +
      "' not declared");
  }
  assert( stentry->valueIr() );

  Value* value = NULL;
  if (stentry->objType().qualifier()&ObjType::eMutable) {
    // stentry->valueIr() is the pointer returned by alloca corresponding to
    // the symbol
    if (access==eWrite) {
      value = stentry->valueIr(); 
    } else {
      value = m_builder.CreateLoad(stentry->valueIr(), symbol.name().c_str());
    }
  } else {
    if (access==eWrite) {
      throw runtime_error::runtime_error("Can't write to value (inmutable data) '"
        + symbol.name() + "'.");
    }
    // stentry->valueIr() is directly the value of the symbol
    value = stentry->valueIr();
  }
  return value;
}

Function* IrBuilderAst::visit(const AstFunDef& funDef) {
  Function* functionIr = visit(funDef.decl());

  m_env.pushScope(); 
  m_builder.SetInsertPoint(
    BasicBlock::Create(getGlobalContext(), "entry", functionIr));

  // Add all arguments to the symbol table and create their allocas.
  Function::arg_iterator iterIr = functionIr->arg_begin();
  list<AstArgDecl*>::const_iterator iterAst = funDef.decl().args().begin();
  for (/*nop*/; iterIr != functionIr->arg_end(); ++iterIr, ++iterAst) {
    // blindly assumes that ObjType::Qualifier is eMutable since currently
    // that is always the case
    const string& argName = (*iterAst)->name();
    AllocaInst *alloca = createEntryBlockAlloca(functionIr, argName);
    m_builder.CreateStore(iterIr, alloca);
    SymbolTableEntry* stentry = new SymbolTableEntry( alloca,
      new ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable), true);
    Env::InsertRet insertRet = m_env.insert(argName, stentry);

    // if name is already in environment, then it must be another argument
    // since we just pushed a new scope. 
    if ( !insertRet.second ) {
      delete stentry;
      throw runtime_error::runtime_error("Argument '" + argName +
        "' already defined");
    }
  }

  m_builder.CreateRet(funDef.body().accept(*this));

  //verifyFunction(*functionIr);

  // Previous building block is again the insert point
  m_builder.SetInsertPoint(m_mainBasicBlock);

  m_env.popScope(); 

  return functionIr;
}

Function* IrBuilderAst::visit(const AstFunDecl& funDecl) {
  // currently rettype and type of args is always int

  // create IR function with given name and signature
  vector<Type*> argsIr(funDecl.args().size(),
    Type::getInt32Ty(getGlobalContext()));
  Type* retTypeIr = Type::getInt32Ty(getGlobalContext());
  FunctionType* functionTypeIr = FunctionType::get(retTypeIr, argsIr, false);
  assert(functionTypeIr);
  Function* functionIr = Function::Create( functionTypeIr,
    Function::ExternalLinkage, funDecl.name(), m_module );
  assert(functionIr);

  // If the names differ (see condition of if statement below) that means a
  // function with that name already existed, so LLVM automaticaly chose a new
  // name. If the function already existed, blindly assume that this
  // declaration's signature matches the signature of the previous declaration
  // (to ensure that was the semantic analyzer's job) and just 'undo' creating
  // a new function by erasing the new wrongly created function.
  if (functionIr->getName() != funDecl.name()) {
    functionIr->eraseFromParent();
    functionIr = m_module->getFunction(funDecl.name());
  }

  // set names of arguments of IR function
  else {
    list<AstArgDecl*>::const_iterator iterArgsAst = funDecl.args().begin();
    Function::arg_iterator iterArgsIr = functionIr->arg_begin();
    for (/*nop*/; iterArgsAst!=funDecl.args().end(); ++iterArgsAst, ++iterArgsIr) {
      iterArgsIr->setName((*iterArgsAst)->name());
    }
  }

  return functionIr;
}

Value* IrBuilderAst::visit(const AstFunCall& funCall, Access access) {
  Function* callee = m_module->getFunction(funCall.name());
  if (!callee) {
    throw runtime_error::runtime_error("Function not defined");
  }

  const list<AstValue*>& argsAst = funCall.args().childs();

  if (callee->arg_size() != argsAst.size()) {
    throw runtime_error::runtime_error("Number of arguments do not match");
  }

  vector<Value*> argsIr;
  for ( list<AstValue*>::const_iterator i = argsAst.begin();
        i != argsAst.end(); ++i ) {
    argsIr.push_back((*i)->accept(*this));
  }

  return m_builder.CreateCall(callee, argsIr, "calltmp");
}

Value* IrBuilderAst::visit(const AstDataDecl& dataDecl, Access) {
  SymbolTableEntry* dummy;
  return visit(dataDecl, dummy);
}

Value* IrBuilderAst::visit(const AstDataDecl& dataDecl,
  SymbolTableEntry*& stentry) {

  Env::InsertRet insertRet = m_env.insert( dataDecl.name(), NULL);
  SymbolTable::iterator& stIter = insertRet.first;
  SymbolTableEntry*& stIterStEntry = stIter->second;
  bool wasAlreadyInMap = !insertRet.second;

  if (!wasAlreadyInMap) {
    stIterStEntry = new SymbolTableEntry(NULL, &dataDecl.objType(true));
  } else {
    assert(stIterStEntry);
    if ( ObjType::eFullMatch != stIterStEntry->objType().match(dataDecl.objType()) ) {
      throw runtime_error::runtime_error("Idenifier '" + dataDecl.name() +
        "' declared or defined again with a different type.");
    }
    stIterStEntry->setObjType(&dataDecl.objType(true));
  }
  stentry = stIterStEntry;
  return ConstantInt::get( getGlobalContext(), APInt(32, 0));
}

Value* IrBuilderAst::visit(const AstDataDef& dataDef, Access access) {
  // process data declaration. That ensures an entry in the symbol table
  SymbolTableEntry* stentry = NULL;
  visit(dataDef.decl(), stentry);
  assert(stentry);

  if (stentry->isDefined()) {
    throw runtime_error::runtime_error("Data object '" +
      dataDef.decl().name() + "' is already defined.");
  }
  stentry->isDefined() = true;

  // Calculate value to initialzie new data object with
  Value* initValue = dataDef.initValue() ?
    dataDef.initValue()->accept(*this) :
    ConstantInt::get( getGlobalContext(), APInt(32, 0));
  assert(initValue);

  // define m_value (type Value*) of symbol table entry. For values that is
  // trivial. For variables aka allocas first an alloca has to be created.
  if ( stentry->objType().qualifier()==ObjType::eNoQualifier ) {
    stentry->valueIr() = initValue;
    if ( eRead!=access ) {
      throw runtime_error::runtime_error("Cannot write to an inmutable data object");
    }
    return initValue; 
  }
  else if ( stentry->objType().qualifier()==ObjType::eMutable ) {
    Function* functionIr = m_builder.GetInsertBlock()->getParent();
    assert(functionIr);
    AllocaInst* alloca =
      createEntryBlockAlloca(functionIr, dataDef.decl().name());
    assert(alloca);
    m_builder.CreateStore(initValue, alloca);
    stentry->valueIr() = alloca;
    return eRead==access ? initValue : alloca;
  }
  else { assert(false); }
}

Value* IrBuilderAst::visit(const AstIf& if_, Access access) {
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
  Value* condIr = capair.m_condition->accept(*this);
  Value* condCmpIr = m_builder.CreateICmpNE(condIr,
    ConstantInt::get(getGlobalContext(), APInt(32, 0)), "ifcond");

  // IR for branch based on condition
  m_builder.CreateCondBr(condCmpIr, ThenBB, ElseBB);

  // IR for then clause
  m_builder.SetInsertPoint(ThenBB);
  Value* thenValue = capair.m_action->accept(*this);
  m_builder.CreateBr(MergeBB);
  ThenBB = m_builder.GetInsertBlock();

  // IR for else clause
  functionIr->getBasicBlockList().push_back(ElseBB);
  m_builder.SetInsertPoint(ElseBB);
  Value* elseValue = NULL;
  if ( if_.elseAction() ) {
    elseValue = if_.elseAction()->accept(*this);
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
  return phi;
}

AllocaInst* IrBuilderAst::createEntryBlockAlloca(Function *functionIr,
  const string &varName) {
  IRBuilder<> irBuilder(&functionIr->getEntryBlock(),
    functionIr->getEntryBlock().begin());
  return m_builder.CreateAlloca(Type::getInt32Ty(getGlobalContext()), 0,
    (varName + "__alloca").c_str());
}
