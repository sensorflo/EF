#ifndef PARSEREXT_H
#define PARSEREXT_H

/** \file
Parser extension: So bison's input file ef_yy can only contain tiny fragments
of code. */

#include "objtype.h"
#include <list>

class SymbolTableEntry;
class Env;
class AstValue;
class AstSeq;
class AstDataDecl;
class AstDataDef;
class AstArgDecl;
class AstFunDecl;
class AstFunDef;

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
  ParserExt(Env& env) : m_env(env) {}

  AstDataDecl* createAstDataDecl(ObjType::Qualifier qualifier,
    RawAstDataDecl*& rawAstDataDecl);
  AstDataDef* createAstDataDef(ObjType::Qualifier qualifier,
    RawAstDataDef*& rawAstDataDef);

  std::pair<AstFunDecl*,SymbolTableEntry*> createAstFunDecl(
    const std::string name, std::list<AstArgDecl*>* args = NULL);
  std::pair<AstFunDecl*,SymbolTableEntry*> createAstFunDecl(
    const std::string name, AstArgDecl* arg1, AstArgDecl* arg2 = NULL,
    AstArgDecl* arg3 = NULL);
  AstFunDef* createAstFunDef(AstFunDecl* funDecl, AstSeq* seq,
    SymbolTableEntry& stentry);

private:
  Env& m_env;
};

#endif
