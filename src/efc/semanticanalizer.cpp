#include "semanticanalizer.h"

#include "ast.h"
#include "env.h"
#include "envinserter.h"
#include "errorhandler.h"
#include "freefromastobject.h"
#include "objtype.h"
#include "templateinstanciator.h"

using namespace std;

SemanticAnalizer::SemanticAnalizer(Env& env, ErrorHandler& errorHandler)
  : m_env(env), m_errorHandler(errorHandler) {
}

void SemanticAnalizer::analyze(AstNode& root) {
  // 'pass 0': Parser created AST and did some responsibilities on the fly, see
  // also GenParserExt.

  // pass 1 over AST: EnvInserter
  EnvInserter envinserter(m_env, m_errorHandler);
  envinserter.insertIntoEnv(root);

  // pass 2 over AST: TemplateInstanciator
  TemplateInstanciator templateInstanciator(m_env, m_errorHandler);
  templateInstanciator.instanciateTemplates(root);

  // pass 3 over AST: SemanticAnalizer itself
  root.setAccessFromAstParent(Access::eIgnoreValueAndAddr);
  root.accept(*this);
}

SemanticAnalizer::FunBodyHelper::FunBodyHelper(
  stack<const AstObjType*>& funRetAstObjTypes, const AstObjType* objType)
  : m_funRetAstObjTypes(funRetAstObjTypes) {
  m_funRetAstObjTypes.push(objType);
}

SemanticAnalizer::FunBodyHelper::~FunBodyHelper() {
  m_funRetAstObjTypes.pop();
}

void SemanticAnalizer::visit(AstCast& cast) {
  preConditionCheck(cast);

  // -- responsibility 1: set access to direct childs and descent AST subtree
  for (const auto arg : cast.args().childs()) {
    setAccessAndCallAcceptOn(*arg, Access::eRead);
  }
  cast.specifiedNewAstObjType().accept(*this);

  // -- responsibility 2: semantic analysis

  // TODO: write test
  // test that only one arg is provided
  const auto argCnt = cast.args().childs().size();
  if (argCnt != 1U) {
    Error::throwError(m_errorHandler, Error::eInvalidArguments, cast.loc(),
      to_string(argCnt), "1");
  }

  // test if conversion is eligible
  const auto& specifiedNewObjType = cast.specifiedNewAstObjType().objType();
  const auto& operand = cast.args().childs().front();
  const auto& operandObjType = operand->object().objType();
  if (!specifiedNewObjType.matchesSaufQualifiers(operandObjType) &&
    !specifiedNewObjType.hasConstructor(operandObjType)) {
    Error::throwError(m_errorHandler, Error::eNoSuchCtor, cast.loc(),
      operandObjType.completeName(), specifiedNewObjType.completeName());
  }

  // -- responsibility 3: set properties of associated object: type, sd, access
  // nop - done by AST node itself

  postConditionCheck(cast);
}

void SemanticAnalizer::visit(AstNop& nop) {
  preConditionCheck(nop);

  // -- responsibility 1: set access to direct childs and descent AST subtree
  // nop

  // -- responsibility 2: semantic analysis
  // nop

  // -- responsibility 3: set properties of associated object: type, sd, access
  // nop - done by AST node itself

  postConditionCheck(nop);
}

void SemanticAnalizer::visit(AstBlock& block) {
  preConditionCheck(block);

  // -- responsibility 1: set access to direct childs and descent AST subtree
  {
    Env::AutoScope scope(m_env, block);
    setAccessAndCallAcceptOn(block.body(), Access::eRead);
  }

  // -- responsibility 2: semantic analysis
  // nop

  // -- responsibility 3: set properties of associated object: type, sd, access
  // nop - done by AST node itself

  postConditionCheck(block);
}

