#include "irgen.h"
#include "ast.h"
#include "env.h"
#include "errorhandler.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_os_ostream.h"
#include <algorithm>
#include <cstdlib>
#include <stdexcept>
#include <sstream>
using namespace std;
using namespace llvm;

Value* const IrGen::m_abstractObject = reinterpret_cast<Value*>(0xFFFFFFFF);

void IrGen::staticOneTimeInit() {
  InitializeNativeTarget();
  InitializeNativeTargetAsmPrinter();
  InitializeNativeTargetAsmParser();
}

IrGen::IrGen(ErrorHandler& errorHandler) :
  m_builder(getGlobalContext()),
  m_errorHandler(errorHandler) {
}

/** Using the given AST, generates LLVM IR code, appending it to the one
implict LLVM module associated with this IrGen object.  At the top level of
the AST, only declarations or definitions are allowed.
\pre SemanticAnalizer must have massaged the AST and the Env */
unique_ptr<Module> IrGen::genIr(AstNode& root) {
  m_module = std::make_unique<Module>("Main", getGlobalContext());

  root.accept(*this);

  stringstream ss;
  llvm::raw_os_ostream llvmss(ss);
  if (verifyModule(*m_module, &llvmss)) {
    llvmss.flush();
    ss << "\n--- llvm module dump: ------------------\n";
    m_module->print(llvmss, nullptr);
    ss << "----------------------------------------\n";
    throw runtime_error(ss.str());
  }

  return move(m_module);
}

llvm::Value* IrGen::callAcceptOn(AstValue& node) {
  node.accept(*this);
  return node.stentry()->irValue(m_builder);
}

void IrGen::visit(AstNop& nop) {
  nop.stentry()->setIrValue(m_abstractObject, m_builder);//void
}

void IrGen::visit(AstBlock& block) {
  block.stentry()->setIrValue(callAcceptOn(block.body()), m_builder);
}

void IrGen::visit(AstCast& cast) {
  auto childIr = callAcceptOn(cast.child());
  assert(childIr);
  llvm::Value* irResult = NULL;

  // At this point, AstCast is always between fundamental types
  const auto& oldtype = dynamic_cast<const ObjTypeFunda&>(cast.child().objType());
  const auto& newtype = dynamic_cast<const ObjTypeFunda&>(cast.objType());
  auto oldsize = oldtype.size();
  auto newsize = newtype.size();
  string irValueName = childIr->getName().str() + "_as_" + newtype.toStr();

  // unity conversion
  if (newtype.type() == oldtype.type()) {
    irResult = childIr; // i.e. a nop
  }

  // eStoredAsIntegral <-> eStoredAsIntegral
  else if ( oldtype.is(ObjType::eStoredAsIntegral) && newtype.is(ObjType::eStoredAsIntegral)) {
    // from bool
    if (oldsize == 1) {
      irResult = m_builder.CreateZExt( childIr, newtype.llvmType(), irValueName);
    }
    // to bool
    else if (newsize == 1) {
      assert(oldsize==32);
      irResult = m_builder.CreateICmpNE(childIr,
          ConstantInt::get(getGlobalContext(), APInt(oldsize, 0)), irValueName);
    }
    // between non-bool integrals, smaller -> larger
    else if (oldsize < newsize) {
      assert(oldsize==8);// implies char, which implies unsigned
      assert(newsize==32); //  implies int, which implies signed
      irResult = m_builder.CreateZExt( childIr, newtype.llvmType(), irValueName);
    }
    // between non-bool integrals, larger -> smaller
    else {
      assert(oldsize==32); //  implies int, which implies signed
      assert(newsize==8);// implies char, which implies unsigned
      irResult = m_builder.CreateTrunc( childIr, newtype.llvmType(), irValueName);
    }
  }

  // eStoredAsIntegral -> double
  else if (oldtype.is(ObjType::eStoredAsIntegral)
    && newtype.type() == ObjTypeFunda::eDouble) {
    // unsigned types: bool, char
    if ( oldsize==1 || oldsize==8 ) {
      irResult = m_builder.CreateUIToFP( childIr, newtype.llvmType(), irValueName);
    }
    // signed types: int
    else {
      assert(oldsize==32);
      irResult = m_builder.CreateSIToFP( childIr, newtype.llvmType(), irValueName);
    }
  }

  // double -> eStoredAsIntegral
 else if (oldtype.type() == ObjTypeFunda::eDouble
    && newtype.is(ObjType::eStoredAsIntegral)) {
    // bool
    if ( newsize==1 ) {
      irResult = m_builder.CreateFCmpONE( childIr,
          ConstantFP::get(getGlobalContext(), APFloat(0.0)), irValueName);
    }
    // unsigned types: char
    else if ( newsize==8 ) {
      irResult = m_builder.CreateFPToUI( childIr, newtype.llvmType(), irValueName);
    }
    // signed types: int
    else {
      assert(newsize==32);
      irResult = m_builder.CreateFPToSI( childIr, newtype.llvmType(), irValueName);
    }
  }

  // invaid casts should be catched by semantic analizer
  else {
    assert(false);
  }

  cast.stentry()->irInitLocal( irResult, m_builder, irValueName );
}

