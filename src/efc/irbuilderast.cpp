#include "irbuilderast.h"
#include "ast.h"
#include "env.h"
#include "errorhandler.h"
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

IrBuilderAst::IrBuilderAst(Env& env, ErrorHandler& errorHandler,
  AstVisitor* enclosingVisitor) :
  m_builder(getGlobalContext()),
  m_module(new Module("Main", getGlobalContext())),
  m_executionEngine(EngineBuilder(m_module).setErrorStr(&m_errStr).create()),
  m_mainFunction(NULL),
  m_env(env),
  m_errorHandler(errorHandler),
  m_enclosingVisitor(enclosingVisitor ? enclosingVisitor : this) {
  assert(m_module);
  assert(m_executionEngine);
}

IrBuilderAst::~IrBuilderAst() {
  if (m_executionEngine) { delete m_executionEngine; }
}

void IrBuilderAst::setEnclosingVisitor(AstVisitor* enclosingVisitor) {
  m_enclosingVisitor = enclosingVisitor ? enclosingVisitor : this;
}

void IrBuilderAst::buildModuleNoImplicitMain(AstNode& root) {
  callAcceptOn(root);
}

/** In contrast to buildModuleNoImplicitMain an implicit main method is put
around the given AST. */
void IrBuilderAst::buildModule(AstValue& root) {

  // protects against double definition of the implicit main method
  assert(!m_mainFunction);

  vector<Type*> argsIr(0);
  Type* retTypeIr = Type::getInt32Ty(getGlobalContext());
  FunctionType* functionTypeIr = FunctionType::get( retTypeIr, argsIr, false);
  m_mainFunction = Function::Create( functionTypeIr,
    Function::ExternalLinkage, "main", m_module );
  assert(m_mainFunction);

  m_builder.SetInsertPoint( BasicBlock::Create( getGlobalContext(), "entry",
      m_mainFunction));

  // Currently the return type is always int, so ensure the return type of the
  // IR code is also Int32
  Value* resultIr = callAcceptOn(root);
  assert(resultIr);
  Value* extResultIr = m_builder.CreateZExt( resultIr, Type::getInt32Ty(getGlobalContext()));
  m_builder.CreateRet(extResultIr);

  verifyFunction(*m_mainFunction);
}

int IrBuilderAst::buildAndRunModule(AstValue& root) {
  buildModule(root);
  return runModule();
}

int IrBuilderAst::runModule() {
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

llvm::Value* IrBuilderAst::callAcceptOn(AstNode& node) {
  node.accept(*m_enclosingVisitor);
  return node.irValue();
}

void IrBuilderAst::visit(AstCast& cast) {
  cast.setIrValue(NULL);
}

void IrBuilderAst::visit(AstCtList&) {
  assert(false); // parent of AstCtList shall not decent run accept on AstCtList
}

void IrBuilderAst::visit(AstOperator& op) {
  const list<AstValue*>& argschilds = op.args().childs();
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
      AstValue& lhsAst = **(iter++);
      resultIr = callAcceptOn(lhsAst);
      assert(resultIr);
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
  case AstOperator::eSeq:
    // nop
    break;
  case AstOperator::eAssign: // fallthrough
  case AstOperator::eEqualTo:
  case AstOperator::eDiv: {
    if ( op_==AstOperator::eAssign ) { assert(argschilds.size()==2); }
    if ( op_==AstOperator::eEqualTo ){ assert(argschilds.size()==2); }
    if ( op_==AstOperator::eDiv )    { assert(argschilds.size()>=2); }
    AstValue& lhsAst = **(iter++);
    if ( op_==AstOperator::eAssign ) {
      // When moved to SementaicAnalizer then the const cast won't be needed
      // anymore. Until then it's not too bad.
      lhsAst.setAccess(eWrite, m_errorHandler);
    }
    resultIr = callAcceptOn(lhsAst);
    assert(resultIr);
    break;
  }
  default:
    assert(false);
  }

  // Iterate trough all operands
  for (; iter!=argschilds.end(); ++iter) {
    AstValue& operandAst = **iter;
    Value* operandIr = callAcceptOn(operandAst);
    if ( op_!=AstOperator::eSeq ) {
      assert(operandIr);
    }

    switch (op_) {
    case AstOperator::eAssign   :            m_builder.CreateStore (operandIr, resultIr            );
                                  resultIr = operandIr; break;
    case AstOperator::eNot      : resultIr = m_builder.CreateNot   (operandIr,            "nottmp" ); break;
    case AstOperator::eAnd      : resultIr = m_builder.CreateAnd   (resultIr,  operandIr, "andtmp" ); break;
    case AstOperator::eOr       : resultIr = m_builder.CreateOr    (resultIr,  operandIr, "ortmp"  ); break;
    case AstOperator::eSub      : resultIr = m_builder.CreateSub   (resultIr,  operandIr, "subtmp" ); break;
    case AstOperator::eAdd      : resultIr = m_builder.CreateAdd   (resultIr,  operandIr, "addtmp" ); break;
    case AstOperator::eMul      : resultIr = m_builder.CreateMul   (resultIr,  operandIr, "multmp" ); break;
    case AstOperator::eDiv      : resultIr = m_builder.CreateSDiv  (resultIr,  operandIr, "divtmp" ); break;
    case AstOperator::eEqualTo  : resultIr = m_builder.CreateICmpEQ(resultIr,  operandIr, "cmptmp" ); break;
    case AstOperator::eSeq      : resultIr = operandIr; break; // replacing previous resultIr
    default: assert(false); break;
    }
  }

  assert(resultIr);
  op.setIrValue(resultIr);
}

