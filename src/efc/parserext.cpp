#include "parserext.h"

#include "ast.h"
#include "env.h"
#include "errorhandler.h"

#include <cassert>
#include <stdexcept>
#include <utility>

using namespace std;

namespace {
AstCtList* combine(
  ErrorHandler& errorHandler, AstCtList* ctorArgs1, AstCtList* ctorArgs2) {
  if (ctorArgs1 && ctorArgs2) {
    Error::throwError(errorHandler, Error::eSameArgWasDefinedMultipleTimes);
  }
  else if (!ctorArgs1 && !ctorArgs2) {
    return new AstCtList();
  }
  else {
    return ctorArgs1 ? ctorArgs1 : ctorArgs2;
  }
  assert(false);
  return nullptr;
}
}

RawAstDataDef::RawAstDataDef(ErrorHandler& errorHandler, std::string name,
  AstCtList* ctorArgs1, AstCtList* ctorArgs2, AstObjType* astObjType,
  StorageDuration storageDuration)
  : m_errorHandler{errorHandler}
  , m_name(std::move(name))
  , m_ctorArgs(combine(m_errorHandler, ctorArgs1, ctorArgs2))
  , m_astObjType(astObjType)
  , m_storageDuration(storageDuration) {
}

ParserExt::ParserExt(Env&, ErrorHandler& errorHandler)
  : m_errorHandler(errorHandler) {
}

AstObjType* ParserExt::mkDefaultType() {
  return new AstObjTypeSymbol(ObjTypeFunda::eInt);
}

StorageDuration ParserExt::mkDefaultStorageDuration() {
  return StorageDuration::eLocal;
}

ObjType::Qualifiers ParserExt::mkDefaultObjectTypeQualifier() {
  return ObjType::eNoQualifier;
}

/** Turns the AstCtList in an AstOperator tree with at most two childs per
node.  The AstCtList object is deleted, its ex-childs are now owned by their
respective AstOperator parent. */
AstOperator* ParserExt::mkOperatorTree(
  const string& op_as_str, AstCtList* args) {
  assert(args);
  args->releaseOwnership();
  const auto op = AstOperator::toEOperationPreferingBinary(op_as_str);
  AstOperator* tree = nullptr;

  // unary operator
  if (op == '!') {
    assert(args->childs().size() == 1);
    tree = new AstOperator(op, args->childs().front());
  }

  // right associative binary operator
  else if (op == '=') {
    assert(args->childs().size() >= 2);
    auto ri = args->childs().rbegin();
    AstObject* last = *ri;
    AstObject* beforelast = *(++ri);
    tree = new AstOperator(op, beforelast, last);
    for (++ri; ri != args->childs().rend(); ++ri) {
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
    for (++i; i != args->childs().end(); ++i) {
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
  return mkOperatorTree(
    op, new AstCtList(child1, child2, child3, child4, child5, child6));
}

AstDataDef* ParserExt::mkDataDef(
  ObjType::Qualifiers qualifiers, RawAstDataDef*& rawAstDataDef) {
  assert(rawAstDataDef);
  const auto unqualifiedAstObjType = rawAstDataDef->m_astObjType != nullptr
    ? rawAstDataDef->m_astObjType
    : mkDefaultType();
  const auto qualifiedAstObjType =
    new AstObjTypeQuali(qualifiers, unqualifiedAstObjType);

  return new AstDataDef(rawAstDataDef->m_name, qualifiedAstObjType,
    rawAstDataDef->m_storageDuration, rawAstDataDef->m_ctorArgs);
}

AstFunDef* ParserExt::mkFunDef(const string name, vector<AstDataDef*>* astArgs,
  AstObjType* retAstObjType, AstObject* astBody) {
  astArgs = astArgs ? astArgs : new vector<AstDataDef*>();
  return new AstFunDef(name, astArgs, retAstObjType, astBody);
}

AstFunDef* ParserExt::mkFunDef(
  const string name, ObjTypeFunda::EType ret, AstObject* body) {
  return mkFunDef(
    name, AstFunDef::createArgs(), new AstObjTypeSymbol(ret), body);
}

AstFunDef* ParserExt::mkFunDef(
  const string name, AstObjType* ret, AstObject* body) {
  return mkFunDef(name, AstFunDef::createArgs(), ret, body);
}

AstFunDef* ParserExt::mkMainFunDef(AstObject* body) {
  return mkFunDef("main", new AstObjTypeSymbol(ObjTypeFunda::eInt), body);
}
