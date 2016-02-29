#include "parserext.h"
#include "env.h"
#include "ast.h"
#include "errorhandler.h"
#include <cassert>
#include <stdexcept>
using namespace std;

RawAstDataDef::RawAstDataDef(ErrorHandler& errorHandler) :
  m_errorHandler{errorHandler},
  m_ctorArgs{},
  m_astObjType{},
  m_isStorageDurationDefined{false},
  m_storageDuration{StorageDuration::eLocal} {
}

RawAstDataDef::RawAstDataDef(ErrorHandler& errorHandler, const std::string& name,
  AstCtList* ctorArgs, AstObjType* astObjType,
  StorageDuration storageDuration) :
  m_errorHandler{errorHandler},
  m_name(name),
  m_ctorArgs(ctorArgs),
  m_astObjType(astObjType),
  m_isStorageDurationDefined(false),
  m_storageDuration(storageDuration) {
}

void RawAstDataDef::setName(const std::string& name) {
  if ( !m_name.empty() ) {
    Error::throwError(m_errorHandler, Error::eSameArgWasDefinedMultipleTimes);
  }
  m_name = name;
}

void RawAstDataDef::setCtorArgs(AstCtList* ctorArgs) {
  if ( m_ctorArgs ) {
    Error::throwError(m_errorHandler, Error::eSameArgWasDefinedMultipleTimes);
  }
  m_ctorArgs = ctorArgs;
}

void RawAstDataDef::setAstObjType(AstObjType* astObjType) {
  if ( m_astObjType ) {
    Error::throwError(m_errorHandler, Error::eSameArgWasDefinedMultipleTimes);
  }
  m_astObjType = astObjType;
}

void RawAstDataDef::setStorageDuration(StorageDuration storageDuration) {
  if ( m_isStorageDurationDefined ) {
    Error::throwError(m_errorHandler, Error::eSameArgWasDefinedMultipleTimes);
  }
  m_isStorageDurationDefined = true;
  m_storageDuration = storageDuration;
}

ParserExt::ParserExt(Env&, ErrorHandler& errorHandler) :
  m_errorHandler(errorHandler) {
}

/** Turns the AstCtList in an AstOperator tree with at most two childs per
node.  The AstCtList object is deleted, its ex-childs are now owned by their
respective AstOperator parent. */
AstOperator* ParserExt::mkOperatorTree(const string& op_as_str, AstCtList* args) {
  assert(args);
  args->releaseOwnership();
  const auto op = AstOperator::toEOperationPreferingBinary(op_as_str);
  AstOperator* tree = NULL;

  // unary operator
  if (op == '!') {
    assert(args->childs().size() == 1);
    tree = new AstOperator(op, args->childs().front());
  }

  // right associative binary operator
  else if ( op == '=' ) {
    assert(args->childs().size() >= 2);
    auto ri = args->childs().rbegin();
    AstObject* last = *ri;
    AstObject* beforelast = *(++ri);
    tree = new AstOperator(op, beforelast, last);
    for ( ++ri; ri!=args->childs().rend(); ++ri ) {
      tree = new AstOperator(op, *ri, tree);
    }
  }

  // left associative binary operator
  else {
    assert(args->childs().size() >= 2);
    auto i = args->childs().begin();
    AstObject* first = *i;
    AstObject* second = *(++i);
    tree = new AstOperator(op, first, second);
    for ( ++i; i!=args->childs().end(); ++i ) {
      tree = new AstOperator(op, tree, *i);
    }
  }

  delete args;
  assert(tree);
  return tree;
}

AstOperator* ParserExt::mkOperatorTree(const string& op, AstObject* child1,
  AstObject* child2, AstObject* child3, AstObject* child4, AstObject* child5,
  AstObject* child6) {
  return mkOperatorTree(op, new AstCtList(child1, child2, child3, child4, child5,
      child6));
}

AstDataDef* ParserExt::mkDataDef(ObjType::Qualifiers qualifiers,
  RawAstDataDef*& rawAstDataDef) {

  assert(rawAstDataDef);
  assert(!rawAstDataDef->m_name.empty());
  const auto unqualifiedAstObjType = rawAstDataDef->m_astObjType ?
    rawAstDataDef->m_astObjType : new AstObjTypeSymbol(ObjType::eInt);
  const auto qualifiedAstObjType =
    new AstObjTypeQuali(qualifiers, unqualifiedAstObjType);

  return new AstDataDef(
    rawAstDataDef->m_name,
    qualifiedAstObjType,
    rawAstDataDef->m_storageDuration,
    rawAstDataDef->m_ctorArgs);
}

AstFunDef* ParserExt::mkFunDef(const string name, vector<AstDataDef*>* astArgs,
  AstObjType* retAstObjType, AstObject* astBody) {
  astArgs = astArgs ? astArgs : new vector<AstDataDef*>();
  return new AstFunDef(name, astArgs, retAstObjType, astBody);
}

AstFunDef* ParserExt::mkFunDef(const string name, ObjType::EType ret,
  AstObject* body) {
  return mkFunDef(name, AstFunDef::createArgs(), new AstObjTypeSymbol(ret),
    body);
}

AstFunDef* ParserExt::mkFunDef(const string name, AstObjType* ret,
  AstObject* body) {
  return mkFunDef(name, AstFunDef::createArgs(), ret, body);
}

AstFunDef* ParserExt::mkMainFunDef(AstObject* body) {
  return mkFunDef("main", new AstObjTypeSymbol(ObjType::eInt), body);
}
