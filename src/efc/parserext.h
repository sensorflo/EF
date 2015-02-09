#ifndef PARSEREXT_H
#define PARSEREXT_H

/** \file
Parser extension: So bison's input file ef_yy can only contain tiny fragments
of code. */

#include "objtype.h"
#include <list>

#include "astforwards.h"
class SymbolTableEntry;
class Env;
class ErrorHandler;

struct RawAstDataDecl {
  RawAstDataDecl(const std::string& name, ObjType* objType) :
    m_name(name), m_objType(objType) {};
  std::string m_name;
  ObjType* m_objType;
};  

struct RawAstDataDef {
  RawAstDataDef(RawAstDataDecl* decl, AstCtList* ctorArgs = NULL) :
    m_decl(decl), m_ctorArgs(ctorArgs) {};
  RawAstDataDecl* m_decl;
  AstCtList* m_ctorArgs;
};  

class ParserExt {
public:
  ParserExt(Env& env, ErrorHandler& errorHandler) : m_env(env),
    m_errorHandler(errorHandler) {}

  AstDataDecl* mkDataDecl(ObjType::Qualifier qualifier,
    RawAstDataDecl*& rawAstDataDecl);
  AstDataDef* mkDataDef(ObjType::Qualifier qualifier,
    RawAstDataDef*& rawAstDataDef);

  AstFunDecl* mkFunDecl(const std::string name, std::list<AstArgDecl*>* args = NULL);
  AstFunDecl* mkFunDecl(const std::string name, AstArgDecl* arg1,
    AstArgDecl* arg2 = NULL, AstArgDecl* arg3 = NULL);
  AstFunDef* mkFunDef(AstFunDecl* decl, AstValue* body);
  ErrorHandler& errorHandler() { return m_errorHandler; }

private:
  Env& m_env;
  ErrorHandler& m_errorHandler;
};

#endif
