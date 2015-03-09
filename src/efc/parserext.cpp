#include "parserext.h"
#include "env.h"
#include "ast.h"
#include "errorhandler.h"
#include <cassert>
#include <stdexcept>
#include "memoryext.h"
using namespace std;

AstDataDecl* ParserExt::mkDataDecl(ObjType::Qualifiers qualifiers,
  RawAstDataDecl*& rawAstDataDecl) {
  assert(rawAstDataDecl);
  assert(rawAstDataDecl->m_objType);

  // add to environment everything except local data objects. Currently there
  // are only local data objects, so currently there is nothing todo.

  AstDataDecl* astDataDecl = new AstDataDecl(
    rawAstDataDecl->m_name,
    &(rawAstDataDecl->m_objType->addQualifiers(qualifiers)));
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

AstFunDecl* ParserExt::mkFunDecl(const string name, list<AstArgDecl*>* args) {

  // create ObjTypeFun object
  args = args ? args : new list<AstArgDecl*>();
  list<AstArgDecl*>::const_iterator iterArgs = args->begin();
  list<shared_ptr<const ObjType> >* argsObjType = new list<shared_ptr<const ObjType> >;
  for (/*nop*/; iterArgs!=args->end(); ++iterArgs) {
    argsObjType->push_back( (*iterArgs)->objTypeShareOwnership() );
  }
  auto objTypeFun = make_shared<const ObjTypeFun>(
    argsObjType, new ObjTypeFunda(ObjTypeFunda::eInt));

  // ensure function is in environment
  Env::InsertRet insertRet = m_env.insert( name, NULL);
  SymbolTable::iterator& stIter = insertRet.first;
  SymbolTableEntry*& stIterStEntry = stIter->second;
  bool wasAlreadyInMap = !insertRet.second;
  if (!wasAlreadyInMap) {
    stIterStEntry = new SymbolTableEntry(move(objTypeFun));
  } else {
    assert(stIterStEntry);
    if ( !stIterStEntry->objType().matchesFully(*objTypeFun) ) {
      m_errorHandler.add(new Error(Error::eIncompatibleRedaclaration));
      throw BuildError();
    }
  }

  return new AstFunDecl(name, args ? args : new list<AstArgDecl*>, stIterStEntry);
}

AstFunDecl* ParserExt::mkFunDecl(const string name, AstArgDecl* arg1,
  AstArgDecl* arg2, AstArgDecl* arg3) {
  return mkFunDecl(name, AstFunDecl::createArgs(arg1, arg2, arg3));
}

AstFunDef* ParserExt::mkFunDef(AstFunDecl* decl, AstValue* body) {
  // handling stentry.isDefined() is done later
  return new AstFunDef(decl, body);
}
