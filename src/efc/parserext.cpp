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
  m_objType{},
  m_isStorageDurationDefined{false},
  m_storageDuration{StorageDuration::eLocal} {
}

RawAstDataDef::RawAstDataDef(ErrorHandler& errorHandler, const std::string& name,
  AstCtList* ctorArgs, ObjType* objType, StorageDuration storageDuration) :
  m_errorHandler{errorHandler},
  m_name(name),
  m_ctorArgs(ctorArgs),
  m_objType(objType),
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

void RawAstDataDef::setObjType(ObjType* objType) {
  if ( m_objType ) {
    Error::throwError(m_errorHandler, Error::eSameArgWasDefinedMultipleTimes);
  }
  m_objType = objType;
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
    reverse_iterator<list<AstValue*>::iterator> ri = args->childs().rbegin();
    AstValue* last = *ri;
    AstValue* beforelast = *(++ri);
    tree = new AstOperator(op, beforelast, last);
    for ( ++ri; ri!=args->childs().rend(); ++ri ) {
      tree = new AstOperator(op, *ri, tree);
    }
  }

  // left associative binary operator
  else {
    assert(args->childs().size() >= 2);
    list<AstValue*>::iterator i = args->childs().begin();
    AstValue* first = *i;
    AstValue* second = *(++i);
    tree = new AstOperator(op, first, second);
    for ( ++i; i!=args->childs().end(); ++i ) {
      tree = new AstOperator(op, tree, *i);
    }
  }

  delete args;
  assert(tree);
  return tree;
}

AstOperator* ParserExt::mkOperatorTree(const string& op, AstValue* child1,
  AstValue* child2, AstValue* child3, AstValue* child4, AstValue* child5,
  AstValue* child6) {
  return mkOperatorTree(op, new AstCtList(child1, child2, child3, child4, child5,
      child6));
}

AstDataDef* ParserExt::mkDataDef(ObjType::Qualifiers qualifiers,
  RawAstDataDef*& rawAstDataDef) {

  assert(rawAstDataDef);
  assert(!rawAstDataDef->m_name.empty());
  if (!rawAstDataDef->m_objType) {
    rawAstDataDef->m_objType = new ObjTypeFunda(ObjTypeFunda::eInt);
  }

  // add to environment everything except local data objects. Currently there
  // are only local data objects, so currently there is nothing todo.

  auto decl = new AstDataDecl(
    rawAstDataDef->m_name,
    &(rawAstDataDef->m_objType->addQualifiers(qualifiers)),
    rawAstDataDef->m_storageDuration);
  return new AstDataDef(decl, rawAstDataDef->m_ctorArgs);
}

AstFunDecl* ParserExt::mkFunDecl(const string name, const ObjType* ret,
  list<AstArgDecl*>* args) {

  // create ObjTypeFun object
  args = args ? args : new list<AstArgDecl*>();
  list<AstArgDecl*>::const_iterator iterArgs = args->begin();
  list<shared_ptr<const ObjType> >* argsObjType = new list<shared_ptr<const ObjType> >;
  for (/*nop*/; iterArgs!=args->end(); ++iterArgs) {
    argsObjType->push_back( (*iterArgs)->declaredObjTypeAsSp() );
  }
  auto objTypeRet = shared_ptr<const ObjType>(ret);
  auto objTypeFun = make_shared<const ObjTypeFun>( argsObjType, objTypeRet);

  // ensure function is in environment. Note that currently there is a flat
  // namespace regarding function names; i.e. also nested functions are
  // nevertheless in the flat global namespace.
  Env::InsertRet insertRet = m_env.insert( name, NULL);
  SymbolTable::iterator& stIter = insertRet.first;
  shared_ptr<SymbolTableEntry>& stIterStEntry = stIter->second;
  bool wasAlreadyInMap = !insertRet.second;
  if (!wasAlreadyInMap) {
    stIterStEntry = make_shared<SymbolTableEntry>(move(objTypeFun),
      StorageDuration::eStatic);
  } else {
    assert(stIterStEntry.get());
    if ( !stIterStEntry->objType().matchesFully(*objTypeFun) ) {
      Error::throwError(m_errorHandler, Error::eIncompatibleRedeclaration);
    }
  }

  return new AstFunDecl(name, args, objTypeRet, stIterStEntry);
}

AstFunDecl* ParserExt::mkFunDecl(const string name, const ObjType* ret,
  AstArgDecl* arg1, AstArgDecl* arg2, AstArgDecl* arg3) {
  return mkFunDecl(name, ret, AstFunDecl::createArgs(arg1, arg2, arg3));
}

AstFunDef* ParserExt::mkFunDef(AstFunDecl* decl, AstValue* body) {
  // handling stentry.isDefined() is done later
  return new AstFunDef(decl, body);
}

AstFunDef* ParserExt::mkMainFunDef(AstValue* body) {
  return mkFunDef(
    mkFunDecl("main", new ObjTypeFunda(ObjTypeFunda::eInt)),
    body);
}
