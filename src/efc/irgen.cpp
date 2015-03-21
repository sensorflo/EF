#include "irgen.h"
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

Value *const IrGen::m_void = reinterpret_cast<Value *>(0xFFFFFFFF);

void IrGen::staticOneTimeInit() {
  InitializeNativeTarget();
}

IrGen::IrGen(ErrorHandler& errorHandler) :
  m_builder(getGlobalContext()),
  m_module(new Module("Main", getGlobalContext())),
  m_executionEngine(EngineBuilder(m_module).setErrorStr(&m_errStr).create()),
  m_errorHandler(errorHandler) {
  assert(m_module);
  assert(m_executionEngine);
}

IrGen::~IrGen() {
  if (m_executionEngine) { delete m_executionEngine; }
}

/** Using the given AST, generates LLVM IR code, appending it to the one
implict LLVM module associated with this IrGen object.  At the top level of
the AST, only declarations or definitions are allowed.
\pre SemanticAnalizer must have massaged the AST and the Env */
void IrGen::genIr(AstNode& root) {
  callAcceptOn(root);
}

int IrGen::jitExecFunction(const string& name) {
  return jitExecFunction(m_module->getFunction(name));
}

int IrGen::jitExecFunction1Arg(const string& name, int arg1) {
  return jitExecFunction1Arg(m_module->getFunction(name), arg1);
}

int IrGen::jitExecFunction2Arg(const string& name, int arg1, int arg2) {
  return jitExecFunction2Arg(m_module->getFunction(name), arg1, arg2);
}

void IrGen::jitExecFunctionVoidRet(const string& name) {
  jitExecFunctionVoidRet(m_module->getFunction(name));
}

int IrGen::jitExecFunction(llvm::Function* function) {
  void* functionVoidPtr =
    m_executionEngine->getPointerToFunction(function);
  assert(functionVoidPtr);
  int (*functionPtr)() = (int (*)())(intptr_t)functionVoidPtr;
  assert(functionPtr);
  return functionPtr();
}

int IrGen::jitExecFunction1Arg(llvm::Function* function, int arg1) {
  void* functionVoidPtr =
    m_executionEngine->getPointerToFunction(function);
  assert(functionVoidPtr);
  int (*functionPtr)(int) = (int (*)(int))(intptr_t)functionVoidPtr;
  assert(functionPtr);
  return functionPtr(arg1);
}

int IrGen::jitExecFunction2Arg(llvm::Function* function, int arg1, int arg2) {
  void* functionVoidPtr =
    m_executionEngine->getPointerToFunction(function);
  assert(functionVoidPtr);
  int (*functionPtr)(int, int) = (int (*)(int, int))(intptr_t)functionVoidPtr;
  assert(functionPtr);
  return functionPtr(arg1, arg2);
}

void IrGen::jitExecFunctionVoidRet(llvm::Function* function) {
  void* functionVoidPtr =
    m_executionEngine->getPointerToFunction(function);
  assert(functionVoidPtr);
  void (*functionPtr)() = (void (*)())(intptr_t)functionVoidPtr;
  assert(functionPtr);
  functionPtr();
}

llvm::Value* IrGen::callAcceptOn(AstNode& node) {
  node.accept(*this);
  return node.irValue();
}

void IrGen::visit(AstNop& nop) {
}

void IrGen::visit(AstCast& cast) {
  cast.child().accept(*this);
  cast.setIrValue( m_builder.CreateZExt(
      cast.child().irValue(),
      Type::getInt32Ty(getGlobalContext())));
}

void IrGen::visit(AstCtList&) {
  assert(false); // parent of AstCtList shall not decent run accept on AstCtList
}

