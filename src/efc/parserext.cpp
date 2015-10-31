#include "parserext.h"
#include "env.h"
#include "object.h"
#include "ast.h"
#include "errorhandler.h"
#include <cassert>
#include <stdexcept>
using namespace std;

RawAstDataDef::RawAstDataDef(ErrorHandler& errorHandler) :
  m_errorHandler{errorHandler},
  m_ctorArgs{},
  m_isStorageDurationDefined{false},
  m_storageDuration{StorageDuration::eLocal} {
}

RawAstDataDef::RawAstDataDef(ErrorHandler& errorHandler, const std::string& name,
  AstCtList* ctorArgs, shared_ptr<const ObjType> objType,
  StorageDuration storageDuration) :
  m_errorHandler{errorHandler},
  m_name(name),
  m_ctorArgs(ctorArgs),
  m_objType(move(objType)),
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

void RawAstDataDef::setObjType(shared_ptr<const ObjType> objType) {
  if ( m_objType ) {
    Error::throwError(m_errorHandler, Error::eSameArgWasDefinedMultipleTimes);
  }
  m_objType = move(objType);
}

void RawAstDataDef::setStorageDuration(StorageDuration storageDuration) {
  if ( m_isStorageDurationDefined ) {
    Error::throwError(m_errorHandler, Error::eSameArgWasDefinedMultipleTimes);
  }
  m_isStorageDurationDefined = true;
  m_storageDuration = storageDuration;
}

ParserExt::ParserExt(Env& env, ErrorHandler& errorHandler) :
  m_env(env), m_errorHandler(errorHandler) {
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
    reverse_iterator<list<AstObject*>::iterator> ri = args->childs().rbegin();
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
    list<AstObject*>::iterator i = args->childs().begin();
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
  const auto unqualifiedObjType = rawAstDataDef->m_objType ?
    rawAstDataDef->m_objType : make_shared<ObjTypeFunda>(ObjTypeFunda::eInt);
  const auto qualifiedobjType =
    make_shared<ObjTypeQuali>(qualifiers, unqualifiedObjType);

  // add to environment everything except local data objects. Currently there
  // are only local data objects, so currently there is nothing todo.

  return new AstDataDef(
    rawAstDataDef->m_name,
    qualifiedobjType,
    rawAstDataDef->m_storageDuration,
    rawAstDataDef->m_ctorArgs);
}

AstFunDef* ParserExt::mkFunDef(const string name, list<AstDataDef*>* astArgs,
  shared_ptr<const ObjType> retObjType, AstObject* astBody) {

  // Insert name into environment. Note that currently there is a flat
  // namespace regarding function names; i.e. also nested functions are
  // nevertheless in the flat global namespace.
  auto&& insertRet = m_env.insert(name, nullptr);
  const auto& wasAlreadyInMap = !insertRet.second;
  if (wasAlreadyInMap) {
    Error::throwError(m_errorHandler, Error::eRedefinition);
    return nullptr;
  }
  auto& entityInEnvSp = insertRet.first->second;

  // default for astArgs
  astArgs = astArgs ? astArgs : new list<AstDataDef*>();

  // create function object type describing the function and an function
  // object representing the function
  const auto& argsObjType = new list<shared_ptr<const ObjType>>;
  for (const auto& astArg: *astArgs) {
    argsObjType->push_back(astArg->declaredObjTypeAsSp());
  }
  auto&& objTypeFun = make_shared<const ObjTypeFun>(argsObjType, retObjType);
  auto&& funObjSp = make_shared<Object>(move(objTypeFun), StorageDuration::eStatic);

  // let environment node point to new function object
  entityInEnvSp = funObjSp;

  // finaly create AST node representing the function
  return new AstFunDef(name, move(funObjSp), astArgs, move(retObjType), astBody);
}

AstFunDef* ParserExt::mkFunDef(const string name, ObjTypeFunda::EType ret,
  AstObject* body) {
  return mkFunDef(name, AstFunDef::createArgs(),
    make_shared<const ObjTypeFunda>(ret), body);
}

AstFunDef* ParserExt::mkFunDef(const string name, shared_ptr<const ObjType> ret,
  AstObject* body) {
  return mkFunDef(name, AstFunDef::createArgs(), move(ret), body);
}

AstFunDef* ParserExt::mkMainFunDef(AstObject* body) {
  return mkFunDef("main", make_shared<ObjTypeFunda>(ObjTypeFunda::eInt), body);
}

AstClassDef* ParserExt::mkClassDef(const string name,
  vector<AstDataDef*>* astDataMembers) {

  assert(astDataMembers);

  // Insert name into environment
  auto&& insertRet = m_env.insert(name, nullptr);
  const auto& wasAlreadyInMap = !insertRet.second;
  if (wasAlreadyInMap) {
    Error::throwError(m_errorHandler, Error::eRedefinition);
    return nullptr;
  }
  auto& entityInEnvSp = insertRet.first->second;

  // create class object type
  vector<shared_ptr<const ObjType>> dataMemberObjTypes;
  for (const auto& astDataMember: *astDataMembers) {
    dataMemberObjTypes.emplace_back(astDataMember->declaredObjTypeAsSp());
  }
  auto&& objTypeClass = make_shared<ObjTypeClass>(name,
    move(dataMemberObjTypes));

  // let environment node point to newly created class object type
  entityInEnvSp = objTypeClass;

  // finaly create AST node representing the class definition
  return new AstClassDef(name, astDataMembers, objTypeClass);
}