void IrGen::visit(AstCtList&) {
  assert(false); // parent of AstCtList shall not decent run accept on AstCtList
}

void IrGen::visit(AstOperator& op) {
  const auto& astOperands = op.args().childs();
  Value* llvmResult = NULL;

  // unary non-arithmetic operators
  if (op.op()==AstOperator::eNot) {
    llvmResult = m_builder.CreateNot(callAcceptOn(*astOperands.front()), "not" );
  } else if (op.op()==AstOperator::eAddrOf) {
    astOperands.front()->accept(*this);
    llvmResult = astOperands.front()->stentry()->irAddr();
  } else if (op.op()==AstOperator::eDeref) {
    auto llvmAddrOfDerefee = callAcceptOn(*astOperands.front());
    // auto llvmm_builder.CreateLoad(llvmAddrOfDerefee, "deref"); 
    // op.stentry is semantically a redunant copy of the stentry denoting the
    // derefee. 'Copy' as good as we can.
    op.stentry()->setIrAddr(llvmAddrOfDerefee);
  }

  // binary logical short circuit operators
  else if (op.op()==AstOperator::eAnd || op.op()==AstOperator::eOr) {
    const string opname = op.op()==AstOperator::eAnd ? "and" : "or";
    Function* functionIr = m_builder.GetInsertBlock()->getParent();
    BasicBlock* rhsBB = BasicBlock::Create(getGlobalContext(),
      opname + "_rhs");
    BasicBlock* mergeBB = BasicBlock::Create(getGlobalContext(),
      opname + "_merge");

    // current/lhs BB:
    auto llvmLhs = callAcceptOn(*astOperands.front());
    assert(llvmLhs);
    if ( op.op()==AstOperator::eAnd ) {
      m_builder.CreateCondBr(llvmLhs, rhsBB, mergeBB);
    } else {
      m_builder.CreateCondBr(llvmLhs, mergeBB, rhsBB);
    }
    BasicBlock* lhsLastBB = m_builder.GetInsertBlock();

    // rhsBB:
    functionIr->getBasicBlockList().push_back(rhsBB);
    m_builder.SetInsertPoint(rhsBB);
    auto llvmRhs = callAcceptOn(*astOperands.back());
    m_builder.CreateBr(mergeBB);
    BasicBlock* rhsLastBB = m_builder.GetInsertBlock();

    // mergeBB:
    functionIr->getBasicBlockList().push_back(mergeBB);
    m_builder.SetInsertPoint(mergeBB);
    PHINode* phi = m_builder.CreatePHI( Type::getInt1Ty(getGlobalContext()),
      2, opname);
    assert(phi);
    phi->addIncoming(llvmLhs, lhsLastBB);
    phi->addIncoming(llvmRhs, rhsLastBB);
    llvmResult = phi;
  }

  // assignment operators
  else if (op.op()==AstOperator::eDotAssign || op.op()==AstOperator::eAssign) {
    astOperands.front()->accept(*this);
    auto llvmRhs = callAcceptOn(*astOperands.back());
    m_builder.CreateStore( llvmRhs, astOperands.front()->stentry()->irAddr());
    if ( op.op()==AstOperator::eAssign ) {
      llvmResult = m_abstractObject; // void
    } else {
      // op.stetry() is the same as astOperands.front().stentry(), so
      // 'returning the result' is now a nop.
    }
  }

  // seq operator: evaluate lhs ignoring the result, then evaluate rhs and
  // then return that result. op.stetry() is the same as
  // astOperands.back().stentry(), so 'returning the result' is now a nop.
  else if (op.op()==AstOperator::eSeq) {
    astOperands.front()->accept(*this);
    astOperands.back()->accept(*this);
  }

  // binary arithmetic operators
  else if (astOperands.size()==2) {
    auto llvmLhs = callAcceptOn(*astOperands.front());
    auto llvmRhs = callAcceptOn(*astOperands.back());
    if (astOperands.front()->objType().is(ObjType::eStoredAsIntegral)) {
      switch (op.op()) {
      case AstOperator::eSub      : llvmResult = m_builder.CreateSub   (llvmLhs, llvmRhs, "sub"); break;
      case AstOperator::eAdd      : llvmResult = m_builder.CreateAdd   (llvmLhs, llvmRhs, "add"); break;
      case AstOperator::eMul      : llvmResult = m_builder.CreateMul   (llvmLhs, llvmRhs, "mul"); break;
      case AstOperator::eDiv      : llvmResult = m_builder.CreateSDiv  (llvmLhs, llvmRhs, "div"); break;
      case AstOperator::eEqualTo  : llvmResult = m_builder.CreateICmpEQ(llvmLhs, llvmRhs, "cmp"); break;
      default: assert(false);
      }
    } else {
      switch (op.op()) {
      case AstOperator::eSub      : llvmResult = m_builder.CreateFSub   (llvmLhs, llvmRhs, "fsub"); break;
      case AstOperator::eAdd      : llvmResult = m_builder.CreateFAdd   (llvmLhs, llvmRhs, "add"); break;
      case AstOperator::eMul      : llvmResult = m_builder.CreateFMul   (llvmLhs, llvmRhs, "mul"); break;
      case AstOperator::eDiv      : llvmResult = m_builder.CreateFDiv   (llvmLhs, llvmRhs, "div"); break;
      case AstOperator::eEqualTo  : llvmResult = m_builder.CreateFCmpOEQ(llvmLhs, llvmRhs, "cmp"); break;
      default: assert(false);
      }
    }
    assert(llvmResult);
  }

  // unary arithmetic operators
  else  {
    assert(astOperands.size()==1);
    const auto& operand = astOperands.front();
    const auto& objType = operand->objType();
    auto llvmOperand = callAcceptOn(*operand);

    if (objType.is(ObjType::eStoredAsIntegral)) {
      auto llvmZero = ConstantInt::get(getGlobalContext(), APInt(objType.size(), 0));
      switch (op.op()) {
      case '-': llvmResult = m_builder.CreateSub   (llvmZero, llvmOperand, "neg"); break;
      default: assert(false);
      }
    } else {
      auto llvmZero = ConstantFP::get(getGlobalContext(), APFloat(0.0));
      switch (op.op()) {
      case '-': llvmResult = m_builder.CreateFSub  (llvmZero, llvmOperand, "fneg"); break;
      default: assert(false);
      }
    }
    assert(llvmResult);
  }

  if ( llvmResult ) {
    op.stentry()->irInitLocal(llvmResult, m_builder);
  }
}