void IrBuilderAst::visit(AstNumber& number) {
  if ( number.objType().match(ObjTypeFunda(ObjTypeFunda::eInt)) ) {
    number.setIrValue(ConstantInt::get( getGlobalContext(),
        APInt(32, number.value())));
  } else if ( number.objType().match(ObjTypeFunda(ObjTypeFunda::eBool))) {
    number.setIrValue(ConstantInt::get( getGlobalContext(),
        APInt(1, number.value())));
  } else {
    assert(false);
  }
}

void IrBuilderAst::visit(AstSymbol& symbol) {
  SymbolTableEntry* stentry = m_env.find(symbol.name());
  if (NULL==stentry) {
    m_errorHandler.add(new Error(Error::eUnknownName));
    throw BuildError();
  }
  assert( stentry->valueIr() );

  Value* resultIr = NULL;
  if (stentry->objType().qualifier()&ObjType::eMutable) {
    // stentry->valueIr() is the pointer returned by alloca corresponding to
    // the symbol
    if (symbol.access()==eWrite) {
      resultIr = stentry->valueIr(); 
    } else {
      resultIr = m_builder.CreateLoad(stentry->valueIr(), symbol.name().c_str());
    }
  } else {
    if (symbol.access()==eWrite) {
      throw runtime_error::runtime_error("Can't write to value (inmutable data) '"
        + symbol.name() + "'.");
    }
    // stentry->valueIr() is directly the llvm::Value of the symbol
    resultIr = stentry->valueIr();
  }
  symbol.setIrValue(resultIr);
}

void IrBuilderAst::visit(AstFunDef& funDef) {
  visit(funDef.decl());
  Function* functionIr = funDef.decl().irFunction();
  assert(functionIr);
  SymbolTableEntry*const& stentry = funDef.decl().stentry();
  assert(stentry);

  if (stentry->isDefined()) {
    m_errorHandler.add(new Error(Error::eRedefinition));
    throw BuildError();
  }
  stentry->isDefined() = true;

  m_env.pushScope(); 
  if ( m_builder.GetInsertBlock() ) {
    m_BasicBlockStack.push(m_builder.GetInsertBlock());
  }
  m_builder.SetInsertPoint(
    BasicBlock::Create(getGlobalContext(), "entry", functionIr));

  // Add all arguments to the symbol table and create their allocas.
  Function::arg_iterator iterIr = functionIr->arg_begin();
  list<AstArgDecl*>::const_iterator iterAst = funDef.decl().args().begin();
  for (/*nop*/; iterIr != functionIr->arg_end(); ++iterIr, ++iterAst) {
    // blindly assumes that ObjType::Qualifier is eMutable since currently
    // that is always the case
    const string& argName = (*iterAst)->name();
    AllocaInst* alloca = createAllocaInEntryBlock(functionIr, argName);
    m_builder.CreateStore(iterIr, alloca);
    SymbolTableEntry* stentry = new SymbolTableEntry( alloca,
      new ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable), true);
    Env::InsertRet insertRet = m_env.insert(argName, stentry);

    // if name is already in environment, then it must be another argument
    // since we just pushed a new scope. 
    if ( !insertRet.second ) {
      delete stentry;
      m_errorHandler.add(new Error(Error::eRedefinition));
      throw BuildError();
    }
  }

  // Currently the return type is always int, so ensure the return type of the
  // IR code is also Int32
  Value* extResultIr = m_builder.CreateZExt( callAcceptOn(funDef.body()),
    Type::getInt32Ty(getGlobalContext()) );
  m_builder.CreateRet(extResultIr);

  //verifyFunction(*functionIr);

  if ( !m_BasicBlockStack.empty() ) {
    m_builder.SetInsertPoint(m_BasicBlockStack.top());
    m_BasicBlockStack.pop();
  }
  m_env.popScope(); 

  funDef.setIrFunction( functionIr );
}

