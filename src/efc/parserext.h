#pragma once
/** \file
yy::Parser implementation extension. So bison's input file ef.yy can only contain
tiny fragments of code. Anything larger than tiny is implemented here and
ef.yy should make only the call.

ParserApiExt is similar, but extends the public interface of yy::Parser. */

#include "objtype.h"
#include "storageduration.h"
#include <list>

#include "astforwards.h"
class SymbolTableEntry;
class Env;
class ErrorHandler;
class ParserExt;

class RawAstDataDef {
public:
  RawAstDataDef();
  RawAstDataDef(const std::string& name, AstCtList* ctorArgs,
    ObjType* objType, StorageDuration storageDuration);

  void setName(const std::string& name);
  void setCtorArgs(AstCtList* ctorArgs);
  void setObjType(ObjType* objType);
  void setStorageDuration(StorageDuration storageDuration);

private:
  friend ParserExt;
  std::string m_name;
  AstCtList* m_ctorArgs;
  ObjType* m_objType;
  StorageDuration m_storageDuration;
};  

/** See file's decription */
class ParserExt {
public:
  ParserExt(Env& env, ErrorHandler& errorHandler);

  AstOperator* mkOperatorTree(const std::string& op, AstCtList* args);
  AstOperator* mkOperatorTree(const std::string& op, AstValue* child1,
    AstValue* child2, AstValue* child3 = NULL, AstValue* child4 = NULL,
    AstValue* child5 = NULL, AstValue* child6 = NULL);

  AstDataDef* mkDataDef(ObjType::Qualifiers qualifiers,
    RawAstDataDef*& rawAstDataDef);

  AstFunDecl* mkFunDecl(const std::string name, const ObjType* ret,
    std::list<AstArgDecl*>* args = NULL);
  AstFunDecl* mkFunDecl(const std::string name, const ObjType* ret,
    AstArgDecl* arg1, AstArgDecl* arg2 = NULL, AstArgDecl* arg3 = NULL);
  AstFunDef* mkFunDef(AstFunDecl* decl, AstValue* body);

  AstFunDef* mkMainFunDef(AstValue* body);

  ErrorHandler& errorHandler() { return m_errorHandler; }

private:
  Env& m_env;
  ErrorHandler& m_errorHandler;
};
