#include "semanticanalizer.h"
#include "ast.h"
#include "astdefaultiterator.h"
#include "env.h"
#include "object.h"
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

  // no need to set object since that is done by AST node itself

  cast.object()->addAccess(cast.access());

  postConditionCheck(cast);
}

void SemanticAnalizer::visit(AstNop& nop) {
  // no need to set object since that is done by AST node itself

  nop.object()->addAccess(nop.access());

  postConditionCheck(nop);
}

void SemanticAnalizer::visit(AstBlock& block) {
  { Env::AutoScope scope(m_env);
    block.body().accept(*this);
  }

  shared_ptr<ObjType> objType{block.body().objType().clone()};
  objType->removeQualifiers(ObjType::eMutable);
  block.setObject(make_shared<Object>(objType, StorageDuration::eLocal));

  block.object()->addAccess(block.access());

  postConditionCheck(block);
}

void SemanticAnalizer::visit(AstCtList& ctList) {
  list<AstObject*>& childs = ctList.childs();
  for (list<AstObject*>::const_iterator i=childs.begin();
       i!=childs.end(); ++i) {
    (*i)->accept(*this);
  }
}

void SemanticAnalizer::visit(AstOperator& op) {
  const list<AstObject*>& argschilds = op.args().childs();

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

  // Set the obj type and optionally also already the object of this
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
      op.setObject(argschilds.front()->objectAsSp());
    }
    // Assignment is always of object type void
    else if ( opop == AstOperator::eAssign ) {
      objType = make_shared<ObjTypeFunda>(ObjTypeFunda::eVoid);
    }
    // The object denoted by the seq operator is exactly that of rhs
    else if ( opop == AstOperator::eSeq ) {
      op.setObject(argschilds.back()->objectAsSp());
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

      // op.object is semantically a redundant copy of the object denoting
      // the derefee. 'Copy' as good as we can. Since the address of the
      // derefee object is obviously known (we're just dereferencing it), we
      // know that the address of the derefee object has been taken.
      op.setObject(make_shared<Object>(objType, StorageDuration::eUnknown));
      op.object()->addAccess(eTakeAddress);    
    }
    // For ther operands, the operator expression's objtype is, now that we
    // know the two operands have the same obj type, either operand's obj
    // type.
    else {
      objType.reset(argschilds.back()->objType().clone());
      objType->removeQualifiers(ObjType::eMutable);
    }
  }

  // Some operators did already set op.object above, the others do it now
  // here
  if ( !op.object() ) {
    op.setObject(make_shared<Object>(objType, StorageDuration::eLocal));
  }
  op.object()->addAccess(op.access());

  postConditionCheck(op);
}

void SemanticAnalizer::visit(AstNumber& number) {
  // should allready have been caught by scanner, thus assert.  Intended to
  // catch errors when AST is build by hand instead by parser, e.g. in tests.
  assert( number.objType().isValueInRange( number.value()));

  // no need to set object since that is done by AST node itself

  number.object()->addAccess(number.access());

  postConditionCheck(number);
}

void SemanticAnalizer::visit(AstSymbol& symbol) {
  shared_ptr<Entity> entity;
  m_env.find(symbol.name(), entity);
  if (NULL==entity) {
    Error::throwError(m_errorHandler, Error::eUnknownName);
  }
  auto object = std::dynamic_pointer_cast<Object>(entity);
  object->addAccess(symbol.access());
  symbol.setObject(move(object));
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

  funCall.createAndSetObjectUsingRetObjType();

  funCall.object()->addAccess(funCall.access());

  postConditionCheck(funCall);
}

void SemanticAnalizer::visit(AstFunDef& funDef) {
  if ( funDef.retObjType().qualifiers() & ObjType::eMutable ) {
    Error::throwError(m_errorHandler, Error::eRetTypeCantHaveMutQualifier);
  }

  {
    m_funRetObjTypes.push(&funDef.retObjType());
    Env::AutoScope scope(m_env);

    list<AstDataDef*>::const_iterator argDefIter = funDef.args().begin();
    list<AstDataDef*>::const_iterator end = funDef.args().end();
    for ( ; argDefIter!=end; ++argDefIter) {
      assert((*argDefIter)->declaredStorageDuration() == StorageDuration::eLocal);
      (*argDefIter)->accept(*this);
    }

    funDef.body().accept(*this);

    m_funRetObjTypes.pop();
  }

  const auto& bodyObjType = funDef.body().objType();
  if ( ! bodyObjType.matchesSaufQualifiers( funDef.retObjType())
    && ! bodyObjType.isNoreturn()) {
    Error::throwError(m_errorHandler, Error::eNoImplicitConversion);
  }

  funDef.object()->addAccess(funDef.access());

  postConditionCheck(funDef);
}

void SemanticAnalizer::visit(AstDataDef& dataDef) {
  // let dataDecl.object() point a newly created symbol table entry
  {
    const auto& insertRet = m_env.insert( dataDef.name(), nullptr );
    auto& envs_entity_ptr = insertRet.first->second;

    // name is not yet in env, thus insert new symbol table entry
    if (insertRet.second) {
      envs_entity_ptr = dataDef.createAndSetObjectUsingDeclaredObjType();
    }

    // name is already in env: unless the type matches that is an error
    else {
      Error::throwError(m_errorHandler, Error::eRedefinition);
    }
  }

  dataDef.ctorArgs().accept(*this);

  if ( dataDef.objType().match(dataDef.initObj().objType()) == ObjType::eNoMatch ) {
    Error::throwError(m_errorHandler, Error::eNoImplicitConversion);
  }
  if ( dataDef.object()->storageDuration() == StorageDuration::eStatic
    && !dataDef.initObj().isCTConst() ) {
    Error::throwError(m_errorHandler, Error::eCTConstRequired);
  }

  dataDef.object()->addAccess(dataDef.access());

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
  if_.setObject(make_shared<Object>(objType, StorageDuration::eLocal));

  if_.object()->addAccess(if_.access());

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

  // no need to set object since that is done by AST node itself
  // loop's ObjType is always void

  loop.object()->addAccess(loop.access());

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

  // no need to set object since that is done by AST node itself
  // return_'s ObjType is always eNoreturn

  return_.object()->addAccess(return_.access());

  postConditionCheck(return_);
}

void SemanticAnalizer::callAcceptWithinNewScope(AstObject& node) {
  Env::AutoScope scope(m_env);
  node.accept(*this);
}

void SemanticAnalizer::postConditionCheck(const AstObject& node) {
  assert(node.object());
}
