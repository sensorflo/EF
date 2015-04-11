#include "irgen.h"
#include "ast.h"
#include "env.h"
#include "errorhandler.h"
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
#include "memoryext.h"
using namespace std;
using namespace llvm;

Value* const IrGen::m_abstractObject = reinterpret_cast<Value*>(0xFFFFFFFF);

void IrGen::staticOneTimeInit() {
  InitializeNativeTarget();
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
  m_module = make_unique<Module>("Main", getGlobalContext());

  callAcceptOn(root);

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

llvm::Value* IrGen::callAcceptOn(AstNode& node) {
  node.accept(*this);
  return node.irValue();
}

void IrGen::visit(AstNop& nop) {
  nop.setIrValue(m_abstractObject);//void
}

void IrGen::visit(AstBlock& block) {
  block.setIrValue(callAcceptOn(block.body()));
}

void IrGen::visit(AstCast& cast) {
  auto childIr = callAcceptOn(cast.child());
  assert(childIr);
  // At this point, AstCast is always between fundamental types
  const auto& oldtype = dynamic_cast<const ObjTypeFunda&>(cast.child().objType());
  const auto& newtype = dynamic_cast<const ObjTypeFunda&>(cast.objType());
  auto oldsize = oldtype.size();
  auto newsize = newtype.size();
  string irValueName = childIr->getName().str() + "_as_" + newtype.toStr();

  // unity conversion
  if (newtype.type() == oldtype.type()) {
    cast.setIrValue( childIr ); // i.e. a nop
  }

  // eStoredAsIntegral <-> eStoredAsIntegral
  else if ( oldtype.is(ObjType::eStoredAsIntegral) && newtype.is(ObjType::eStoredAsIntegral)) {
    // from bool
    if (oldsize == 1) {
      cast.setIrValue( m_builder.CreateZExt( childIr, newtype.llvmType(),
          irValueName));
    }
    // to bool
    else if (newsize == 1) {
      assert(oldsize==32);
      cast.setIrValue( m_builder.CreateICmpNE(childIr,
          ConstantInt::get(getGlobalContext(), APInt(oldsize, 0)), irValueName));
    }
    // between non-bool integrals, smaller -> larger
    else if (oldsize < newsize) {
      assert(oldsize==8);// implies char, which implies unsigned
      assert(newsize==32); //  implies int, which implies signed
      cast.setIrValue( m_builder.CreateZExt( childIr, newtype.llvmType(),
          irValueName));
    }
    // between non-bool integrals, larger -> smaller
    else {
      assert(oldsize==32); //  implies int, which implies signed
      assert(newsize==8);// implies char, which implies unsigned
      cast.setIrValue( m_builder.CreateTrunc( childIr, newtype.llvmType(),
          irValueName));
    }
  }

  // eStoredAsIntegral -> double
  else if (oldtype.is(ObjType::eStoredAsIntegral)
    && newtype.type() == ObjTypeFunda::eDouble) {
    // unsigned types: bool, char
    if ( oldsize==1 || oldsize==8 ) {
      cast.setIrValue( m_builder.CreateUIToFP( childIr, newtype.llvmType(),
          irValueName));
    }
    // signed types: int
    else {
      assert(oldsize==32);
      cast.setIrValue( m_builder.CreateSIToFP( childIr, newtype.llvmType(),
          irValueName));
    }
  }

  // double -> eStoredAsIntegral
 else if (oldtype.type() == ObjTypeFunda::eDouble
    && newtype.is(ObjType::eStoredAsIntegral)) {
    // bool
    if ( newsize==1 ) {
      cast.setIrValue( m_builder.CreateFCmpONE( childIr,
          ConstantFP::get(getGlobalContext(), APFloat(0.0)), irValueName));
    }
    // unsigned types: char
    else if ( newsize==8 ) {
      cast.setIrValue( m_builder.CreateFPToUI( childIr, newtype.llvmType(),
          irValueName));
    }
    // signed types: int
    else {
      assert(newsize==32);
      cast.setIrValue( m_builder.CreateFPToSI( childIr, newtype.llvmType(),
          irValueName));
    }
  }

  // invaid casts should be catched by semantic analizer
  else {
    assert(false);
  }
}

void IrGen::visit(AstCtList&) {
  assert(false); // parent of AstCtList shall not decent run accept on AstCtList
}

void IrGen::visit(AstOperator& op) {
  const auto& astOperands = op.args().childs();
  Value* llvmResult = NULL;

  assert( AstOperator::eMemberAccess!=op.class_() ); // not yet implemented

  // unary operators
  if (op.op()==AstOperator::eNot) {
    llvmResult = m_builder.CreateNot(callAcceptOn(*astOperands.front()), "not" );
  }

  // binary short circuit operators
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

  // binary non short circuit operators
  else {
    auto llvmLhs = callAcceptOn(*astOperands.front());
    auto llvmRhs = callAcceptOn(*astOperands.back());
    switch (op.op()) {
    case AstOperator::eDotAssign:            m_builder.CreateStore (llvmRhs, llvmLhs);
                                  llvmResult = op.access()==eWrite ? llvmLhs : llvmRhs; break;
    case AstOperator::eAssign   :            m_builder.CreateStore (llvmRhs, llvmLhs);
                                  llvmResult = m_abstractObject; break;
    case AstOperator::eSeq      : llvmResult = llvmRhs; break;
    default: 
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
    }
  }

  assert(llvmResult);
  op.setIrValue(llvmResult);
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
  number.setIrValue(value);
}

void IrGen::visit(AstSymbol& symbol) {
  SymbolTableEntry*const stentry = symbol.stentry();
  assert( stentry );
  assert( stentry->valueIr() );

  Value* resultIr = NULL;

  if (stentry->objType().is(ObjType::eFunction)) {
    resultIr = stentry->valueIr();
  }

  else if ( stentry->objType().qualifiers() & ObjType::eMutable
    || stentry->objType().storageDuration() == ObjType::eStatic) {
    // write access wants the object's address as argument, thus return
    // exactly that
    if (symbol.access()==eWrite) {
      resultIr = stentry->valueIr(); 
    } else {
      resultIr = m_builder.CreateLoad(stentry->valueIr(), symbol.name().c_str());
    }
  }

  else {
    // KLUDGE: It is currently assumed that an immutable local data object
    // needs no storage and thus can be repressented as llvm::Value. See also
    // visit of AstDataDecl.
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
  Function::arg_iterator llvmArgIter = functionIr->arg_begin();
  list<AstArgDecl*>::const_iterator astArgIter = funDef.decl().args().begin();
  for (/*nop*/; llvmArgIter != functionIr->arg_end(); ++llvmArgIter, ++astArgIter) {
    AllocaInst* argAddr = createAllocaInEntryBlock(functionIr,
      (*astArgIter)->name(), (*astArgIter)->objType().llvmType());
    m_builder.CreateStore(llvmArgIter, argAddr);
    (*astArgIter)->stentry()->setValueIr(argAddr);
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
    vector<Type*> llvmArgs;
    for ( const auto& astArg : funDecl.args() ) {
      llvmArgs.push_back(astArg->objType().llvmType());
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

    funDecl.stentry()->setValueIr(llvmFunction);
    funDecl.setIrFunction(llvmFunction);
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
    funCall.setIrValue(m_abstractObject);
  } else {
    funCall.setIrValue(m_builder.CreateCall(callee, llvmArgs, callee->getName()));
  }
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
  const ObjType& objType = dataDef.objType();
  if ( objType.storageDuration() == ObjType::eStatic ) {
    GlobalVariable* variableAddr = new GlobalVariable( *m_module, objType.llvmType(),
      ! objType.qualifiers() & ObjType::eMutable, GlobalValue::InternalLinkage,
      static_cast<Constant*>(initValue),
      dataDef.decl().name());
    stentry->setValueIr(variableAddr);
    dataDef.setIrValue(dataDef.access()==eRead ? initValue : variableAddr);
  } else {
    if ( objType.qualifiers() & ObjType::eMutable ) {
      Function* functionIr = m_builder.GetInsertBlock()->getParent();
      assert(functionIr);
      AllocaInst* variableAddr =
        createAllocaInEntryBlock(functionIr, dataDef.decl().name(),
          objType.llvmType());
      assert(variableAddr);
      m_builder.CreateStore(initValue, variableAddr);
      stentry->setValueIr(variableAddr);
      dataDef.setIrValue(dataDef.access()==eRead ? initValue : variableAddr);
    } else {
      // KLUDGE: It is currently assumed that an immutable local data object
      // needs no storage and thus can be repressented as llvm::Value. However
      // this is in general not true. It fails if its address is taken;
      // currently there is no address of operator.
      stentry->setValueIr(initValue);
      dataDef.setIrValue(initValue);
    }
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
    if_.setIrValue(phi);
  } else {
    if_.setIrValue(m_abstractObject);
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
  loop.setIrValue(m_abstractObject);
}

void IrGen::visit(AstReturn& return_) {
  Value* retVal = callAcceptOn( return_.retVal());
  assert( retVal);
  if ( return_.retVal().objType().isVoid() ) {
    m_builder.CreateRetVoid();
  } else {
    m_builder.CreateRet( retVal);
  }
  return_.setIrValue(m_abstractObject);
}

/** \internal We want allocas in the entry block to facilitate llvm's mem2reg
pass.*/
AllocaInst* IrGen::createAllocaInEntryBlock(Function* functionIr,
  const string &varName, llvm::Type* type) {
  IRBuilder<> irBuilder(&functionIr->getEntryBlock(),
    functionIr->getEntryBlock().begin());
  return irBuilder.CreateAlloca(type, 0, varName.c_str());
}