void IrGen::visit(AstNumber& number) {
  Value* value = nullptr;
  switch (number.objType().type()) { 
  case ObjTypeFunda::eChar: // fall through
  case ObjTypeFunda::eInt:  // fall through
  case ObjTypeFunda::eBool:
    value = ConstantInt::get( getGlobalContext(),
      APInt(number.objType().size(), number.value()));
    break;
  case ObjTypeFunda::eDouble:
    value = ConstantFP::get( getGlobalContext(), APFloat(number.value()));
    break;
  default:
    assert(false);
  }
  number.stentry()->irInitLocal(value, m_builder, "literal");
}

void IrGen::visit(AstSymbol& symbol) {
  // nop - irValue of object denoted by symbol.stentry() has been set/stored
  // before and will be querried later by caller
}

void IrGen::visit(AstFunDef& funDef) {
  visit(funDef.decl());
  auto functionIr = static_cast<Function*>(funDef.stentry()->irAddr());
  assert(functionIr);

  if ( m_builder.GetInsertBlock() ) {
    m_BasicBlockStack.push(m_builder.GetInsertBlock());
  }
  m_builder.SetInsertPoint(
    BasicBlock::Create(getGlobalContext(), "entry", functionIr));

  // Add all arguments to the symbol table and create their allocas.
  Function::arg_iterator llvmArgIter = functionIr->arg_begin();
  list<AstArgDecl*>::const_iterator astArgIter = funDef.decl().args().begin();
  for (/*nop*/; llvmArgIter != functionIr->arg_end(); ++llvmArgIter, ++astArgIter) {
    auto stentry = (*astArgIter)->stentry();
    if ( stentry->isStoredInMemory() ) {
      AllocaInst* argAddr = createAllocaInEntryBlock(functionIr,
        (*astArgIter)->name(), stentry->objType().llvmType());
      stentry->setIrAddr(argAddr);
    }
    stentry->setIrValue(llvmArgIter, m_builder);
  }

  Value* bodyVal = callAcceptOn( funDef.body());
  assert( bodyVal);
  if ( funDef.body().objType().isVoid() ) {
    m_builder.CreateRetVoid();
  } else if ( ! funDef.body().objType().isNoreturn() )  {
    m_builder.CreateRet( bodyVal);
  }

  if ( !m_BasicBlockStack.empty() ) {
    m_builder.SetInsertPoint(m_BasicBlockStack.top());
    m_BasicBlockStack.pop();
  }

  // stentry().irValue was already set by AstFunDecl
}