void IrBuilderAst::visit(AstFunDecl& funDecl) {
  // currently rettype and type of args is always int

  // It is required that an earlier pass did insert this AstFundDecl into the
  // environment
  assert(funDecl.stentry());

  // If there is not yet an empty IR function associated to the stentry, to it
  // now. 
  if ( ! funDecl.stentry()->valueIr() ) {
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
    // name. That cannot be since above our environment said the name is unique.
    assert(functionIr->getName() == funDecl.name());

    // set names of arguments of IR function
    list<AstArgDecl*>::const_iterator iterArgsAst = funDecl.args().begin();
    Function::arg_iterator iterArgsIr = functionIr->arg_begin();
    for (/*nop*/; iterArgsAst!=funDecl.args().end(); ++iterArgsAst, ++iterArgsIr) {
      iterArgsIr->setName((*iterArgsAst)->name());
    }

    funDecl.stentry()->valueIr() = functionIr;
    funDecl.setIrFunction(functionIr);
  }

  // An earlier declaration of this function, i.e. one with the same stentry,
  // allready created the empty IR function
  else {
    funDecl.setIrValue(funDecl.stentry()->valueIr());
  }
}

void IrBuilderAst::visit(AstFunCall& funCall) {
  Function* callee = dynamic_cast<Function*>(callAcceptOn(funCall.address()));
  assert(callee);
    
  const list<AstValue*>& argsAst = funCall.args().childs();

  if (callee->arg_size() != argsAst.size()) {
    throw runtime_error::runtime_error("Number of arguments do not match");
  }

  vector<Value*> argsIr;
  for ( list<AstValue*>::const_iterator i = argsAst.begin();
        i != argsAst.end(); ++i ) {
    Value* argIr = callAcceptOn(**i);
    assert(argIr);
    argsIr.push_back(argIr);
  }

  funCall.setIrValue(m_builder.CreateCall(callee, argsIr, "calltmp"));
}

void IrBuilderAst::visit(AstDataDecl& dataDecl) {

  // Add to environment only local data objects. Currently there are only
  // local data objects. Insert new name as a new symbol table entry into the
  // environment, with NULL as ValueIr field. If the name is allready in the
  // environment, verify the type matches.
  if (!dataDecl.stentry()) {
    Env::InsertRet insertRet = m_env.insert( dataDecl.name(), NULL);
    SymbolTableEntry*& envs_stentry_ptr = insertRet.first->second;
    if (insertRet.second) {
      envs_stentry_ptr = new SymbolTableEntry(NULL, &dataDecl.objType(true));
    } else {
      assert(envs_stentry_ptr);
      if ( ObjType::eFullMatch != envs_stentry_ptr->objType().match(dataDecl.objType()) ) {
        m_errorHandler.add(new Error(Error::eIncompatibleRedaclaration));
        throw BuildError();
      }
    }

    // Since later the non-IR generating part will be extracted, a temporary
    // const cast is not too bad
    const_cast<AstDataDecl&>(dataDecl).setStentry(envs_stentry_ptr);
  }
  
  // note that this can be NULL. E.g. when we just created a new symbol table
  // entry above, or if it was only declared but never defined so far. Also
  // note that since currently there are only local vars, and since local vars
  // can only be defined but not declared-only, we don't have to care about
  // eRead/eWrite access yet.
  dataDecl.setIrValue(dataDecl.stentry()->valueIr());
}

void IrBuilderAst::visit(AstArgDecl& argDecl) {
  visit( dynamic_cast<AstDataDecl&>(argDecl));
}

