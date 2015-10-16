#include "semanticanalizer.h"
#include "ast.h"
#include "astdefaultiterator.h"
#include "env.h"
#include "symboltableentry.h"
#include "errorhandler.h"
#include "objtype.h"
using namespace std;

SemanticAnalizer::SemanticAnalizer(Env& env, ErrorHandler& errorHandler) :
  m_env(env),
  m_errorHandler(errorHandler) {
}

void SemanticAnalizer::analyze(AstNode& root) {
  root.accept(*this);
}

void SemanticAnalizer::visit(AstCast& cast) {
  cast.child().accept(*this);
  // there's no need to set ObjType, AstCast knows its own ObjType

  // test if conversion is eligible
  if ( !cast.objType().matchesSaufQualifiers(cast.child().objType())
    && !cast.objType().hasConstructor(cast.child().objType()) ) {
    Error::throwError(m_errorHandler, Error::eNoSuchMember);
  }

  // no need to set stentry since that is done by AST node itself

  cast.stentry()->addAccessToObject(cast.access());

  postConditionCheck(cast);
}

void SemanticAnalizer::visit(AstNop& nop) {
  // no need to set stentry since that is done by AST node itself

  nop.stentry()->addAccessToObject(nop.access());

  postConditionCheck(nop);
}

void SemanticAnalizer::visit(AstBlock& block) {
  { Env::AutoScope scope(m_env);
    block.body().accept(*this);
  }

  shared_ptr<ObjType> objType{block.body().objType().clone()};
  objType->removeQualifiers(ObjType::eMutable);
  block.setStentry(make_shared<SymbolTableEntry>(objType, StorageDuration::eLocal));

  block.stentry()->addAccessToObject(block.access());

  postConditionCheck(block);
}

void SemanticAnalizer::visit(AstCtList& ctList) {
  list<AstValue*>& childs = ctList.childs();
  for (list<AstValue*>::const_iterator i=childs.begin();
       i!=childs.end(); ++i) {
    (*i)->accept(*this);
  }
}

