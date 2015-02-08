#include "parserext.h"
#include "env.h"
#include "ast.h"
#include <cassert>
#include <stdexcept>
using namespace std;

AstDataDecl* ParserExt::mkDataDecl(ObjType::Qualifier qualifier,
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

pair<AstFunDecl*,SymbolTableEntry*> ParserExt::mkFunDecl(
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

pair<AstFunDecl*,SymbolTableEntry*> ParserExt::mkFunDecl(
  const string name, AstArgDecl* arg1, AstArgDecl* arg2, AstArgDecl* arg3) {
  return mkFunDecl( name, 
    AstFunDecl::createArgs(arg1, arg2, arg3));
}

AstFunDef* ParserExt::mkFunDef(std::pair<AstFunDecl*,SymbolTableEntry*> decl_stentry,
  AstValue* body) {
  assert(decl_stentry.second);
  if (decl_stentry.second->isDefined()) {
    throw runtime_error::runtime_error("Function '" + decl_stentry.first->name() +
      "' is already defined.");
  }
  decl_stentry.second->isDefined() = true;
  return new AstFunDef(decl_stentry.first, body);
}
