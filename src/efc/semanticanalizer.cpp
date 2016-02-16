#include "semanticanalizer.h"
#include "ast.h"
#include "env.h"
#include "object.h"
#include "errorhandler.h"
#include "objtype.h"
#include "envinserter.h"
#include "signatureaugmentor.h"
using namespace std;

SemanticAnalizer::SemanticAnalizer(Env& env, ErrorHandler& errorHandler) :
  m_env(env),
  m_errorHandler(errorHandler) {
}

void SemanticAnalizer::analyze(AstNode& root) {
  EnvInserter envinserter(m_env, m_errorHandler);
  envinserter.insertIntoEnv(root);

  SignatureAugmentor signatureaugmentor(m_env, m_errorHandler);
  signatureaugmentor.augmentEntities(root);

  root.setAccess(Access::eIgnoreValueAndAddr);
  root.accept(*this);
}

SemanticAnalizer::FunBodyHelper::FunBodyHelper(
  stack<const AstObjType*>& funRetAstObjTypes, const AstObjType* objType) :
  m_funRetAstObjTypes(funRetAstObjTypes) {
  m_funRetAstObjTypes.push(objType);
}

SemanticAnalizer::FunBodyHelper::~FunBodyHelper() {
  m_funRetAstObjTypes.pop();
}

void SemanticAnalizer::visit(AstCast& cast) {
  preConditionCheck(cast);

  // -- responsibility 1: set access to direct childs and descent AST subtree
  setAccessAndCallAcceptOn(cast.child(), Access::eRead);
  cast.specifiedNewAstObjType().accept(*this);

  // -- responsibility 2: semantic analysis
  // test if conversion is eligible
  const auto& specifiedNewObjType = cast.specifiedNewAstObjType().objType();
  if ( !specifiedNewObjType.matchesSaufQualifiers(cast.child().objType())
    && !specifiedNewObjType.hasConstructor(cast.child().objType()) ) {
    Error::throwError(m_errorHandler, Error::eNoSuchMember);
  }

  // -- responsibility 3: set properties of associated object: type, sd, access
  cast.setObject(
    make_shared<Object>(
      cast.specifiedNewAstObjType().objTypeAsSp(),
      StorageDuration::eLocal));
  cast.object()->addAccess(cast.access());

  postConditionCheck(cast);
}

void SemanticAnalizer::visit(AstNop& nop) {
  preConditionCheck(nop);

  // -- responsibility 1: set access to direct childs and descent AST subtree
  nop.object()->addAccess(nop.access());

  // -- responsibility 2: semantic analysis
  // nop

  // -- responsibility 3: set properties of associated object: type, sd, access
  // done by AST node itself

  postConditionCheck(nop);
}

void SemanticAnalizer::visit(AstBlock& block) {
  preConditionCheck(block);

  // -- responsibility 1: set access to direct childs and descent AST subtree
  {
    Env::AutoScope scope(m_env, block.name(), Env::AutoScope::descentScope);
    setAccessAndCallAcceptOn(block.body(), Access::eRead);
  }

  // -- responsibility 2: semantic analysis
  // nop

  // -- responsibility 3: set properties of associated object: type, sd, access
  const auto& blockObjType = block.body().objType().unqualifiedObjType();
  block.setObject(make_shared<Object>(blockObjType, StorageDuration::eLocal));
  block.object()->addAccess(block.access());

  postConditionCheck(block);
}

void SemanticAnalizer::visit(AstCtList& ctList) {
  // parent shall iterate itself so it can set access to each element
  // independently
  assert(false);
}