void SemanticAnalizer::visit(AstOperator& op) {
  const list<AstValue*>& argschilds = op.args().childs();

  // Note that orrect number of arguments was already handled in AstOperator's
  // ctor; we're allowed to count on that.

  const auto class_ = op.class_();
  const auto opop = op.op();

  // Set EAccess of childs.
  {
    if ( AstOperator::eAssignment == class_) {
      argschilds.front()->setAccess(eWrite);
    } else if ( AstOperator::eSeq == opop ) {
      argschilds.front()->setAccess(eIgnore);
      argschilds.back()->setAccess(op.access());
    } else if ( opop==AstOperator::eAddrOf ) {
      argschilds.front()->setAccess(eTakeAddress);
    }
  }

  op.args().accept(*this);

  // Check that all operands are of the same obj type, sauf
  // qualifiers. Currently there are no implicit conversions. However the rhs
  // of logical and/or is allowed to be eNoreturn.
  if ( argschilds.size()==2 ) {
    auto& lhs = argschilds.front()->objType();
    auto& rhs = argschilds.back()->objType();
    if ( op.class_() == AstOperator::eLogical ) {
      if ( !lhs.matchesSaufQualifiers(rhs)
        && !rhs.matchesSaufQualifiers(ObjTypeFunda(ObjTypeFunda::eNoreturn))) {
        Error::throwError(m_errorHandler, Error::eNoImplicitConversion);
      }
    } else if ( class_ != AstOperator::eOther ) {
      if ( !lhs.matchesSaufQualifiers(rhs) ) {
        Error::throwError(m_errorHandler, Error::eNoImplicitConversion);
      }
    }
  } else {
    // In case there is only one arg, that arg's type can't missmatch
    // anything, so it's implicitely ok. If there are more than two args, the
    // implementation is missing.
    assert(argschilds.size()==1);
  }

  // If an operand is modified ensure its writable. Note that reading is
  // always successfull, and if not, its because of mismatching type and/or
  // because lhs does not support the requested operator. That includes trying
  // to read or do an operation from / with void or noreturn. Both cases are
  // handled elsewhere.
  if ( class_ == AstOperator::eAssignment ) {
    if ( ! (argschilds.front()->objType().qualifiers() & ObjType::eMutable) ) {
      Error::throwError(m_errorHandler, Error::eWriteToImmutable);
    }
  }

  // Verify that the first argument has the operator as member function.
  if ( opop!=AstOperator::eSeq // eSeq is a global operator, not a member function
    && !argschilds.front()->objType().hasMember(opop)) {
    Error::throwError(m_errorHandler, Error::eNoSuchMember);
  }

  // Report eUnreachableCode if the lhs of an sequence operator is of obj type
  // noreturn.  That is actually also true for other operators, but there the
  // error eNoSuchMember has higher priority.
  if ( opop==AstOperator::eSeq ) {
    if ( argschilds.front()->objType().isNoreturn() ) {
      Error::throwError(m_errorHandler, Error::eUnreachableCode);
    }
  }

  // On eComputedValueNotUsed
  if ( opop!=AstOperator::eSeq // computes no value
    && opop!=AstOperator::eAnd // shirt circuit operator, is effectively flow control
    && opop!=AstOperator::eOr  // dito
    && class_!=AstOperator::eAssignment // have side effects
    && op.access()==eIgnore ) {
    Error::throwError(m_errorHandler, Error::eComputedValueNotUsed);
  }

  // Set the obj type and optionally also already the stentry of this
  // AstOperator node.
  shared_ptr<ObjType> objType;
  {
    // comparision ops store the result of their computation in a new a
    // temporary object of type bool
    if ( class_ == AstOperator::eComparison ) {
      objType = std::make_unique<ObjTypeFunda>(ObjTypeFunda::eBool);
    }
    // The object denoted by the dot-assignment is exactly that of lhs
    else if ( opop == AstOperator::eDotAssign ) {
      op.setStentry(argschilds.front()->stentryAsSp());
    }
    // Assignment is always of object type void
    else if ( opop == AstOperator::eAssign ) {
      objType = make_shared<ObjTypeFunda>(ObjTypeFunda::eVoid);
    }
    // The object denoted by the seq operator is exactly that of rhs
    else if ( opop == AstOperator::eSeq ) {
      op.setStentry(argschilds.back()->stentryAsSp());
    }
    // Addrof 
    else if ( opop == AstOperator::eAddrOf) {
      objType =
        make_shared<ObjTypePtr>(
          shared_ptr<ObjType>{argschilds.back()->objType().clone()});
    }
    // 
    else if ( opop == AstOperator::eDeref) {
      // It is known that static cast is safe because it was checked before
      // that the operand of the deref operator has the deref operator as
      // member function.
      const ObjTypePtr& opObjType =
        static_cast<const ObjTypePtr&>(argschilds.back()->objType());
      objType.reset(opObjType.pointee().clone());

      // op.stentry is semantically a redundant copy of the stentry denoting
      // the derefee. 'Copy' as good as we can. Since the address of the
      // derefee object is obviously known (we're just dereferencing it), we
      // know that the address of the derefee object has been taken.
      op.setStentry(make_shared<SymbolTableEntry>(objType, StorageDuration::eUnknown));
      op.stentry()->addAccessToObject(eTakeAddress);    
    }
    // For ther operands, the operator expression's objtype is, now that we
    // know the two operands have the same obj type, either operand's obj
    // type.
    else {
      objType.reset(argschilds.back()->objType().clone());
      objType->removeQualifiers(ObjType::eMutable);
    }
  }

  // Some operators did already set op.stentry above, the others do it now
  // here
  if ( !op.stentry() ) {
    op.setStentry(make_shared<SymbolTableEntry>(objType, StorageDuration::eLocal));
  }
  op.stentry()->addAccessToObject(op.access());

  postConditionCheck(op);
}

void SemanticAnalizer::visit(AstNumber& number) {
  // should allready have been caught by scanner, thus assert.  Intended to
  // catch errors when AST is build by hand instead by parser, e.g. in tests.
  assert( number.objType().isValueInRange( number.value()));

  // no need to set stentry since that is done by AST node itself

  number.stentry()->addAccessToObject(number.access());

  postConditionCheck(number);
}

void SemanticAnalizer::visit(AstSymbol& symbol) {
  shared_ptr<SymbolTableEntry> stentry;
  m_env.find(symbol.name(), stentry);
  if (NULL==stentry) {
    Error::throwError(m_errorHandler, Error::eUnknownName);
  }
  stentry->addAccessToObject(symbol.access());
  symbol.setStentry(move(stentry));
  postConditionCheck(symbol);
}