void IrGen::visit(AstFunDecl& funDecl) {
  // An earlier AstFunDecl node declaring the same function allready did the
  // job, so there's nothing to do anymore
  if ( funDecl.stentry()->irAddr() ) {
    return;
  }

  // create IR function with given name and signature
  vector<Type*> llvmArgs;
  for ( const auto& astArg : funDecl.args() ) {
    llvmArgs.push_back(astArg->declaredObjType().llvmType());
  }
  auto llvmFunctionType = FunctionType::get(
    funDecl.retObjType().llvmType(), llvmArgs, false);
  auto llvmFunction = Function::Create( llvmFunctionType,
    Function::ExternalLinkage, funDecl.name(), m_module.get() );
  assert(llvmFunction);

  // If the names differ (see condition of if statement below) that means a
  // function with that name already existed, so LLVM automaticaly chose a new
  // name. That cannot be since above our environment said the name is unique.
  assert(llvmFunction->getName() == funDecl.name());

  // set names of arguments of IR function
  auto astArgIter = funDecl.args().begin();
  auto llvmArgIter = llvmFunction->arg_begin();
  for (/*nop*/; astArgIter!=funDecl.args().end(); ++astArgIter, ++llvmArgIter) {
    llvmArgIter->setName((*astArgIter)->name());
  }

  funDecl.stentry()->setIrAddr(llvmFunction);
}

void IrGen::visit(AstFunCall& funCall) {
  funCall.address().accept(*this);
  Function* callee = static_cast<Function*>(funCall.address().stentry()->irAddr());
  assert(callee);
    
  const auto& astArgs = funCall.args().childs();

  vector<Value*> llvmArgs;
  for ( const auto& astArg : astArgs ) {
    Value* llvmArg = callAcceptOn(*astArg);
    assert(llvmArg);
    llvmArgs.push_back(llvmArg);
  }

  const ObjTypeFun& objTypeFun = dynamic_cast<const ObjTypeFun&>(funCall.address().objType());
  if ( objTypeFun.ret().isVoid() ) {
    m_builder.CreateCall(callee, llvmArgs);
    funCall.stentry()->irInitLocal(m_abstractObject, m_builder);
  } else {
    funCall.stentry()->irInitLocal(
      m_builder.CreateCall(callee, llvmArgs, callee->getName()), m_builder);
  }
}

void IrGen::visit(AstDataDecl& dataDecl) {
  // nop -- dataDecl.irValue() internally allready refers to
  // llvm::Value* of its associated symbol table entry
}

void IrGen::visit(AstArgDecl& argDecl) {
  visit( dynamic_cast<AstDataDecl&>(argDecl));
}

void IrGen::visit(AstDataDef& dataDef) {
  // process data declaration. That ensures an entry in the symbol table
  dataDef.decl().accept(*this);
  SymbolTableEntry*const stentry = dataDef.stentry();
  assert(stentry);

  // define m_value (type Value*) of symbol table entry. For values that is
  // trivial. For variables aka allocas first an alloca has to be created.
  Value* initValue = callAcceptOn(dataDef.initValue());
  assert(initValue);
  const ObjType& objType = dataDef.objType();

  if ( objType.storageDuration() == ObjType::eStatic ) {
    GlobalVariable* variableAddr = new GlobalVariable( *m_module, objType.llvmType(),
      ! objType.qualifiers() & ObjType::eMutable, GlobalValue::InternalLinkage,
      static_cast<Constant*>(initValue),
      dataDef.decl().name());
    stentry->setIrAddr(variableAddr);
    // don't stentry->setIrAddr(...) since the initialization of a static
    // variable shall not occur again at run-time when controll flow reaches
    // the data definition expresssion.
  }

  else {
    stentry->irInitLocal(initValue, m_builder, dataDef.decl().name());
  }
}

