#include "parserext.h"
#include <cassert>

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