void SemanticAnalizer::visit(AstFunCall& funCall) {
  funCall.address().accept(*this);
  funCall.args().accept(*this);

  const ObjTypeFun& objTypeFun = dynamic_cast<const ObjTypeFun&>(funCall.address().objType());
  const auto& argsCall = funCall.args().childs();
  const auto& argsCallee = objTypeFun.args();
  if ( argsCall.size() != argsCallee.size() ) {
    Error::throwError(m_errorHandler, Error::eInvalidArguments);
  }
  auto argCallIter = argsCall.begin();
  auto argCallEnd = argsCall.end();
  auto argCalleeIter = argsCallee.begin();
  for ( ; argCallIter!=argCallEnd; ++argCallIter, ++argCalleeIter) {
    if ( ! (*argCallIter)->objType().matchesSaufQualifiers(**argCalleeIter) ) {
      Error::throwError(m_errorHandler, Error::eInvalidArguments);
    }
  }

  funCall.createAndSetStEntryUsingRetObjType();

  funCall.stentry()->addAccessToObject(funCall.access());

  postConditionCheck(funCall);
}

void SemanticAnalizer::visit(AstFunDef& funDef) {
  funDef.decl().accept(*this);

  {
    m_funRetObjTypes.push(&funDef.decl().retObjType());
    Env::AutoScope scope(m_env);

    list<AstArgDecl*>::const_iterator argDeclIter = funDef.decl().args().begin();
    list<AstArgDecl*>::const_iterator end = funDef.decl().args().end();
    for ( ; argDeclIter!=end; ++argDeclIter) {
      (*argDeclIter)->accept(*this);
      (*argDeclIter)->stentry()->markAsDefined(m_errorHandler);
    }

    funDef.body().accept(*this);

    m_funRetObjTypes.pop();
  }

  const auto& bodyObjType = funDef.body().objType();
  if ( ! bodyObjType.matchesSaufQualifiers( funDef.decl().retObjType())
    && ! bodyObjType.isNoreturn()) {
    Error::throwError(m_errorHandler, Error::eNoImplicitConversion);
  }


  funDef.setStentry( funDef.decl().stentryAsSp() );
  funDef.stentry()->markAsDefined(m_errorHandler);

  funDef.stentry()->addAccessToObject(funDef.access());

  postConditionCheck(funDef);
}

void SemanticAnalizer::visit(AstFunDecl& funDecl) {
  // Note that visit(AstFundDef) does all of the work for the case AstFunDecl
  // is a child of AstFundDef.
  if ( funDecl.retObjType().qualifiers() & ObjType::eMutable ) {
    Error::throwError(m_errorHandler, Error::eRetTypeCantHaveMutQualifier);
  }
  funDecl.stentry()->addAccessToObject(funDecl.access());
  postConditionCheck(funDecl);
}

void SemanticAnalizer::visit(AstDataDecl& dataDecl) {

  // let dataDecl.stentry() point either to a newly created symbol table entry
  // or to an allready existing one declaring the same data object
  {
    Env::InsertRet insertRet = m_env.insert( dataDecl.name(), nullptr );
    shared_ptr<SymbolTableEntry>& envs_stentry_ptr = insertRet.first->second;

    // name is not yet in env, thus insert new symbol table entry
    if (insertRet.second) {
      envs_stentry_ptr = dataDecl.createAndSetStEntryUsingDeclaredObjType();
    }

    // name is already in env: unless the type matches that is an error
    else {
      assert(envs_stentry_ptr.get());
      const auto objTypeMatchesFully =
        envs_stentry_ptr->objType().matchesFully(dataDecl.declaredObjType());
      const auto storageDurationMatches =
        envs_stentry_ptr->storageDuration() ==
        dataDecl.declaredStorageDuration();
      if ( !objTypeMatchesFully || !storageDurationMatches ) {
        Error::throwError(m_errorHandler, Error::eIncompatibleRedeclaration);
      }
      dataDecl.setStentry(envs_stentry_ptr);
    }
  }

  dataDecl.stentry()->addAccessToObject(dataDecl.access());
  postConditionCheck(dataDecl);
}

void SemanticAnalizer::visit(AstArgDecl& argDecl) {
  visit( dynamic_cast<AstDataDecl&>(argDecl));
}

