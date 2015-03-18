#include "semanticanalizer.h"
#include "ast.h"
#include "astdefaultiterator.h"
#include "env.h"
#include "errorhandler.h"
#include <stdexcept>
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
  postConditionCheck(cast);
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

  if (AstOperator::eAssign == op.op()) {
    argschilds.front()->setAccess(eWrite, m_errorHandler);
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

  // Set the obj type of this AstOperator node. It is a temporary object which
  // is always immutable. For the sequence operator this is the type of the
  // rhs. For the other operators, now that we know the two operands have the
  // same obj type, either operand's obj type can be taken.
  auto objType = unique_ptr<ObjType>(argschilds.back()->objType().clone());
  objType->removeQualifiers(ObjType::eMutable);
  op.setObjType(move(objType));

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

  m_env.pushScope();

  list<AstArgDecl*>::const_iterator argDeclIter = funDef.decl().args().begin();
  list<AstArgDecl*>::const_iterator end = funDef.decl().args().end();
  for ( ; argDeclIter!=end; ++argDeclIter) {
    (*argDeclIter)->accept(*this);
    (*argDeclIter)->stentry()->markAsDefined(m_errorHandler);
  }

  funDef.body().accept(*this);

  m_env.popScope();

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
  if_.action().accept(*this);
  if (if_.elseAction()) {
    if_.elseAction()->accept(*this);
  }

  if ( !if_.condition().objType().matchesSaufQualifiers(
      ObjTypeFunda(ObjTypeFunda::eBool)) ) {
    Error::throwError(m_errorHandler, Error::eNoImplicitConversion);
  }

  // In case of two clauses, check that both clauses are of the same obj type,
  // sauf qualifiers. Currently there are no implicit conversions
  if ( if_.elseAction() ) {
    auto& lhs = if_.action().objType();
    auto& rhs = if_.elseAction()->objType();
    if ( !lhs.matchesSaufQualifiers(rhs) ) {
      Error::throwError(m_errorHandler, Error::eNoImplicitConversion);
    }
  }

  // Set the obj type of this AstIf node. It is a temporary object which is
  // always immutable. Now that we know the two clauses have the same obj
  // type, either clause's obj type can be taken.
  if ( if_.elseAction() ) {
    auto objType = unique_ptr<ObjType>(if_.action().objType().clone());
    objType->removeQualifiers(ObjType::eMutable);
    if_.setObjType(move(objType));
  } else {
    // KLUDGE. It should be void, but that type does not yet exist.
  }

  postConditionCheck(if_);
}

void SemanticAnalizer::postConditionCheck(const AstValue& node) {
  assert((node.objType(),true)); // asserts internally if not defined
}