void IrBuilderAst::visit(AstDataDef& dataDef) {
  // process data declaration. That ensures an entry in the symbol table
  visit(dataDef.decl());
  SymbolTableEntry*const& stentry = dataDef.decl().stentry();
  assert(stentry);

  if (stentry->isDefined()) {
    m_errorHandler.add(new Error(Error::eRedefinition));
    throw BuildError();
  }
  stentry->isDefined() = true;

  // define m_value (type Value*) of symbol table entry. For values that is
  // trivial. For variables aka allocas first an alloca has to be created.
  Value* initValue = callAcceptOn(dataDef.initValue());
  assert(initValue);
  if ( stentry->objType().qualifier()==ObjType::eNoQualifier ) {
    stentry->valueIr() = initValue;
    if ( dataDef.access()!=eRead ) {
      throw runtime_error::runtime_error("Cannot write to an inmutable data object");
    }
    dataDef.setIrValue(initValue);
  }
  else if ( stentry->objType().qualifier()==ObjType::eMutable ) {
    Function* functionIr = m_builder.GetInsertBlock()->getParent();
    assert(functionIr);
    AllocaInst* alloca =
      createAllocaInEntryBlock(functionIr, dataDef.decl().name());
    assert(alloca);
    m_builder.CreateStore(initValue, alloca);
    stentry->valueIr() = alloca;
    dataDef.setIrValue(dataDef.access()==eRead ? initValue : alloca);
  }
  else { assert(false); }
}

void IrBuilderAst::visit(AstIf& if_) {
  // misc setup
  const list<AstIf::ConditionActionPair>& capairs = if_.conditionActionPairs();
  assert(capairs.size()==1); // curently size>=2 is not yet supported
  const AstIf::ConditionActionPair& capair = capairs.front(); 

  // setup needed basic blocks
  Function* functionIr = m_builder.GetInsertBlock()->getParent();
  BasicBlock* ThenFirstBB = BasicBlock::Create(getGlobalContext(), "ifthen", functionIr);
  BasicBlock* ElseFirstBB = BasicBlock::Create(getGlobalContext(), "ifelse");
  BasicBlock* MergeBB = BasicBlock::Create(getGlobalContext(), "ifmerge");

  // IR for evaluate condition
  assert(capair.m_condition);
  Value* condIr = callAcceptOn(*capair.m_condition);
  assert(condIr);
  Value* condCmpIr = m_builder.CreateICmpNE(condIr,
    ConstantInt::get(getGlobalContext(), APInt(1, 0)), "ifcond");

  // IR for branch based on condition
  m_builder.CreateCondBr(condCmpIr, ThenFirstBB, ElseFirstBB);

  // IR for then clause
  m_builder.SetInsertPoint(ThenFirstBB);
  assert(capair.m_action);
  Value* thenValue = callAcceptOn(*capair.m_action);
  assert(thenValue);
  m_builder.CreateBr(MergeBB);
  BasicBlock* ThenLastBB = m_builder.GetInsertBlock();

  // IR for else clause
  functionIr->getBasicBlockList().push_back(ElseFirstBB);
  m_builder.SetInsertPoint(ElseFirstBB);
  Value* elseValue = NULL;
  if ( if_.elseAction() ) {
    elseValue = callAcceptOn(*if_.elseAction());
  } else {
    elseValue = ConstantInt::get( getGlobalContext(), APInt(32, 0));
  }
  assert(elseValue);
  m_builder.CreateBr(MergeBB);
  BasicBlock* ElseLastBB = m_builder.GetInsertBlock();

  // IR for merge of then/else clauses
  functionIr->getBasicBlockList().push_back(MergeBB);
  m_builder.SetInsertPoint(MergeBB);
  PHINode* phi = m_builder.CreatePHI(Type::getInt32Ty(getGlobalContext()), 2,
    "ifphi");
  assert(phi);
  phi->addIncoming(thenValue, ThenLastBB);
  phi->addIncoming(elseValue, ElseLastBB);
  if_.setIrValue(phi);
}

/** We want allocas in the entry block to facilitate llvm's mem2reg pass.*/
AllocaInst* IrBuilderAst::createAllocaInEntryBlock(Function* functionIr,
  const string &varName) {
  IRBuilder<> irBuilder(&functionIr->getEntryBlock(),
    functionIr->getEntryBlock().begin());
  return irBuilder.CreateAlloca(Type::getInt32Ty(getGlobalContext()), 0,
    varName.c_str());
}
