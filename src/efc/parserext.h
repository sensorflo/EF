#ifndef PARSEREXT_H
#define PARSEREXT_H

/** \file
Parser extension: So bison's input file ef_yy can only contain tiny fragments
of code. */

#include "ast.h"
#include "objtype.h"
#include "env.h"

struct RawAstDataDecl {
  RawAstDataDecl(const std::string& name, ObjType* objType) :
    m_name(name), m_objType(objType) {};
  std::string m_name;
  ObjType* m_objType;
};  

struct RawAstDataDef {
  RawAstDataDef(RawAstDataDecl* decl, AstValue* initValue = NULL) :
    m_decl(decl), m_initValue(initValue) {};
  RawAstDataDecl* m_decl;
  AstValue* m_initValue;
};  

class ParserExt {
public:
  ParserExt(Env& /*env*/) /*: m_env(env)*/ {}

  AstDataDecl* createAstDataDecl(ObjType::Qualifier qualifier,
    RawAstDataDecl*& rawAstDataDecl);
  AstDataDef* createAstDataDef(ObjType::Qualifier qualifier,
    RawAstDataDef*& rawAstDataDef);

private:
  //Env& m_env;
};

#endif
