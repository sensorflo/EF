#include "irgen.h"
#include "ast.h"
#include "object.h"
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

llvm::Value* IrGen::callAcceptOn(AstObject& node) {
  node.accept(*this);
  return node.object()->irValue(m_builder);
}

void IrGen::visit(AstNop& nop) {
  nop.object()->setIrValue(m_abstractObject, m_builder);//void
}

void IrGen::visit(AstBlock& block) {
  block.object()->setIrValue(callAcceptOn(block.body()), m_builder);
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

  cast.object()->irInitLocal( irResult, m_builder, irValueName );
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
    llvmResult = astOperands.front()->object()->irAddr();
  } else if (op.op()==AstOperator::eDeref) {
    auto llvmAddrOfDerefee = callAcceptOn(*astOperands.front());
    // auto llvmm_builder.CreateLoad(llvmAddrOfDerefee, "deref"); 
    // op.object is semantically a redunant copy of the object denoting the
    // derefee. 'Copy' as good as we can.
    op.object()->setIrAddr(llvmAddrOfDerefee);
  }

  // binary logical short circuit operators
  else if (op.isBinaryLogicalShortCircuit()) {
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
    } else if ( op.op()==AstOperator::eOr )  {
      m_builder.CreateCondBr(llvmLhs, mergeBB, rhsBB);
    } else {
      assert(false);
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
    m_builder.CreateStore( llvmRhs, astOperands.front()->object()->irAddr());
    if ( op.op()==AstOperator::eAssign ) {
      llvmResult = m_abstractObject; // void
    } else {
      // op.object() is the same as astOperands.front().object(), so
      // 'returning the result' is now a nop.
    }
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
      case '+': llvmResult = llvmOperand; break;
      default: assert(false);
      }
    } else {
      auto llvmZero = ConstantFP::get(getGlobalContext(), APFloat(0.0));
      switch (op.op()) {
      case '-': llvmResult = m_builder.CreateFSub  (llvmZero, llvmOperand, "fneg"); break;
      case '+': llvmResult = llvmOperand; break;
      default: assert(false);
      }
    }
    assert(llvmResult);
  }

  if ( llvmResult ) {
    op.object()->irInitLocal(llvmResult, m_builder);
  }
}

void IrGen::visit(AstSeq& seq) {
  // evaluate all operands, but ignore all results but the result of the
  // last operand. seq.object() is the same as seq.operands.last().object(), so
  // 'returning the result' is a nop.
  for (const auto& op: seq.operands()) {
    op->accept(*this);
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
  number.object()->irInitLocal(value, m_builder, "literal");
}

void IrGen::visit(AstSymbol& symbol) {
  // nop - irValue of object denoted by symbol.object() has been set/stored
  // before and will be querried later by caller
}

void IrGen::visit(AstFunDef& funDef) {
  // create IR function with given name and signature
  vector<Type*> llvmArgs;
  for ( const auto& astArg : funDef.args() ) {
    llvmArgs.push_back(astArg->declaredObjType().llvmType());
  }
  auto llvmFunctionType = FunctionType::get(
    funDef.retObjType().llvmType(), llvmArgs, false);
  auto functionIr = Function::Create( llvmFunctionType,
    Function::ExternalLinkage, funDef.name(), m_module.get() );
  assert(functionIr);

  // If the names differ that means a function with that name already existed,
  // so LLVM automaticaly chose a new name. That cannot be since above our
  // environment said the name is unique.
  assert(functionIr->getName() == funDef.name());

  // Before accepting the body to facilitate recursive calls. As soon as EF
  // supports calling method which are defined later in the AST that should
  // not be an relevant issue anymore.
  funDef.object()->setIrAddr(functionIr);

  if ( m_builder.GetInsertBlock() ) {
    m_BasicBlockStack.push(m_builder.GetInsertBlock());
  }
  m_builder.SetInsertPoint(
    BasicBlock::Create(getGlobalContext(), "entry", functionIr));

  // Add all arguments to the symbol table and create their allocas. Also tell
  // llvm the name of each arg.
  Function::arg_iterator llvmArgIter = functionIr->arg_begin();
  list<AstDataDef*>::const_iterator astArgIter = funDef.args().begin();
  for (/*nop*/; llvmArgIter != functionIr->arg_end(); ++llvmArgIter, ++astArgIter) {
    auto object = (*astArgIter)->object();
    if ( object->isStoredInMemory() ) {
      AllocaInst* argAddr = createAllocaInEntryBlock(functionIr,
        (*astArgIter)->name(), object->objType().llvmType());
      object->setIrAddr(argAddr);
    }
    object->setIrValue(llvmArgIter, m_builder);

    llvmArgIter->setName((*astArgIter)->name());
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
}

void IrGen::visit(AstFunCall& funCall) {
  funCall.address().accept(*this);
  Function* callee = static_cast<Function*>(funCall.address().object()->irAddr());
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
    funCall.object()->irInitLocal(m_abstractObject, m_builder);
  } else {
    funCall.object()->irInitLocal(
      m_builder.CreateCall(callee, llvmArgs, callee->getName()), m_builder);
  }
}

void IrGen::visit(AstDataDef& dataDef) {
  // note that AstDataDef being function parameters are _not_ handled here but
  // in visit of AstFunDef

  Object*const object = dataDef.object();
  assert(object);

  const auto ctorArgs = dataDef.ctorArgs().childs();
  // currently a data object must be initialized withe exactly one initializer
  assert(ctorArgs.size()==1);

  // define m_value (type Value*) of symbol table entry. For values that is
  // trivial. For variables aka allocas first an alloca has to be created.
  Value* initObj = callAcceptOn(*ctorArgs.front());
  assert(initObj);
  const ObjType& objType = dataDef.objType();

  if ( object->storageDuration() == StorageDuration::eStatic ) {
    GlobalVariable* variableAddr = new GlobalVariable( *m_module, objType.llvmType(),
      ! objType.qualifiers() & ObjType::eMutable, GlobalValue::InternalLinkage,
      static_cast<Constant*>(initObj),
      dataDef.name());
    object->setIrAddr(variableAddr);
    // don't object->setIrAddr(...) since the initialization of a static
    // variable shall not occur again at run-time when controll flow reaches
    // the data definition expresssion.
  }

  else {
    object->irInitLocal(initObj, m_builder, dataDef.name());
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
    if_.object()->irInitLocal(phi, m_builder);
  } else {
    if_.object()->irInitLocal(m_abstractObject, m_builder);
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
  loop.object()->irInitLocal(m_abstractObject, m_builder);
}

void IrGen::visit(AstReturn& return_) {
  Value* retVal = callAcceptOn( return_.retVal());
  assert( retVal);
  if ( return_.retVal().objType().isVoid() ) {
    m_builder.CreateRetVoid();
  } else {
    m_builder.CreateRet( retVal);
  }
  return_.object()->irInitLocal(m_abstractObject, m_builder);
}

void IrGen::visit(AstClassDef& class_){
  assert(false); // not yet implemented
}

/** \internal We want allocas in the entry block to facilitate llvm's mem2reg
pass.*/
AllocaInst* IrGen::createAllocaInEntryBlock(Function* functionIr,
  const string &varName, llvm::Type* type) {
  IRBuilder<> irBuilder(&functionIr->getEntryBlock(),
    functionIr->getEntryBlock().begin());
  return irBuilder.CreateAlloca(type, 0, varName.c_str());
}
