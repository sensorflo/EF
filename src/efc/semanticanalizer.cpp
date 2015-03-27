#include "semanticanalizer.h"
#include "ast.h"
#include "astdefaultiterator.h"
#include "env.h"
#include "errorhandler.h"
#include "memoryext.h"
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
  // ObjType must not be set, AstCast knows its own ObjType

  //test if conversion is eligible, i.e. if such a constructor exists
  if ( !cast.objType().hasConstructor(cast.objType()) ) {
    Error::throwError(m_errorHandler, Error::eNoSuchMember);
  }

  postConditionCheck(cast);
}

void SemanticAnalizer::visit(AstNop& nop) {
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

  // Set EAccess of childs.
  {
    if (AstOperator::eAssignment == op.class_()) {
      argschilds.front()->setAccess(eWrite, m_errorHandler);
    } else if ( AstOperator::eSeq == op.op() ) {
      argschilds.front()->setAccess(eIgnore, m_errorHandler);
      argschilds.back()->setAccess(op.access(), m_errorHandler);
    }
  }

  op.args().accept(*this);

  // Check that all operands are of the same obj type, sauf
  // qualifiers. Currently there are no implicit conversions
  if ( argschilds.size()==2 ) {
    if ( op.class_() != AstOperator::eOther ) {
      auto& lhs = argschilds.front()->objType();
      auto& rhs = argschilds.back()->objType();
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

  // Verify that the first argument has the operator as member function.
  if ( op.op()!=AstOperator::eSeq // eSeq is a global operator, not a member function
    && !argschilds.front()->objType().hasMember(op.op())) {
    Error::throwError(m_errorHandler, Error::eNoSuchMember);
  }

  // Report eUnreachableCode if the lhs of an sequence operator is of obj type
  // noreturn.  That is actually also true for other operators, but there the
  // error eNoSuchMember has higher priority.
  if ( op.op()==AstOperator::eSeq ) {
    static const ObjTypeFunda noreturnObjType(ObjTypeFunda::eNoreturn);
    if ( argschilds.front()->objType().matchesFully(noreturnObjType) ) {
      Error::throwError(m_errorHandler, Error::eUnreachableCode);
    }
  }

  // Set the obj type of this AstOperator node. It is a temporary object which
  // is always immutable.
  {
    if ( op.class_() == AstOperator::eComparison ) {
      op.setObjType(make_unique<ObjTypeFunda>(ObjTypeFunda::eBool));
    } else if ( op.class_() == AstOperator::eAssignment ) {
      if ( op.op() == AstOperator::eDotAssign ) {
        op.setObjType(unique_ptr<ObjType>(argschilds.front()->objType().clone()));
        // don't remove mutable qualifier
      } else {
        op.setObjType(make_unique<ObjTypeFunda>(ObjTypeFunda::eVoid));
      }
    }
    // The obj type of the seq operator is exactly that of its rhs
    else if ( op.op() == AstOperator::eSeq ) {
      op.setObjType(unique_ptr<ObjType>(argschilds.back()->objType().clone()));
      // don't remove mutable qualifier
    }
    // For ther operands, the operator expression's objtype is, now that we
    // know the two operands have the same obj type, either operand's obj
    // type.
    else {
      auto objType = unique_ptr<ObjType>(argschilds.back()->objType().clone());
      objType->removeQualifiers(ObjType::eMutable);
      op.setObjType(move(objType));

    }
  }
  postConditionCheck(op);
}

void SemanticAnalizer::visit(AstNumber& number) {
  // should allready have been caught by scanner, thus assert.
  // Intended to catch errors when AST is build by hand instead
  // by parser, e.g. in tests.
  assert( number.objType().isValueInRange( number.value()));

  postConditionCheck(number);
}

void SemanticAnalizer::visit(AstSymbol& symbol) {
  shared_ptr<SymbolTableEntry> stentry;
  m_env.find(symbol.name(), stentry);
  if (NULL==stentry) {
    Error::throwError(m_errorHandler, Error::eUnknownName);
  }
  symbol.setStentry(move(stentry));
  if (symbol.access()==eWrite &&
    !(symbol.objType().qualifiers() & ObjType::eMutable)) {
    Error::throwError(m_errorHandler, Error::eWriteToImmutable);
  }

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

  // ObjType must not be set, AstFunCall knows its own ObjType

  postConditionCheck(funCall);
}

void SemanticAnalizer::visit(AstFunDef& funDef) {
  funDef.decl().accept(*this);

  funDef.decl().stentry()->markAsDefined(m_errorHandler);

  m_funRetObjTypes.push(&funDef.decl().retObjType());
  m_env.pushScope();

  list<AstArgDecl*>::const_iterator argDeclIter = funDef.decl().args().begin();
  list<AstArgDecl*>::const_iterator end = funDef.decl().args().end();
  for ( ; argDeclIter!=end; ++argDeclIter) {
    (*argDeclIter)->accept(*this);
    (*argDeclIter)->stentry()->markAsDefined(m_errorHandler);
  }

  funDef.body().accept(*this);

  const auto& bodyObjType = funDef.body().objType();
  static const ObjTypeFunda noreturnObjType(ObjTypeFunda::eNoreturn);
  if ( ! bodyObjType.matchesSaufQualifiers( funDef.decl().retObjType())
    && ! bodyObjType.matchesSaufQualifiers( noreturnObjType)) {
    Error::throwError(m_errorHandler, Error::eNoImplicitConversion);
  }

  m_env.popScope();
  m_funRetObjTypes.pop();

  postConditionCheck(funDef);
}

void SemanticAnalizer::visit(AstFunDecl& funDecl) {
  // Note that visit(AstFundDef) does all of the work for the case AstFunDecl
  // is a child of AstFundDef.
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
      envs_stentry_ptr = make_shared<SymbolTableEntry>(
        dataDecl.objTypeShareOwnership());
    }

    // name is already in env: unless the type matches that is an error
    else {
      assert(envs_stentry_ptr.get());
      if ( !envs_stentry_ptr->objType().matchesFully(dataDecl.objType()) ) {
        Error::throwError(m_errorHandler, Error::eIncompatibleRedaclaration);
      }
    }

    dataDecl.setStentry(envs_stentry_ptr);
  }

  postConditionCheck(dataDecl);
}