void SemanticAnalizer::visit(AstCtList& /*ctList*/) {
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
    if (AstOperator::eAssignment == class_) { access = Access::eWrite; }
    else if (opop == AstOperator::eAddrOf) {
      access = Access::eTakeAddress;
    }
    for (const auto arg : op.args().childs()) {
      setAccessAndCallAcceptOn(*arg, access);
    }
  }

  // -- responsibility 2: semantic analysis

  // Check that all operands are of the same obj type, sauf
  // qualifiers. Currently there are no implicit conversions. However the rhs
  // of logical and/or is allowed to be eNoreturn.
  if (argschilds.size() == 2) {
    auto& lhs = argschilds.front()->object().objType();
    auto& rhs = argschilds.back()->object().objType();
    if (op.class_() == AstOperator::eLogical) {
      if (!lhs.matchesSaufQualifiers(rhs) &&
        (!op.isBinaryLogicalShortCircuit() ||
          !rhs.matchesSaufQualifiers(ObjTypeFunda(ObjTypeFunda::eNoreturn)))) {
        Error::throwError(m_errorHandler, Error::eNoImplicitConversion,
          op.loc(), rhs.completeName(), lhs.completeName());
      }
    }
    else if (class_ != AstOperator::eOther) {
      if (!lhs.matchesSaufQualifiers(rhs)) {
        Error::throwError(m_errorHandler, Error::eNoImplicitConversion,
          op.loc(), rhs.completeName(), lhs.completeName());
      }
    }
  }
  else {
    // In case there is only one arg, that arg's type can't missmatch
    // anything, so it's implicitely ok. If there are more than two args, the
    // implementation is missing.
    assert(argschilds.size() == 1);
  }

  // If an operand is modified ensure its writable. Note that reading is
  // always successfull, and if not, its because of mismatching type and/or
  // because lhs does not support the requested operator. That includes trying
  // to read or do an operation from / with void or noreturn. Both cases are
  // handled elsewhere.
  if (class_ == AstOperator::eAssignment) {
    if (!(argschilds.front()->object().objType().qualifiers() &
          ObjType::eMutable)) {
      Error::throwError(m_errorHandler, Error::eWriteToImmutable, op.loc());
    }
  }

  // Verify that the first argument has the operator as member function.
  const auto& lhs = *argschilds.front();
  const auto& lhsObjType = lhs.object().objType();
  if (!lhsObjType.hasMemberFun(opop)) {
    Error::throwError(m_errorHandler, Error::eNoSuchMemberFun, op.loc(),
      lhsObjType.completeName(), op.funName());
  }

  // -- responsibility 3: set properties of associated object: type, sd, access

  if (class_ == AstOperator::eComparison) {
    op.setObjType(std::make_unique<ObjTypeFunda>(ObjTypeFunda::eBool));
    op.setStorageDuration(StorageDuration::eLocal);
  }
  // The object denoted by the dot-assignment is exactly that of lhs
  else if (opop == AstOperator::eAssign) {
    op.setReferencedObjAndPropagateAccess(argschilds.front()->object());
  }
  // Assignment is always of object type void
  else if (opop == AstOperator::eVoidAssign) {
    op.setObjType(make_shared<ObjTypeFunda>(ObjTypeFunda::eVoid));
    op.setStorageDuration(StorageDuration::eLocal);
  }
  // Addrof
  else if (opop == AstOperator::eAddrOf) {
    op.object().addAccess(Access::eTakeAddress);
    op.setObjType(
      make_shared<ObjTypePtr>(argschilds.back()->object().objTypeAsSp()));
    op.setStorageDuration(StorageDuration::eLocal);
  }
  // Deref: The memory region at the address given by the value of the
  // operand, is the Object denoted by this AstNode.
  else if (opop == AstOperator::eDeref) {
    // It is known that static cast is safe because it was checked before
    // that the operand of the deref operator has the deref operator as
    // member function.
    const auto& opObjType =
      static_cast<const ObjTypePtr&>(argschilds.back()->object().objType());
    op.setReferencedObjAndPropagateAccess(
      make_unique<FreeFromAstObject>(opObjType.pointee()));
  }
  // For ther operands, the operator expression's objtype is, now that we
  // know the two operands have the same obj type, either operand's obj
  // type.
  else {
    op.setObjType(argschilds.back()->object().objType().unqualifiedObjType());
    op.setStorageDuration(StorageDuration::eLocal);
  }

  postConditionCheck(op);
}

