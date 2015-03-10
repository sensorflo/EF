#include "semanticanalizer.h"
#include "ast.h"
#include "astdefaultiterator.h"
#include "env.h"
#include "errorhandler.h"
#include <stdexcept>
using namespace std;

SemanticAnalizer::SemanticAnalizer(Env& env, ErrorHandler& errorHandler,
  AstVisitor* nextVisitor) :
  m_env(env),
  m_errorHandler(errorHandler),
  m_nextVisitor(nextVisitor ? *nextVisitor : *new AstDefaultIterator(*this)),
  m_ownsNextVisitor(nextVisitor==NULL) {
}

SemanticAnalizer::~SemanticAnalizer() {
  if (m_ownsNextVisitor) {
    delete &m_nextVisitor;
  }
}

void SemanticAnalizer::visit(AstCast& cast) {
  m_nextVisitor.visit(cast);
}

void SemanticAnalizer::visit(AstCtList& ctList) {
  m_nextVisitor.visit(ctList);
}

void SemanticAnalizer::visit(AstOperator& op) {
  const list<AstValue*>& argschilds = op.args().childs();
  switch (op.op()) {
  case AstOperator::eSeq:
    if (argschilds.empty()) {
      throw runtime_error("Empty sequence not allowed (yet)");
    }
    break;
  default:
    break;
  }
  m_nextVisitor.visit(op);
}

void SemanticAnalizer::visit(AstNumber& number) {
  m_nextVisitor.visit(number);
}

void SemanticAnalizer::visit(AstSymbol& symbol) {
  shared_ptr<SymbolTableEntry> stentry;
  m_env.find(symbol.name(), stentry);
  if (NULL==stentry) {
    m_errorHandler.add(new Error(Error::eUnknownName));
    throw BuildError();
  }
  symbol.setStentry(stentry);
  m_nextVisitor.visit(symbol);
}

void SemanticAnalizer::visit(AstFunCall& funCall) {
  m_nextVisitor.visit(funCall);
}

void SemanticAnalizer::visit(AstFunDef& funDef) {
  m_nextVisitor.visit(funDef);
}

void SemanticAnalizer::visit(AstFunDecl& funDecl) {
  m_nextVisitor.visit(funDecl);
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

  m_nextVisitor.visit(dataDecl);
}

void SemanticAnalizer::visit(AstArgDecl& argDecl) {
  m_nextVisitor.visit(argDecl);
}

void SemanticAnalizer::visit(AstDataDef& dataDef) {
  // initializer must be of same type. Currently there are no implicit conversions
  if ( dataDef.decl().objType().match(dataDef.initValue().objType()) == ObjType::eNoMatch ) {
    throw runtime_error("Object type missmatch");
  }
  m_nextVisitor.visit(dataDef);
}

void SemanticAnalizer::visit(AstIf& if_) {
  m_nextVisitor.visit(if_);
}
