#include "semanticanalizer.h"
#include "ast.h"
#include "astdefaultiterator.h"
#include "errorhandler.h"
#include <stdexcept>
using namespace std;

SemanticAnalizer::SemanticAnalizer(ErrorHandler& errorHandler,
  AstVisitor* nextVisitor) :
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
      throw runtime_error::runtime_error("Empty sequence not allowed (yet)");
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
  m_nextVisitor.visit(dataDecl);
}

void SemanticAnalizer::visit(AstArgDecl& argDecl) {
  m_nextVisitor.visit(argDecl);
}

void SemanticAnalizer::visit(AstDataDef& dataDef) {
  // initializer must be of same type. Currently there are no implicit conversions
  if ( ObjType::eNoMatch ==
    dataDef.decl().objType().match(dataDef.initValue().objType()) ) {
    throw runtime_error::runtime_error("Object type missmatch");
  }
  m_nextVisitor.visit(dataDef);
}

void SemanticAnalizer::visit(AstIf& if_) {
  m_nextVisitor.visit(if_);
}