void IrGen::visit(AstOperator& op) {
  const list<AstValue*>& argschilds = op.args().childs();
  list<AstValue*>::const_iterator iter = argschilds.begin();
  Value* resultIr = NULL;
  const AstOperator::EOperation op_ = op.op();

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
  case AstOperator::eDotAssign:
  case AstOperator::eEqualTo:
  case AstOperator::eDiv: {
    if ( op_==AstOperator::eAssign ) { assert(argschilds.size()==2); }
    if ( op_==AstOperator::eDotAssign ) { assert(argschilds.size()==2); }
    if ( op_==AstOperator::eEqualTo ){ assert(argschilds.size()==2); }
    if ( op_==AstOperator::eDiv )    { assert(argschilds.size()>=2); }
    AstValue& lhsAst = **(iter++);
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
    case AstOperator::eDotAssign:            m_builder.CreateStore (operandIr, resultIr            );
                                  resultIr = op.access()==eWrite ? resultIr : operandIr; break;
    case AstOperator::eAssign   :            m_builder.CreateStore (operandIr, resultIr            );
                                  resultIr = m_void; break;
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

void IrGen::visit(AstNumber& number) {
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

void IrGen::visit(AstSymbol& symbol) {
  SymbolTableEntry*const stentry = symbol.stentry();
  assert( stentry );
  assert( stentry->valueIr() );

  Value* resultIr = NULL;
  if (stentry->objType().qualifiers() & ObjType::eMutable) {
    // stentry->valueIr() is the pointer returned by alloca corresponding to
    // the symbol
    if (symbol.access()==eWrite) {
      resultIr = stentry->valueIr(); 
    } else {
      resultIr = m_builder.CreateLoad(stentry->valueIr(), symbol.name().c_str());
    }
  } else {
    // stentry->valueIr() is directly the llvm::Value of the symbol
    resultIr = stentry->valueIr();
  }
  symbol.setIrValue(resultIr);
}

void IrGen::visit(AstFunDef& funDef) {
  visit(funDef.decl());
  Function* functionIr = funDef.decl().irFunction();
  assert(functionIr);

  if ( m_builder.GetInsertBlock() ) {
    m_BasicBlockStack.push(m_builder.GetInsertBlock());
  }
  m_builder.SetInsertPoint(
    BasicBlock::Create(getGlobalContext(), "entry", functionIr));

  // Add all arguments to the symbol table and create their allocas.
  Function::arg_iterator iterIr = functionIr->arg_begin();
  list<AstArgDecl*>::const_iterator iterAst = funDef.decl().args().begin();
  for (/*nop*/; iterIr != functionIr->arg_end(); ++iterIr, ++iterAst) {
    AllocaInst* alloca = createAllocaInEntryBlock(functionIr,
      (*iterAst)->name(), (*iterAst)->objType().llvmType());
    m_builder.CreateStore(iterIr, alloca);
    (*iterAst)->stentry()->setValueIr(alloca);
  }

  Value* ret = callAcceptOn( funDef.body());
  if ( ret == m_void ) {
    m_builder.CreateRetVoid();
  } else {
    m_builder.CreateRet( ret);
  }

  //verifyFunction(*functionIr);

  if ( !m_BasicBlockStack.empty() ) {
    m_builder.SetInsertPoint(m_BasicBlockStack.top());
    m_BasicBlockStack.pop();
  }

  funDef.setIrFunction( functionIr );
}

void IrGen::visit(AstFunDecl& funDecl) {
  // currently rettype and type of args is always int

  // It is required that an earlier pass did insert this AstFundDecl into the
  // environment
  assert(funDecl.stentry());

  // If there is not yet an empty IR function associated to the stentry, to it
  // now.
  if ( ! funDecl.stentry()->valueIr() ) {
    // create IR function with given name and signature
    vector<Type*> argsIr;
    for ( const auto& arg : funDecl.args() ) {
      argsIr.push_back(arg->objType().llvmType());
    }
    Type* retTypeIr = funDecl.retObjType().llvmType();
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

    funDecl.stentry()->setValueIr(functionIr);
    funDecl.setIrFunction(functionIr);
  }

  // An earlier declaration of this function, i.e. one with the same stentry,
  // allready created the empty IR function
  else {
    funDecl.setIrValue(funDecl.stentry()->valueIr());
  }
}

void IrGen::visit(AstFunCall& funCall) {
  Function* callee = dynamic_cast<Function*>(callAcceptOn(funCall.address()));
  assert(callee);
    
  const list<AstValue*>& argsAst = funCall.args().childs();

  vector<Value*> argsIr;
  for ( list<AstValue*>::const_iterator i = argsAst.begin();
        i != argsAst.end(); ++i ) {
    Value* argIr = callAcceptOn(**i);
    assert(argIr);
    argsIr.push_back(argIr);
  }

  funCall.setIrValue(m_builder.CreateCall(callee, argsIr, "calltmp"));
}

void IrGen::visit(AstDataDecl& dataDecl) {
  // nop -- dataDecl.valueIr() internally allready refers to
  // llvm::Value* of its associated symbol table entry
}

void IrGen::visit(AstArgDecl& argDecl) {
  visit( dynamic_cast<AstDataDecl&>(argDecl));
}

void IrGen::visit(AstDataDef& dataDef) {
  // process data declaration. That ensures an entry in the symbol table
  callAcceptOn(dataDef.decl());
  SymbolTableEntry*const stentry = dataDef.decl().stentry();
  assert(stentry);

  // define m_value (type Value*) of symbol table entry. For values that is
  // trivial. For variables aka allocas first an alloca has to be created.
  Value* initValue = callAcceptOn(dataDef.initValue());
  assert(initValue);
  if ( dataDef.objType().qualifiers() & ObjType::eMutable ) {
    Function* functionIr = m_builder.GetInsertBlock()->getParent();
    assert(functionIr);
    AllocaInst* alloca =
      createAllocaInEntryBlock(functionIr, dataDef.decl().name(),
        dataDef.objType().llvmType());
    assert(alloca);
    m_builder.CreateStore(initValue, alloca);
    stentry->setValueIr(alloca);
    dataDef.setIrValue(dataDef.access()==eRead ? initValue : alloca);
  } else {
    stentry->setValueIr(initValue);
    dataDef.setIrValue(initValue);
  }
}

void IrGen::visit(AstIf& if_) {
  // setup needed basic blocks
  Function* functionIr = m_builder.GetInsertBlock()->getParent();
  BasicBlock* ThenFirstBB = BasicBlock::Create(getGlobalContext(), "ifthen", functionIr);
  BasicBlock* ElseFirstBB = BasicBlock::Create(getGlobalContext(), "ifelse");
  BasicBlock* MergeBB = BasicBlock::Create(getGlobalContext(), "ifmerge");

  // IR for evaluate condition
  Value* condIr = callAcceptOn(if_.condition());
  assert(condIr);
  Value* condCmpIr = m_builder.CreateICmpNE(condIr,
    ConstantInt::get(getGlobalContext(), APInt(1, 0)), "ifcond");

  // IR for branch based on condition
  m_builder.CreateCondBr(condCmpIr, ThenFirstBB, ElseFirstBB);

  // IR for then clause
  m_builder.SetInsertPoint(ThenFirstBB);
  Value* thenValue = callAcceptOn(if_.action());
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
    // KLUDGE: int32 as type of else branch is hard coded, it needs to be a
    // dynamic type
    elseValue = ConstantInt::get( getGlobalContext(), APInt(32, 0));
  }
  assert(elseValue);
  m_builder.CreateBr(MergeBB);
  BasicBlock* ElseLastBB = m_builder.GetInsertBlock();

  // IR for merge of then/else clauses
  functionIr->getBasicBlockList().push_back(MergeBB);
  m_builder.SetInsertPoint(MergeBB);
  PHINode* phi = m_builder.CreatePHI( if_.objType().llvmType(), 2,
    "ifphi");
  assert(phi);
  phi->addIncoming(thenValue, ThenLastBB);
  phi->addIncoming(elseValue, ElseLastBB);
  if_.setIrValue(phi);
}

/** We want allocas in the entry block to facilitate llvm's mem2reg pass.*/
AllocaInst* IrGen::createAllocaInEntryBlock(Function* functionIr,
  const string &varName, llvm::Type* type) {
  IRBuilder<> irBuilder(&functionIr->getEntryBlock(),
    functionIr->getEntryBlock().begin());
  return irBuilder.CreateAlloca(type, 0, varName.c_str());
}