void IrGen::visit(AstIf& if_) {
  // setup needed basic blocks
  Function* functionIr = m_builder.GetInsertBlock()->getParent();
  BasicBlock* ThenFirstBB = BasicBlock::Create(getGlobalContext(), "if_then", functionIr);
  BasicBlock* ElseFirstBB = BasicBlock::Create(getGlobalContext(), "if_else");
  BasicBlock* MergeBB = BasicBlock::Create(getGlobalContext(), "if_merge");

  // current BB:
  Value* condIr = callAcceptOn(if_.condition());
  assert(condIr);
  m_builder.CreateCondBr(condIr, ThenFirstBB, ElseFirstBB);

  // thenFirstBB:
  m_builder.SetInsertPoint(ThenFirstBB);
  Value* thenValue = callAcceptOn(if_.action());
  assert(thenValue);
  if ( ! if_.action().objType().isNoreturn() ) {
    m_builder.CreateBr(MergeBB);
  }
  BasicBlock* ThenLastBB = m_builder.GetInsertBlock();

  // elseFirstBB:
  functionIr->getBasicBlockList().push_back(ElseFirstBB);
  m_builder.SetInsertPoint(ElseFirstBB);
  Value* elseValue = NULL;
  if ( if_.elseAction() ) {
    elseValue = callAcceptOn(*if_.elseAction());
    assert(elseValue);
    if ( ! if_.elseAction()->objType().isNoreturn() ) {
      m_builder.CreateBr(MergeBB);
    }
  } else {
    elseValue = m_abstractObject;
    m_builder.CreateBr(MergeBB);
  }
  BasicBlock* ElseLastBB = m_builder.GetInsertBlock();

  // mergeBB:
  // also sets IrValue of this AstIf
  functionIr->getBasicBlockList().push_back(MergeBB);
  m_builder.SetInsertPoint(MergeBB);
  if ( thenValue!=m_abstractObject && elseValue!=m_abstractObject ) {
    PHINode* phi = m_builder.CreatePHI( thenValue->getType(), 2, "if_phi");
    assert(phi);
    phi->addIncoming(thenValue, ThenLastBB);
    phi->addIncoming(elseValue, ElseLastBB);
    if_.stentry()->irInitLocal(phi, m_builder);
  } else {
    if_.stentry()->irInitLocal(m_abstractObject, m_builder);
  }
}

void IrGen::visit(AstLoop& loop) {
  // setup needed basic blocks
  Function* functionIr = m_builder.GetInsertBlock()->getParent();
  BasicBlock* condBB = BasicBlock::Create(getGlobalContext(), "loop_cond");
  BasicBlock* bodyBB = BasicBlock::Create(getGlobalContext(), "loop_body");
  BasicBlock* afterBB = BasicBlock::Create(getGlobalContext(), "after_loop");

  // current BB:
  m_builder.CreateBr(condBB);

  // condBB:
  functionIr->getBasicBlockList().push_back(condBB);
  m_builder.SetInsertPoint(condBB);
  Value* condIr = callAcceptOn(loop.condition());
  assert(condIr);
  m_builder.CreateCondBr(condIr, bodyBB, afterBB);

  // bodyBB:
  functionIr->getBasicBlockList().push_back(bodyBB);
  m_builder.SetInsertPoint(bodyBB);
  callAcceptOn(loop.body());
  if ( ! loop.body().objType().isNoreturn() ) {
    m_builder.CreateBr(condBB);
  }

  // afterBB:
  functionIr->getBasicBlockList().push_back(afterBB);
  m_builder.SetInsertPoint(afterBB);

  //
  loop.stentry()->irInitLocal(m_abstractObject, m_builder);
}

void IrGen::visit(AstReturn& return_) {
  Value* retVal = callAcceptOn( return_.retVal());
  assert( retVal);
  if ( return_.retVal().objType().isVoid() ) {
    m_builder.CreateRetVoid();
  } else {
    m_builder.CreateRet( retVal);
  }
  return_.stentry()->irInitLocal(m_abstractObject, m_builder);
}

/** \internal We want allocas in the entry block to facilitate llvm's mem2reg
pass.*/
AllocaInst* IrGen::createAllocaInEntryBlock(Function* functionIr,
  const string &varName, llvm::Type* type) {
  IRBuilder<> irBuilder(&functionIr->getEntryBlock(),
    functionIr->getEntryBlock().begin());
  return irBuilder.CreateAlloca(type, 0, varName.c_str());
}