void SemanticAnalizer::visit(AstArgDecl& argDecl) {
  visit( dynamic_cast<AstDataDecl&>(argDecl));
}

void SemanticAnalizer::visit(AstDataDef& dataDef) {
  dataDef.decl().accept(*this);
  dataDef.decl().stentry()->markAsDefined(m_errorHandler);
  dataDef.ctorArgs().accept(*this);
  if ( dataDef.access()==eWrite &&
    !(dataDef.objType().qualifiers() & ObjType::eMutable)) {
    Error::throwError(m_errorHandler, Error::eWriteToImmutable);
  }
  if ( dataDef.decl().objType().match(dataDef.initValue().objType()) == ObjType::eNoMatch ) {
    Error::throwError(m_errorHandler, Error::eNoImplicitConversion);
  }
  postConditionCheck(dataDef);
}

void SemanticAnalizer::visit(AstIf& if_) {
  if_.condition().accept(*this);
  if_.action().setAccess(if_.access(), m_errorHandler);
  if_.action().accept(*this);
  if (if_.elseAction()) {
    if_.elseAction()->setAccess(if_.access(), m_errorHandler);
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
  static const ObjTypeFunda noreturnObjType(ObjTypeFunda::eNoreturn);
  const bool actionIsOfTypeNoreturn =
    if_.action().objType().matchesSaufQualifiers(noreturnObjType);
  if ( if_.elseAction() ) {
    auto& lhs = if_.action().objType();
    auto& rhs = if_.elseAction()->objType();
    if ( !lhs.matchesSaufQualifiers(rhs) ) {
      const bool elseActionIsOfTypeNoreturn =
        rhs.matchesSaufQualifiers(noreturnObjType);
      if ( !actionIsOfTypeNoreturn && !elseActionIsOfTypeNoreturn) {
        Error::throwError(m_errorHandler, Error::eNoImplicitConversion);
      }
    }
  }

  // Set the obj type of this AstIf node. It's exactly the type of the
  // evaluated clause, i.e. it's not a new (immutable) temporary.  Now that we
  // know the two clauses have the same obj type, either clause's obj type can
  // be taken.
  if ( if_.elseAction() ) {
    if ( !actionIsOfTypeNoreturn ) {
      if_.setObjType(unique_ptr<ObjType>(if_.action().objType().clone()));
    } else {
      if_.setObjType(unique_ptr<ObjType>(if_.elseAction()->objType().clone()));
    }
    // don't remove mutable qualifier
  } else {
    if_.setObjType(make_unique<ObjTypeFunda>(ObjTypeFunda::eVoid));
  }

  postConditionCheck(if_);
}

void SemanticAnalizer::visit(AstReturn& return_) {
  if ( m_funRetObjTypes.empty() ) {
    Error::throwError(m_errorHandler, Error::eNotInFunBodyContext);
  }
  const auto& currentFunReturnType = *m_funRetObjTypes.top();
  if ( ! return_.retVal().objType().matchesSaufQualifiers( currentFunReturnType ) ) {
    Error::throwError(m_errorHandler, Error::eNoImplicitConversion);
  }
}

void SemanticAnalizer::postConditionCheck(const AstValue& node) {
  assert((node.objType(),true)); // asserts internally if not defined
}
