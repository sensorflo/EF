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
  const auto& oldtype = cast.child().objType();
  const auto& newtype = cast.objType();
  auto oldsize = oldtype.size();
  auto newsize = newtype.size();
  string irValueName = childIr->getName().str() + "_as_" + newtype.toStr();

  // unity conversion
  if (newtype.matchesSaufQualifiers(oldtype)) {
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
    // between non-bool integrals
    else {
      // none currently
      assert(false);
    }
  }

  // eStoredAsIntegral -> double
  else if (oldtype.is(ObjType::eStoredAsIntegral)
    && newtype.matchesSaufQualifiers(ObjTypeFunda(ObjTypeFunda::eDouble))) {
    // unsigned types: bool
    if ( oldsize==1 ) {
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
  else if (oldtype.matchesSaufQualifiers(ObjTypeFunda(ObjTypeFunda::eDouble))
    && newtype.is(ObjType::eStoredAsIntegral)) {
    // bool
    if ( newsize==1 ) {
      cast.setIrValue( m_builder.CreateFCmpONE( childIr,
          ConstantFP::get(getGlobalContext(), APFloat(0.0)), irValueName));
    }

    // unsigned types: none currently

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
  const list<AstValue*>& argschilds = op.args().childs();
  Value* resultIr = NULL;

  // unary operators
  if (op.op()==AstOperator::eNot) {
    resultIr = m_builder.CreateNot(callAcceptOn(*argschilds.front()), "not" );
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
    auto lhsIr = callAcceptOn(*argschilds.front());
    assert(lhsIr);
    if ( op.op()==AstOperator::eAnd ) {
      m_builder.CreateCondBr(lhsIr, rhsBB, mergeBB);
    } else {
      m_builder.CreateCondBr(lhsIr, mergeBB, rhsBB);
    }
    BasicBlock* lhsLastBB = m_builder.GetInsertBlock();

    // rhsBB:
    functionIr->getBasicBlockList().push_back(rhsBB);
    m_builder.SetInsertPoint(rhsBB);
    auto rhsIr = callAcceptOn(*argschilds.back());
    m_builder.CreateBr(mergeBB);
    BasicBlock* rhsLastBB = m_builder.GetInsertBlock();

    // mergeBB:
    functionIr->getBasicBlockList().push_back(mergeBB);
    m_builder.SetInsertPoint(mergeBB);
    PHINode* phi = m_builder.CreatePHI( Type::getInt1Ty(getGlobalContext()),
      2, opname);
    assert(phi);
    phi->addIncoming(lhsIr, lhsLastBB);
    phi->addIncoming(rhsIr, rhsLastBB);
    resultIr = phi;
  }

  // binary non short circuit operators
  else {
    auto lhsIr = callAcceptOn(*argschilds.front());
    auto rhsIr = callAcceptOn(*argschilds.back());
    switch (op.op()) {
    case AstOperator::eDotAssign:            m_builder.CreateStore (rhsIr, lhsIr);
                                  resultIr = op.access()==eWrite ? lhsIr : rhsIr; break;
    case AstOperator::eAssign   :            m_builder.CreateStore (rhsIr, lhsIr);
                                  resultIr = m_abstractObject; break;
    case AstOperator::eSeq      : resultIr = rhsIr; break;
    default: 
      if (argschilds.front()->objType().is(ObjType::eStoredAsIntegral)) {
        switch (op.op()) {
        case AstOperator::eSub      : resultIr = m_builder.CreateSub   (lhsIr, rhsIr, "sub"); break;
        case AstOperator::eAdd      : resultIr = m_builder.CreateAdd   (lhsIr, rhsIr, "add"); break;
        case AstOperator::eMul      : resultIr = m_builder.CreateMul   (lhsIr, rhsIr, "mul"); break;
        case AstOperator::eDiv      : resultIr = m_builder.CreateSDiv  (lhsIr, rhsIr, "div"); break;
        case AstOperator::eEqualTo  : resultIr = m_builder.CreateICmpEQ(lhsIr, rhsIr, "cmp"); break;
        default: assert(false);
        }
      } else {
        switch (op.op()) {
        case AstOperator::eSub      : resultIr = m_builder.CreateFSub   (lhsIr, rhsIr, "fsub"); break;
        case AstOperator::eAdd      : resultIr = m_builder.CreateFAdd   (lhsIr, rhsIr, "add"); break;
        case AstOperator::eMul      : resultIr = m_builder.CreateFMul   (lhsIr, rhsIr, "mul"); break;
        case AstOperator::eDiv      : resultIr = m_builder.CreateFDiv   (lhsIr, rhsIr, "div"); break;
        case AstOperator::eEqualTo  : resultIr = m_builder.CreateFCmpOEQ(lhsIr, rhsIr, "cmp"); break;
        default: assert(false);
        }
      }
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
  } else if ( number.objType().match(ObjTypeFunda(ObjTypeFunda::eDouble))) {
    number.setIrValue(ConstantFP::get( getGlobalContext(),
        APFloat(number.value())));
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
  assert( ret);
  if ( funDef.body().objType().isVoid() ) {
    m_builder.CreateRetVoid();
  } else if ( ! funDef.body().objType().isNoreturn() )  {
    m_builder.CreateRet( ret);
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
    vector<Type*> argsIr;
    for ( const auto& arg : funDecl.args() ) {
      argsIr.push_back(arg->objType().llvmType());
    }
    Type* retTypeIr = funDecl.retObjType().llvmType();
    FunctionType* functionTypeIr = FunctionType::get(retTypeIr, argsIr, false);
    assert(functionTypeIr);
    Function* functionIr = Function::Create( functionTypeIr,
      Function::ExternalLinkage, funDecl.name(), m_module.get() );
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

  const ObjTypeFun& objTypeFun = dynamic_cast<const ObjTypeFun&>(funCall.address().objType());
  if ( objTypeFun.ret().isVoid() ) {
    m_builder.CreateCall(callee, argsIr);
    funCall.setIrValue(m_abstractObject);
  } else {
    funCall.setIrValue(m_builder.CreateCall(callee, argsIr, callee->getName()));
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
  Value* ret = callAcceptOn( return_.retVal());
  assert( ret);
  if ( return_.retVal().objType().isVoid() ) {
    m_builder.CreateRetVoid();
  } else {
    m_builder.CreateRet( ret);
  }
  return_.setIrValue(m_abstractObject);
}

/** We want allocas in the entry block to facilitate llvm's mem2reg pass.*/
AllocaInst* IrGen::createAllocaInEntryBlock(Function* functionIr,
  const string &varName, llvm::Type* type) {
  IRBuilder<> irBuilder(&functionIr->getEntryBlock(),
    functionIr->getEntryBlock().begin());
  return irBuilder.CreateAlloca(type, 0, varName.c_str());
}