void SemanticAnalizer::visit(AstOperator& op) {
  preConditionCheck(op);

  // Note that orrect number of arguments was already handled in AstOperator's
  // ctor; we're allowed to count on that.
  auto& argschilds = op.args().childs();
  const auto class_ = op.class_();
  const auto opop = op.op();

  // -- responsibility 1: set access to direct childs and descent AST subtree
  {
    Access access = Access::eRead;
    if ( AstOperator::eAssignment == class_) {
      access = Access::eWrite;
    } else if ( opop==AstOperator::eAddrOf ) {
      access = Access::eTakeAddress;
    }
    for (const auto arg: op.args().childs()) {
      setAccessAndCallAcceptOn(*arg, access);
    }
  }

  // -- responsibility 2: semantic analysis

  // Check that all operands are of the same obj type, sauf
  // qualifiers. Currently there are no implicit conversions. However the rhs
  // of logical and/or is allowed to be eNoreturn.
  if ( argschilds.size()==2 ) {
    auto& lhs = argschilds.front()->objType();
    auto& rhs = argschilds.back()->objType();
    if ( op.class_() == AstOperator::eLogical ) {
      if ( !lhs.matchesSaufQualifiers(rhs)
        && (!op.isBinaryLogicalShortCircuit() ||
            !rhs.matchesSaufQualifiers(ObjTypeFunda(ObjTypeFunda::eNoreturn)))) {
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
  if ( !argschilds.front()->objType().hasMember(opop)) {
    Error::throwError(m_errorHandler, Error::eNoSuchMember);
  }

  // -- responsibility 3: set properties of associated object: type, sd, access

  // Set the obj type and optionally also already the object of this
  // AstOperator node.
  shared_ptr<const ObjType> objType;
  {
    // comparision ops store the result of their computation in a new a
    // temporary object of type bool
    if ( class_ == AstOperator::eComparison ) {
      objType = std::make_unique<ObjTypeFunda>(ObjTypeFunda::eBool);
    }
    // The object denoted by the dot-assignment is exactly that of lhs
    else if ( opop == AstOperator::eAssign ) {
      op.setObject(argschilds.front()->objectAsSp());
    }
    // Assignment is always of object type void
    else if ( opop == AstOperator::eVoidAssign ) {
      objType = make_shared<ObjTypeFunda>(ObjTypeFunda::eVoid);
    }
    // Addrof 
    else if ( opop == AstOperator::eAddrOf) {
      objType = make_shared<ObjTypePtr>(argschilds.back()->objTypeAsSp());
    }
    // 
    else if ( opop == AstOperator::eDeref) {
      // It is known that static cast is safe because it was checked before
      // that the operand of the deref operator has the deref operator as
      // member function.
      const ObjTypePtr& opObjType =
        static_cast<const ObjTypePtr&>(argschilds.back()->objType());
      objType = opObjType.pointee();

      // op.object is semantically a redundant copy of the object denoting
      // the derefee. 'Copy' as good as we can. Since the address of the
      // derefee object is obviously known (we're just dereferencing it), we
      // know that the address of the derefee object has been taken.
      op.setObject(make_shared<Object>(objType, StorageDuration::eUnknown));
      op.object()->addAccess(Access::eTakeAddress);    
    }
    // For ther operands, the operator expression's objtype is, now that we
    // know the two operands have the same obj type, either operand's obj
    // type.
    else {
      objType = argschilds.back()->objType().unqualifiedObjType();
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

void SemanticAnalizer::visit(AstSeq& seq) {
  preConditionCheck(seq);

  assert(!seq.operands().empty());

  AstObject* lastOperandAsAstObject{};
  const auto& lastOp = seq.operands().back();
  for (const auto& op: seq.operands()) {
    // -- responsibility 1: set access to direct childs and descent AST subtree
    auto access = (op==lastOp) ? seq.access() : Access::eIgnoreValueAndAddr;
    setAccessAndCallAcceptOn(*op, access);

    // -- responsibility 2: semantic analysis
    // see also beginning of method
    if ( op==lastOp ) {
      lastOperandAsAstObject = &seq.lastOperandAsAstObject(m_errorHandler);
    }
    else if ( op->isObjTypeNoReturn() ) {
      Error::throwError(m_errorHandler, Error::eUnreachableCode);
    }

    // -- responsibility 3: set properties of associated object: type, sd, access
    if ( op==lastOp ) {
      // The Object denoted by the AstSeq node is exactly that of the
      // lastOperandAsAstObject
      assert(lastOperandAsAstObject);
      seq.setObject(lastOperandAsAstObject->objectAsSp());
      seq.object()->addAccess(seq.access());
    }
  }

  postConditionCheck(seq);
}

void SemanticAnalizer::visit(AstNumber& number) {
  preConditionCheck(number);

  // -- responsibility 1: set access to direct childs and descent AST subtree
  // nop

  // -- responsibility 2: semantic analysis
  // should allready have been caught by scanner, thus assert.  Intended to
  // catch errors when AST is build by hand instead by parser, e.g. in tests.
  assert(number.declaredAstObjType().isValueInRange(number.value()));

  // -- responsibility 3: set properties of associated object: type, sd, access
  number.setObject(
    make_shared<Object>(
      number.declaredAstObjType().objTypeAsSp(),
      StorageDuration::eLocal));
  number.object()->addAccess(number.access());

  postConditionCheck(number);
}

void SemanticAnalizer::visit(AstSymbol& symbol) {
  preConditionCheck(symbol);

  // -- responsibility 1: set access to direct childs and descent AST subtree
  // nop

  // -- responsibility 2: semantic analysis
  shared_ptr<EnvNode>* node = m_env.find(symbol.name());
  if (nullptr==node) {
    Error::throwError(m_errorHandler, Error::eUnknownName);
  }
  assert(*node);
  const auto object = std::dynamic_pointer_cast<Object>(*node);
  assert(object);

  // 1) note that the access of this node is querried, as opposed to the
  //    access of the associated object (which is cummulative)
  if (object->storageDuration()==StorageDuration::eLocal &&
    !object->isInitialized() &&
    symbol.access() != Access::eIgnoreValueAndAddr /*1)*/) {
    Error::throwError(m_errorHandler,
      Error::eNonIgnoreAccessToLocalDataObjectBeforeItsInitialization);
  }

  // -- responsibility 3: set properties of associated object: type, sd, access
  object->addAccess(symbol.access());
  symbol.setObject(move(object));

  postConditionCheck(symbol);
}

void SemanticAnalizer::visit(AstFunCall& funCall) {
  preConditionCheck(funCall);

  // -- responsibility 1: set access to direct childs and descent AST subtree
  setAccessAndCallAcceptOn(funCall.address(), Access::eRead);
  for (const auto arg: funCall.args().childs()) {
    setAccessAndCallAcceptOn(*arg, Access::eRead);
  }

  // -- responsibility 2: semantic analysis
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

  // -- responsibility 3: set properties of associated object: type, sd, access
  funCall.createAndSetObjectUsingRetObjType();
  funCall.object()->addAccess(funCall.access());

  postConditionCheck(funCall);
}

void SemanticAnalizer::visit(AstFunDef& funDef) {
  preConditionCheck(funDef);

  // -- responsibility 2 / part 1 of 2: semantic analysis
  // verify signature before descending into AST subtree, so the subtree can
  // see a valid signature
  if ( funDef.declaredRetAstObjType().objType().qualifiers() & ObjType::eMutable ) {
    Error::throwError(m_errorHandler, Error::eRetTypeCantHaveMutQualifier);
  }
  for (const auto& arg : funDef.declaredArgs()) {
    if (arg->declaredStorageDuration() != StorageDuration::eLocal) {
      Error::throwError(m_errorHandler, Error::eOnlyLocalStorageDurationApplicable);
    }
  }

  // -- responsibility 1: set access to direct childs and descent AST subtree
  {
    FunBodyHelper dummy(m_funRetAstObjTypes, &funDef.declaredRetAstObjType());
    Env::AutoScope scope(m_env, funDef.name(), Env::AutoScope::descentScope);
    for (const auto& arg : funDef.declaredArgs()) {
      setAccessAndCallAcceptOn(*arg, Access::eIgnoreValueAndAddr);
    }
    setAccessAndCallAcceptOn(funDef.body(), Access::eRead);
  }

  // -- responsibility 2 / part 2 of 2: semantic analysis
  const auto& bodyObjType = funDef.body().objType();
  if ( ! bodyObjType.matchesSaufQualifiers( funDef.declaredRetAstObjType().objType())
    && ! bodyObjType.isNoreturn()) {
    Error::throwError(m_errorHandler, Error::eNoImplicitConversion);
  }

  // -- responsibility 3: set properties of associated object: type, sd, access
  // As with all definitions, SignatureAugmentor, did already define type and
  // sd
  funDef.object()->addAccess(funDef.access());

  postConditionCheck(funDef);
}

void SemanticAnalizer::visit(AstDataDef& dataDef) {
  preConditionCheck(dataDef);

  if ( !dataDef.doNotInit() ) {
    // default member initializer not yet supported
    assert(dataDef.declaredStorageDuration() != StorageDuration::eMember);

    auto&& ctorArgs = dataDef.ctorArgs().childs();

    // Currrently zero arguments are not supported. When none args are given,
    // a default arg is used.
    if (ctorArgs.empty() && !dataDef.doNotInit()) {
      // Note that the AST node created here obviously is not visited by the
      // passes prior to SemanticAnalizer
      ctorArgs.push_back(dataDef.declaredAstObjType().
        createDefaultAstObjectForSemanticAnalizer());
    }

    for (const auto arg: ctorArgs) {
      setAccessAndCallAcceptOn(*arg, Access::eRead);
    }

    // currently a data object must be initialized with exactly one initializer
    assert(ctorArgs.size()==1);
    const auto initializer = ctorArgs.front();
    if ( dataDef.objType().match(initializer->objType()) == ObjType::eNoMatch ) {
      Error::throwError(m_errorHandler, Error::eNoImplicitConversion);
    }
    if ( dataDef.object()->storageDuration() == StorageDuration::eStatic
      && !initializer->isCTConst() ) {
      Error::throwError(m_errorHandler, Error::eCTConstRequired);
    }
  }

  // -- responsibility 3: set properties of associated object: type, sd, access
  // As with all definitions, SignatureAugmentor, did already define type and
  // sd
  if (dataDef.declaredStorageDuration() != StorageDuration::eMember) {
    // AstDataDef with storage duration eMember have no associated Object, thus
    // it's meaningless to assign 'that object' an access value.
    dataDef.object()->addAccess(dataDef.access());

    dataDef.object()->setIsInitialized(true);

    postConditionCheck(dataDef);
  }
}

void SemanticAnalizer::visit(AstIf& if_) {
  preConditionCheck(if_);

  // -- responsibility 1: set access to direct childs and descent AST subtree
  setAccessAndCallAcceptOn(if_.condition(), Access::eRead);
  setAccessAndCallAcceptOn(if_.action(), Access::eRead);
  if (if_.elseAction()) {
    setAccessAndCallAcceptOn(*if_.elseAction(), Access::eRead);
  }

  // -- responsibility 2: semantic analysis
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

  // -- responsibility 3: set properties of associated object: type, sd, access
  // The AstIf's obj type is a temporary with the underlying type of either
  // clause; now that we now both clauses have the same type.
  shared_ptr<const ObjType> objType;
  if ( if_.elseAction() ) {
    if ( !actionIsOfTypeNoreturn ) {
      objType = if_.action().objType().unqualifiedObjType();
    } else {
      objType = if_.elseAction()->objType().unqualifiedObjType();
    }
  } else {
    objType = make_shared<ObjTypeFunda>(ObjTypeFunda::eVoid);
  }
  if_.setObject(make_shared<Object>(objType, StorageDuration::eLocal));
  if_.object()->addAccess(if_.access());

  postConditionCheck(if_);
}

void SemanticAnalizer::visit(AstLoop& loop) {
  preConditionCheck(loop);

  // -- responsibility 1: set access to direct childs and descent AST subtree
  setAccessAndCallAcceptOn(loop.condition(), Access::eRead);
  setAccessAndCallAcceptOn(loop.body(), Access::eIgnoreValueAndAddr);

  // -- responsibility 2: semantic analysis
  if ( !loop.condition().objType().matchesSaufQualifiers(
      ObjTypeFunda(ObjTypeFunda::eBool)) ) {
    Error::throwError(m_errorHandler, Error::eNoImplicitConversion);
  }

  // -- responsibility 3: set properties of associated object: type, sd, access
  // no need to set object since that is done by AST node itself
  // loop's ObjType is always void
  loop.object()->addAccess(loop.access());

  postConditionCheck(loop);
}

void SemanticAnalizer::visit(AstReturn& return_) {
  preConditionCheck(return_);

  // -- responsibility 1: set access to direct childs and descent AST subtree
  setAccessAndCallAcceptOn(return_.retVal(), Access::eRead);

  // -- responsibility 2: semantic analysis
  if ( m_funRetAstObjTypes.empty() ) {
    Error::throwError(m_errorHandler, Error::eNotInFunBodyContext);
  }
  const auto& currentFunReturnType = *m_funRetAstObjTypes.top();
  if ( ! return_.retVal().objType().matchesSaufQualifiers(
      currentFunReturnType.objType() ) ) {
    Error::throwError(m_errorHandler, Error::eNoImplicitConversion);
  }

  // -- responsibility 3: set properties of associated object: type, sd, access
  // Object and it's ObjType, being ObjTypeFunda::eNoreturn, was already set
  // in AstReturn's ctor.
  return_.object()->addAccess(return_.access());

  postConditionCheck(return_);
}

void SemanticAnalizer::visit(AstObjTypeSymbol& symbol) {
  // nop, everything was already done in previous passes
  preConditionCheck(symbol);
  postConditionCheck(symbol);
}

void SemanticAnalizer::visit(AstObjTypeQuali& quali) {
  // nop, everything was already done in previous passes
  preConditionCheck(quali);
  postConditionCheck(quali);
}

void SemanticAnalizer::visit(AstObjTypePtr& ptr) {
  // nop, everything was already done in previous passes
  preConditionCheck(ptr);
  postConditionCheck(ptr);
}

void SemanticAnalizer::visit(AstClassDef& class_) {
  preConditionCheck(class_);
  for (const auto& dataMember: class_.dataMembers()) {
    setAccessAndCallAcceptOn(*dataMember, Access::eIgnoreValueAndAddr);
  }
  postConditionCheck(class_);
}

void SemanticAnalizer::preConditionCheck(const AstObject& node) {
  assert(node.access()!=Access::eYetUndefined);
}

void SemanticAnalizer::preConditionCheck(const AstObjType& node) {
  // none yet
}

void SemanticAnalizer::postConditionCheck(const AstObject& node) {
  assert(node.object()); //todo:move to signature augmentor
  assert(node.object()->objTypeAsSp());
}

void SemanticAnalizer::postConditionCheck(const AstObjType& node) {
  // If the reference returned by objType() was initialized with nullptr, that
  // is undefined behavior. In practice however taking the address of it will
  // deliver again a nullptr, and we assert for that.
  assert(&node.objType());
}

// The purpose of this method is to help to remember that these two (almost)
// always go together
void SemanticAnalizer::setAccessAndCallAcceptOn(AstNode& node,
  Access access) {
  node.setAccess(access);
  node.accept(*this);
}
