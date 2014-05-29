#include "parserext.h"
#include "env.h"
#include "ast.h"
#include <cassert>
#include <stdexcept>
using namespace std;

AstDataDecl* ParserExt::createAstDataDecl(ObjType::Qualifier qualifier,
  RawAstDataDecl*& rawAstDataDecl) {
  assert(rawAstDataDecl);
  assert(rawAstDataDecl->m_objType);
  AstDataDecl* astDataDecl = new AstDataDecl(
    rawAstDataDecl->m_name,
    &(rawAstDataDecl->m_objType->addQualifier(qualifier)));
  delete rawAstDataDecl;
  rawAstDataDecl = NULL;
  return astDataDecl;
}

AstDataDef* ParserExt::createAstDataDef(ObjType::Qualifier qualifier,
  RawAstDataDef*& rawAstDataDef) {
  assert(rawAstDataDef);
  AstDataDef* astDataDef = new AstDataDef(
    createAstDataDecl(qualifier, rawAstDataDef->m_decl),
    rawAstDataDef->m_initValue);
  delete rawAstDataDef;
  rawAstDataDef = NULL;
  return astDataDef;
}

pair<AstFunDecl*,SymbolTableEntry*> ParserExt::createAstFunDecl(
  const string name, list<AstArgDecl*>* args) {

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
      throw runtime_error::runtime_error("Idenifier '" + name +
        "' declared or defined again with a different type.");
    }
    delete objTypeFun;
  }

  return make_pair(
    new AstFunDecl(name, args ? args : new list<AstArgDecl*>),
    stIterStEntry);
}

pair<AstFunDecl*,SymbolTableEntry*> ParserExt::createAstFunDecl(
  const string name, AstArgDecl* arg1, AstArgDecl* arg2, AstArgDecl* arg3) {
  return createAstFunDecl( name, 
    AstFunDecl::createArgs(arg1, arg2, arg3));
}

AstFunDef* ParserExt::createAstFunDef(AstFunDecl* funDecl, AstSeq* seq,
  SymbolTableEntry& stentry) {
  assert(funDecl);
  if (stentry.isDefined()) {
    throw runtime_error::runtime_error("Function '" + funDecl->name() +
      "' is already defined.");
  }
  stentry.isDefined() = true;
  return new AstFunDef(funDecl, seq);
}
