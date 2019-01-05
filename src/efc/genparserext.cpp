#include "genparserext.h"

#include "ast.h"
#include "env.h"
#include "errorhandler.h"

#include <cassert>
#include <stdexcept>
#include <utility>

using namespace std;

namespace {
AstCtList* combine(ErrorHandler& errorHandler, AstCtList* ctorArgs1,
  AstCtList* ctorArgs2, const Location& loc) {
  if (ctorArgs1 && ctorArgs2) {
    Error::throwError(errorHandler, Error::eMultipleInitializers, loc);
  }
  else if (!ctorArgs1 && !ctorArgs2) {
    return new AstCtList(loc);
  }
  else {
    return ctorArgs1 ? ctorArgs1 : ctorArgs2;
  }
  assert(false);
  return nullptr;
}
}

RawAstDataDef::RawAstDataDef(ErrorHandler& errorHandler, string name,
  AstCtList* ctorArgs1, AstCtList* ctorArgs2, AstObjType* astObjType,
  StorageDuration storageDuration, const Location& loc)
  : m_errorHandler{errorHandler}
  , m_name(move(name))
  , m_ctorArgs(combine(m_errorHandler, ctorArgs1, ctorArgs2, loc))
  , m_astObjType(astObjType)
  , m_storageDuration(storageDuration) {
}

GenParserExt::GenParserExt(Env&, ErrorHandler& errorHandler)
  : m_errorHandler(errorHandler) {
}

AstObjType* GenParserExt::mkDefaultType(Location loc) {
  return new AstObjTypeSymbol(ObjTypeFunda::eInfer, move(loc));
}

StorageDuration GenParserExt::mkDefaultStorageDuration() {
  return StorageDuration::eLocal;
}

ObjType::Qualifiers GenParserExt::mkDefaultObjectTypeQualifier() {
  return ObjType::eNoQualifier;
}

/** Turns the AstCtList in an AstOperator tree with at most two childs per
node.  The AstCtList object is deleted, its ex-childs are now owned by their
respective AstOperator parent. */
AstOperator* GenParserExt::mkOperatorTree(
  const string& op_as_str, AstCtList* args, Location loc) {
  assert(args);
  args->releaseOwnership();
  const auto op = AstOperator::toEOperationPreferingBinary(op_as_str);
  AstOperator* tree = nullptr;

  // unary operator
  if (op == '!') {
    assert(args->childs().size() == 1);
    tree = new AstOperator(op, args->childs().front(), nullptr, move(loc));
  }

  // right associative binary operator
  else if (op == '=') {
    assert(args->childs().size() >= 2);
    auto ri = args->childs().rbegin();
    AstObject* last = *ri;
    AstObject* beforelast = *(++ri);
    tree = new AstOperator(op, beforelast, last, loc);
    for (++ri; ri != args->childs().rend(); ++ri) {
      tree = new AstOperator(op, *ri, tree, loc);
    }
  }

  // left associative binary operator
  else {
    assert(args->childs().size() >= 2);
    auto i = args->childs().begin();
    AstObject* first = *i;
    AstObject* second = *(++i);
    tree = new AstOperator(op, first, second, loc);
    for (++i; i != args->childs().end(); ++i) {
      tree = new AstOperator(op, tree, *i, loc);
    }
  }

  delete args;
  assert(tree);
  return tree;
}

AstOperator* GenParserExt::mkOperatorTree(const string& op, AstObject* child1,
  AstObject* child2, AstObject* child3, AstObject* child4, AstObject* child5,
  AstObject* child6) {
  return mkOperatorTree(
    op, new AstCtList(child1, child2, child3, child4, child5, child6));
}

AstDataDef* GenParserExt::mkDataDef(
  ObjType::Qualifiers qualifiers, RawAstDataDef*& rawAstDataDef, Location loc) {
  assert(rawAstDataDef);

  // 1) location of data declaration is also used as location of its implicit
  //    type and its explicit type qualifiers
  const auto unqualifiedAstObjType = rawAstDataDef->m_astObjType != nullptr
    ? rawAstDataDef->m_astObjType
    : mkDefaultType(loc); // 1)
  const auto qualifiedAstObjType =
    new AstObjTypeQuali(qualifiers, unqualifiedAstObjType, loc); // 2)

  return new AstDataDef(rawAstDataDef->m_name, qualifiedAstObjType,
    rawAstDataDef->m_storageDuration, rawAstDataDef->m_ctorArgs, loc);
}

AstFunDef* GenParserExt::mkFunDef(const string name,
  vector<AstDataDef*>* astArgs, AstObjType* retAstObjType, AstObject* astBody,
  Location loc) {
  astArgs = astArgs ? astArgs : new vector<AstDataDef*>();
  return new AstFunDef(name, astArgs, retAstObjType, astBody, move(loc));
}

AstFunDef* GenParserExt::mkFunDef(
  const string name, ObjTypeFunda::EType ret, AstObject* body, Location loc) {
  return mkFunDef(
    name, AstFunDef::createArgs(), new AstObjTypeSymbol(ret), body, move(loc));
}

AstFunDef* GenParserExt::mkFunDef(
  const string name, AstObjType* ret, AstObject* body, Location loc) {
  return mkFunDef(name, AstFunDef::createArgs(), ret, body, move(loc));
}

AstFunDef* GenParserExt::mkMainFunDef(AstObject* body) {
  // note that a valid Location is created, opposed to passing s_nullLoc
  return mkFunDef("main", new AstObjTypeSymbol(ObjTypeFunda::eInt, Location{}),
    body, Location{});
}
