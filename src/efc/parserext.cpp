#include "parserext.h"
#include "env.h"
#include "ast.h"
#include "errorhandler.h"
#include <cassert>
#include <stdexcept>
using namespace std;

/** Turns the AstCtList in an AstOperator tree with at most two childs per
node.  The AstCtList object is deleted, its ex-childs are now owned by their
respective AstOperator parent. */
AstOperator* ParserExt::mkOperatorTree(const string& op_as_str, AstCtList* args) {
  assert(args);
  args->releaseOwnership();
  const AstOperator::EOperation op = AstOperator::toEOperation(op_as_str);
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

AstDataDecl* ParserExt::mkDataDecl(ObjType::Qualifiers qualifiers,
  RawAstDataDecl*& rawAstDataDecl) {
  assert(rawAstDataDecl);
  assert(rawAstDataDecl->m_objType);

  // add to environment everything except local data objects. Currently there
  // are only local data objects, so currently there is nothing todo.

  AstDataDecl* astDataDecl = new AstDataDecl(
    rawAstDataDecl->m_name,
    &(rawAstDataDecl->m_objType->addQualifiers(qualifiers)),
    StorageDuration::eLocal);
  delete rawAstDataDecl;
  rawAstDataDecl = NULL;
  return astDataDecl;
}

AstDataDef* ParserExt::mkDataDef(ObjType::Qualifiers qualifiers,
  RawAstDataDef*& rawAstDataDef) {
  assert(rawAstDataDef);
  AstDataDef* astDataDef = new AstDataDef(
    mkDataDecl(qualifiers, rawAstDataDef->m_decl),
    rawAstDataDef->m_ctorArgs);
  delete rawAstDataDef;
  rawAstDataDef = NULL;
  return astDataDef;
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
