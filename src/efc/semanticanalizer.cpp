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

void SemanticAnalizer::visit(AstCast& cast) { }

void SemanticAnalizer::visit(AstCtList& ctList) {
  list<AstValue*>& childs = ctList.childs();
  for (list<AstValue*>::const_iterator i=childs.begin();
       i!=childs.end(); ++i) {
    (*i)->accept(*this);
  }
}

void SemanticAnalizer::visit(AstOperator& op) {
  const list<AstValue*>& argschilds = op.args().childs();

  switch (op.op()) {
  case AstOperator::eSeq: {
    if (argschilds.empty()) {
      throw runtime_error("Empty sequence not allowed (yet)");
    }
    break;
  }
  case AstOperator::eAssign: {
    assert( !argschilds.empty() );
    argschilds.front()->setAccess(eWrite, m_errorHandler);
    break;
  }
  default:
    break;
  }
  op.args().accept(*this);
}

void SemanticAnalizer::visit(AstNumber& number) { }

void SemanticAnalizer::visit(AstSymbol& symbol) {
  shared_ptr<SymbolTableEntry> stentry;
  m_env.find(symbol.name(), stentry);
  if (NULL==stentry) {
    m_errorHandler.add(new Error(Error::eUnknownName));
    throw BuildError();
  }
  symbol.setStentry(move(stentry));
  if (symbol.access()==eWrite &&
    !(symbol.objType().qualifiers() & ObjType::eMutable)) {
    m_errorHandler.add(new Error(Error::eWriteToImmutable));
    throw BuildError();
  }
}

void SemanticAnalizer::visit(AstFunCall& funCall) {
  funCall.address().accept(*this);
  funCall.args().accept(*this);
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
}

void SemanticAnalizer::visit(AstFunDecl& funDecl) {
  // Note that visit(AstFundDef) does all of the work for the case AstFunDecl
  // is a child of AstFundDef.
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
        m_errorHandler.add(new Error(Error::eIncompatibleRedaclaration));
        throw BuildError();
      }
    }

    dataDecl.setStentry(envs_stentry_ptr);
  }
}

void SemanticAnalizer::visit(AstArgDecl& argDecl) {
  visit( dynamic_cast<AstDataDecl&>(argDecl));
}

void SemanticAnalizer::visit(AstDataDef& dataDef) {
  dataDef.decl().accept(*this);
  dataDef.decl().stentry()->markAsDefined(m_errorHandler);
  dataDef.ctorArgs().accept(*this);
  if ( dataDef.decl().objType().match(dataDef.initValue().objType()) == ObjType::eNoMatch ) {
    throw runtime_error("Object type missmatch");
  }
}

void SemanticAnalizer::visit(AstIf& if_) {
  list<AstIf::ConditionActionPair>::iterator i = if_.conditionActionPairs().begin();
  for ( /*nop*/; i!=if_.conditionActionPairs().end(); ++i ) {
    i->m_condition->accept(*this);
    i->m_action->accept(*this);
  }
  if (if_.elseAction()) {
    if_.elseAction()->accept(*this);
  }
}