void SemanticAnalizer::visit(AstSeq& seq) {
  preConditionCheck(seq);

  assert(!seq.operands().empty());

  // -- responsibility 2, part 1 of 2: semantic analysis
  if (!seq.lastOperandIsAnObject()) {
    Error::throwError(
      m_errorHandler, Error::eObjectExpected, seq.operands().back()->loc());
  }

  const auto& lastOp = seq.operands().back();
  for (const auto& op : seq.operands()) {
    // -- responsibility 1: set access to direct childs and descent AST subtree
    const auto access =
      (op == lastOp) ? seq.accessFromAstParent() : Access::eIgnoreValueAndAddr;
    setAccessAndCallAcceptOn(*op, access);

    // -- responsibility 2, part 2 of 2: semantic analysis
    // see also beginning of method
    if (op != lastOp && op->isObjTypeNoReturn()) {
      Error::throwError(m_errorHandler, Error::eUnreachableCode, op->loc());
    }

    // -- responsibility 3: set properties of associated object: type, sd, access
    // type & sd done by AST node itself. access to seq.object(), which
    // equals lastoperand.object(), was set in responsibility 1 above.
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
  // nop - done by AST node itself

  postConditionCheck(number);
}

void SemanticAnalizer::visit(AstSymbol& symbol) {
  preConditionCheck(symbol);

  // -- responsibility 1: set access to direct childs and descent AST subtree
  // nop

  // -- responsibility 2, part 1 of 2: semantic analysis
  auto node = m_env.find(symbol.name());
  if (nullptr == node) {
    Error::throwError(
      m_errorHandler, Error::eUnknownName, symbol.loc(), symbol.name());
  }
  const auto object = dynamic_cast<AstObjDef*>(node);
  assert(object);

  // -- responsibility 3: set properties of associated object: type, sd, access
  symbol.setreferencedAstObjAndPropagateAccess(*object);

  // -- responsibility 2, part 2 of 2: semantic analysis
  // 1) note that the access to this AST node is querried, as opposed to the
  //    access to the object associated to the AST node. See also
  //    AstNode::setAccessFromAstParent and AstObject::addAccess.
  if (symbol.storageDuration() == StorageDuration::eLocal &&
    !symbol.isInitialized() &&
    symbol.accessFromAstParent() != Access::eIgnoreValueAndAddr /*1)*/) {
    Error::throwError(m_errorHandler,
      Error::eNonIgnoreAccessToLocalDataObjectBeforeItsInitialization,
      symbol.loc(), symbol.name());
  }

  postConditionCheck(symbol);
}

void SemanticAnalizer::visit(AstFunCall& funCall) {
  preConditionCheck(funCall);

  // -- responsibility 1: set access to direct childs and descent AST subtree
  setAccessAndCallAcceptOn(funCall.address(), Access::eRead);
  for (const auto arg : funCall.args().childs()) {
    setAccessAndCallAcceptOn(*arg, Access::eRead);
  }

  // -- responsibility 2: semantic analysis
  const auto& objTypeFun =
    dynamic_cast<const ObjTypeFun&>(funCall.address().object().objType());
  const auto& argsCall = funCall.args().childs();
  const auto& argsCallee = objTypeFun.args();
  if (argsCall.size() != argsCallee.size()) {
    Error::throwError(m_errorHandler, Error::eInvalidArguments, funCall.loc(),
      to_string(argsCall.size()), to_string(argsCallee.size()));
  }
  auto argCallIter = argsCall.begin();
  auto argCallEnd = argsCall.end();
  auto argCalleeIter = argsCallee.begin();
  for (; argCallIter != argCallEnd; ++argCallIter, ++argCalleeIter) {
    const auto& argCallObjType = (*argCallIter)->object().objType();
    const auto& argCalleeObjType = **argCalleeIter;
    if (!argCallObjType.matchesSaufQualifiers(argCalleeObjType)) {
      Error::throwError(m_errorHandler, Error::eNoImplicitConversion,
        (*argCallIter)->loc(), argCallObjType.completeName(),
        argCalleeObjType.completeName());
    }
  }

  // -- responsibility 3: set properties of associated object: type, sd, access
  // nop - done by AST node itself

  postConditionCheck(funCall);
}

void SemanticAnalizer::visit(AstFunDef& funDef) {
  preConditionCheck(funDef);
  const auto& retObjType = funDef.ret().objType();

  // -- responsibility 2 / part 1 of 2: semantic analysis
  // verify signature before descending into AST subtree, so the subtree can
  // see a valid signature
  if (retObjType.qualifiers() & ObjType::eMutable) {
    Error::throwError(
      m_errorHandler, Error::eRetTypeCantHaveMutQualifier, funDef.ret().loc());
  }
  for (const auto& arg : funDef.declaredArgs()) {
    if (arg->declaredStorageDuration() != StorageDuration::eLocal) {
      Error::throwError(m_errorHandler, Error::eInvalidStorageDurationInDef,
        arg->loc(), toString(arg->declaredStorageDuration()));
    }
  }

  // -- responsibility 1: set access to direct childs and descent AST subtree
  {
    FunBodyHelper dummy(m_funRetAstObjTypes, &funDef.ret());
    Env::AutoScope scope(m_env, funDef);
    for (const auto& arg : funDef.declaredArgs()) {
      setAccessAndCallAcceptOn(*arg, Access::eIgnoreValueAndAddr);
    }
    setAccessAndCallAcceptOn(funDef.body(), Access::eRead);
    funDef.ret().accept(*this);
  }

  // -- responsibility 2 / part 2 of 2: semantic analysis
  const auto& bodyObjType = funDef.body().object().objType();
  if (!bodyObjType.matchesSaufQualifiers(retObjType) &&
    !bodyObjType.isNoreturn()) {
    Error::throwError(m_errorHandler, Error::eNoImplicitConversion,
      funDef.body().loc(), bodyObjType.completeName(),
      retObjType.completeName());
  }

  // -- responsibility 3: set properties of associated object: type, sd, access
  // nop - done by AST node itself

  postConditionCheck(funDef);
}

void SemanticAnalizer::visit(AstDataDef& dataDef) {
  if (dataDef.declaredStorageDuration() == StorageDuration::eMember) { return; }

  preConditionCheck(dataDef);

  auto&& ctorArgs = dataDef.ctorArgs().childs();

  if (!dataDef.doNotInit()) {
    // special responsibility: insert new auto created nodes into AST.  Note
    // that the AST nodes created here obviously are not visited by the passes
    // prior to SemanticAnalizer.
    //
    // If there are zero ctor arguments, that means we should call the default
    // ctor. However default ctors are not yet implemented. Thus the workaround
    // is to insert a default arg into the AST.
    if (ctorArgs.empty()) {
      const auto& loc = dataDef.ctorArgs().loc();
      const auto& ot = dataDef.declaredAstObjType();
      ctorArgs.push_back(ot.createDefaultAstObjectForSemanticAnalizer(loc));
    }

    // -- responsibility 1: set access to direct childs and descent AST subtree
    for (const auto arg : ctorArgs) {
      setAccessAndCallAcceptOn(*arg, Access::eRead);
    }
    dataDef.declaredAstObjType().accept(*this);

    // -- responsibility 2: semantic analysis
    // currently a data object must be initialized with exactly one
    // initializer. Note that the case of passing zero argument was handled
    // above.
    if (ctorArgs.size() != 1) {
      Error::throwError(m_errorHandler, Error::eInvalidArguments,
        dataDef.ctorArgs().loc(), to_string(ctorArgs.size()), "1");
    }
    const auto initializer = ctorArgs.front();
    const auto& initializerObjType = initializer->object().objType();
    if (dataDef.objType().match(initializerObjType) == ObjType::eNoMatch) {
      Error::throwError(m_errorHandler, Error::eNoImplicitConversion,
        initializer->loc(), initializerObjType.completeName(),
        dataDef.objType().completeName());
    }
    if (dataDef.storageDuration() == StorageDuration::eStatic &&
      !initializer->isCTConst()) {
      Error::throwError(m_errorHandler, Error::eCTConstRequired, dataDef.loc());
    }
  }

  // -- responsibility 3: set properties of associated object: type, sd, access
  // As with all definitions, TemplateInstanciator, did already define type and
  // sd
  dataDef.notifyInitialized();

  postConditionCheck(dataDef);
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
  const auto& conditionObjType = if_.condition().object().objType();
  if (!conditionObjType.matchesSaufQualifiers(
        ObjTypeFunda(ObjTypeFunda::eBool))) {
    Error::throwError(m_errorHandler, Error::eNoImplicitConversion,
      if_.condition().loc(), conditionObjType.completeName(),
      ObjTypeFunda(ObjTypeFunda::eBool).completeName());
  }

  // In case of two clauses, check that both clauses are of the same obj type,
  // sauf qualifiers. Currently there are no implicit conversions.  As an
  // exception, if one of the two clauses is of type noreturn, then the if
  // expression's type is that of the other clause.
  const bool actionIsOfTypeNoreturn =
    if_.action().object().objType().isNoreturn();
  if (if_.elseAction()) {
    auto& lhs = if_.action().object().objType();
    auto& rhs = if_.elseAction()->object().objType();
    if (!lhs.matchesSaufQualifiers(rhs)) {
      if (!actionIsOfTypeNoreturn && !rhs.isNoreturn()) {
        Error::throwError(m_errorHandler, Error::eNoImplicitConversion,
          if_.loc(), rhs.completeName(), lhs.completeName());
      }
    }
  }

  // -- responsibility 3: set properties of associated object: type, sd, access
  // The AstIf's obj type is a temporary with the underlying type of either
  // clause; now that we now both clauses have the same type.
  if (if_.elseAction()) {
    if (!actionIsOfTypeNoreturn) {
      if_.setObjectType(if_.action().object().objType().unqualifiedObjType());
    }
    else {
      if_.setObjectType(
        if_.elseAction()->object().objType().unqualifiedObjType());
    }
  }
  else {
    if_.setObjectType(make_shared<ObjTypeFunda>(ObjTypeFunda::eVoid));
  }

  postConditionCheck(if_);
}

void SemanticAnalizer::visit(AstLoop& loop) {
  preConditionCheck(loop);

  // -- responsibility 1: set access to direct childs and descent AST subtree
  setAccessAndCallAcceptOn(loop.condition(), Access::eRead);
  setAccessAndCallAcceptOn(loop.body(), Access::eIgnoreValueAndAddr);

  // -- responsibility 2: semantic analysis
  const auto& conditionObjType = loop.condition().object().objType();
  if (!conditionObjType.matchesSaufQualifiers(
        ObjTypeFunda(ObjTypeFunda::eBool))) {
    Error::throwError(m_errorHandler, Error::eNoImplicitConversion,
      loop.condition().loc(), conditionObjType.completeName(),
      ObjTypeFunda(ObjTypeFunda::eBool).completeName());
  }

  // -- responsibility 3: set properties of associated object: type, sd, access
  // nop - done by AST node itself

  postConditionCheck(loop);
}

void SemanticAnalizer::visit(AstReturn& return_) {
  preConditionCheck(return_);

  auto&& ctorArgs = return_.ctorArgs().childs();

  // -- responsibility 1: set access to direct childs and descent AST subtree
  for (const auto arg : ctorArgs) {
    setAccessAndCallAcceptOn(*arg, Access::eRead);
  }

  // -- responsibility 2: semantic analysis
  if (m_funRetAstObjTypes.empty()) {
    // todo: this fails e.g. within a class def which is within a function
    // def. But anyway, the grammar should change such that runtime expressions
    // only appear within a function def or initializer of class member variable.
    Error::throwError(
      m_errorHandler, Error::eNotInFunBodyContext, return_.loc());
  }
  if (ctorArgs.size() != 1U) {
    Error::throwError(m_errorHandler, Error::eInvalidArguments,
      return_.ctorArgs().loc(), to_string(ctorArgs.size()), "1");
  }
  const auto& definedReturnObjType = m_funRetAstObjTypes.top()->objType();
  const auto& returnedObjType = ctorArgs.front()->object().objType();
  if (!returnedObjType.matchesSaufQualifiers(definedReturnObjType)) {
    Error::throwError(m_errorHandler, Error::eNoImplicitConversion,
      return_.loc(), returnedObjType.completeName(),
      definedReturnObjType.completeName());
  }

  // -- responsibility 3: set properties of associated object: type, sd, access
  // nop - done by AST node itself

  postConditionCheck(return_);
}

void SemanticAnalizer::visit(AstObjTypeSymbol& symbol) {
  // nop, everything was already done in previous passes
  preConditionCheck(symbol);
  if (symbol.name() == "infer") {
    Error::throwError(
      m_errorHandler, Error::eTypeInferenceIsNotYetSupported, symbol.loc());
  }
  postConditionCheck(symbol);
}

void SemanticAnalizer::visit(AstObjTypeQuali& quali) {
  // nop, everything was already done in previous passes
  preConditionCheck(quali);
  quali.targetType().accept(*this);
  postConditionCheck(quali);
}

void SemanticAnalizer::visit(AstObjTypePtr& ptr) {
  // nop, everything was already done in previous passes
  preConditionCheck(ptr);
  ptr.pointee().accept(*this);
  postConditionCheck(ptr);
}

void SemanticAnalizer::visit(AstClassDef& class_) {
  preConditionCheck(class_);
  for (const auto& dataMember : class_.dataMembers()) {
    setAccessAndCallAcceptOn(*dataMember, Access::eIgnoreValueAndAddr);
  }
  postConditionCheck(class_);
}

void SemanticAnalizer::preConditionCheck(const AstObject& node) {
  assert(node.accessFromAstParent() != Access::eYetUndefined);
}

void SemanticAnalizer::preConditionCheck(const AstObjType& node) {
  // none yet
}

void SemanticAnalizer::postConditionCheck(const AstObject& node) {
  assert(node.object().objTypeAsSp()); //todo:move to TemplateInstanciator
  assert(node.object().storageDuration() != StorageDuration::eYetUndefined);
}

void SemanticAnalizer::postConditionCheck(const AstObjType& node) {
  // If the reference returned by objType() was initialized with nullptr, that
  // is undefined behavior. In practice however taking the address of it will
  // deliver again a nullptr, and we assert for that.
  assert(&node.objType());
}

// The purpose of this method is to help to remember that these two (almost)
// always go together
void SemanticAnalizer::setAccessAndCallAcceptOn(AstNode& node, Access access) {
  node.setAccessFromAstParent(access);
  node.accept(*this);
}
