#include "parserext.h"
#include "env.h"
#include "ast.h"
#include "errorhandler.h"
#include <cassert>
#include <stdexcept>
using namespace std;

AstDataDecl* ParserExt::mkDataDecl(ObjType::Qualifier qualifier,
  RawAstDataDecl*& rawAstDataDecl) {
  assert(rawAstDataDecl);
  assert(rawAstDataDecl->m_objType);

  // add to environment everything except local data objects. Currently there
  // are only local data objects, so currently there is nothing todo.

  AstDataDecl* astDataDecl = new AstDataDecl(
    rawAstDataDecl->m_name,
    &(rawAstDataDecl->m_objType->addQualifier(qualifier)));
  delete rawAstDataDecl;
  rawAstDataDecl = NULL;
  return astDataDecl;
}

AstDataDef* ParserExt::mkDataDef(ObjType::Qualifier qualifier,
  RawAstDataDef*& rawAstDataDef) {
  assert(rawAstDataDef);
  AstDataDef* astDataDef = new AstDataDef(
    mkDataDecl(qualifier, rawAstDataDef->m_decl),
    rawAstDataDef->m_ctorArgs);
  delete rawAstDataDef;
  rawAstDataDef = NULL;
  return astDataDef;
}

AstFunDecl* ParserExt::mkFunDecl(const string name, list<AstArgDecl*>* args) {

  // create ObjTypeFun object
  args = args ? args : new list<AstArgDecl*>();
  list<AstArgDecl*>::const_iterator iterArgs = args->begin();
  list<ObjType*>* argsObjType = new list<ObjType*>;
  for (/*nop*/; iterArgs!=args->end(); ++iterArgs) {
    argsObjType->push_back( &((*iterArgs)->objType(true)) );
  }
  ObjTypeFun* objTypeFun = new ObjTypeFun(
    argsObjType, new ObjTypeFunda(ObjTypeFunda::eInt));

  // ensure function is in environment
  Env::InsertRet insertRet = m_env.insert( name, NULL);
  SymbolTable::iterator& stIter = insertRet.first;
  SymbolTableEntry*& stIterStEntry = stIter->second;
  bool wasAlreadyInMap = !insertRet.second;
  if (!wasAlreadyInMap) {
    stIterStEntry = new SymbolTableEntry(NULL, objTypeFun);
  } else {
    assert(stIterStEntry);
    if ( ObjType::eFullMatch != stIterStEntry->objType().match(*objTypeFun) ) {
      m_errorHandler.add(new Error(Error::eIncompatibleRedaclaration));
      throw BuildError();
    }
    delete objTypeFun;
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