void SemanticAnalizer::visit(AstDataDef& dataDef) {
  dataDef.decl().accept(*this);
  dataDef.ctorArgs().accept(*this);

  if ( dataDef.decl().objType().match(dataDef.initValue().objType()) == ObjType::eNoMatch ) {
    Error::throwError(m_errorHandler, Error::eNoImplicitConversion);
  }
  if ( dataDef.decl().stentry()->storageDuration() == StorageDuration::eStatic
    && !dataDef.initValue().isCTConst() ) {
    Error::throwError(m_errorHandler, Error::eCTConstRequired);
  }

  dataDef.setStentry( dataDef.decl().stentryAsSp() );
  dataDef.stentry()->markAsDefined(m_errorHandler);

  dataDef.stentry()->addAccessToObject(dataDef.access());

  postConditionCheck(dataDef);
}

void SemanticAnalizer::visit(AstIf& if_) {
  if_.condition().accept(*this);

  if_.action().setAccess(if_.access());
  if_.action().accept(*this);

  if (if_.elseAction()) {
    if_.elseAction()->setAccess(if_.access());
    if_.elseAction()->accept(*this);
  }

  if ( !if_.condition().objType().matchesSaufQualifiers(
      ObjTypeFunda(ObjTypeFunda::eBool)) ) {
    Error::throwError(m_errorHandler, Error::eNoImplicitConversion);
  }

  // In case of two clauses, check that both clauses are of the same obj type,
  // sauf qualifiers. Currently there are no implicit conversions.  As an
  // exception, if one of the two clauses is of type noreturn, then the if
  // expression's type is that of the other clause.
  const bool actionIsOfTypeNoreturn = if_.action().objType().isNoreturn();
  if ( if_.elseAction() ) {
    auto& lhs = if_.action().objType();
    auto& rhs = if_.elseAction()->objType();
    if ( !lhs.matchesSaufQualifiers(rhs) ) {
      if ( !actionIsOfTypeNoreturn && !rhs.isNoreturn()) {
        Error::throwError(m_errorHandler, Error::eNoImplicitConversion);
      }
    }
  }

  // Set the obj type of this AstIf node. It's exactly the type of the
  // evaluated clause, i.e. it's not a new (immutable) temporary.  Now that we
  // know the two clauses have the same obj type, either clause's obj type can
  // be taken.
  shared_ptr<ObjType> objType;
  if ( if_.elseAction() ) {
    if ( !actionIsOfTypeNoreturn ) {
      objType.reset(if_.action().objType().clone());
    } else {
      objType.reset(if_.elseAction()->objType().clone());
    }
    // don't remove mutable qualifier
  } else {
    objType = make_shared<ObjTypeFunda>(ObjTypeFunda::eVoid);
  }
  if_.setStentry(make_shared<SymbolTableEntry>(objType, StorageDuration::eLocal));

  if_.stentry()->addAccessToObject(if_.access());

  postConditionCheck(if_);
}

void SemanticAnalizer::visit(AstLoop& loop) {
  // access to condition is eRead, which we  need not explicitely set
  loop.condition().accept(*this);

  loop.body().setAccess(eIgnore);
  loop.body().accept(*this);

  if ( !loop.condition().objType().matchesSaufQualifiers(
      ObjTypeFunda(ObjTypeFunda::eBool)) ) {
    Error::throwError(m_errorHandler, Error::eNoImplicitConversion);
  }

  // no need to set stentry since that is done by AST node itself
  // loop's ObjType is always void

  loop.stentry()->addAccessToObject(loop.access());

  postConditionCheck(loop);
}

void SemanticAnalizer::visit(AstReturn& return_) {
  if ( m_funRetObjTypes.empty() ) {
    Error::throwError(m_errorHandler, Error::eNotInFunBodyContext);
  }
  const auto& currentFunReturnType = *m_funRetObjTypes.top();
  if ( ! return_.retVal().objType().matchesSaufQualifiers( currentFunReturnType ) ) {
    Error::throwError(m_errorHandler, Error::eNoImplicitConversion);
  }

  // no need to set stentry since that is done by AST node itself
  // return_'s ObjType is always eNoreturn

  return_.stentry()->addAccessToObject(return_.access());

  postConditionCheck(return_);
}

void SemanticAnalizer::callAcceptWithinNewScope(AstValue& node) {
  Env::AutoScope scope(m_env);
  node.accept(*this);
}

void SemanticAnalizer::postConditionCheck(const AstValue& node) {
  assert(node.stentry());
}
