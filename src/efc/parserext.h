#pragma once
/** \file
yy::Parser implementation extension. Has two related responsibilities:

1) So bison's input file ef.yy can only contain tiny fragments of
   code. Anything larger than tiny is implemented here and ef.yy should make
   only the call.  ParserApiExt is similar, but extends the public interface
   of yy::Parser.

2) So SemanticAnalizer can make less traversals of the AST, ParserExt does
   some of SemanticAnalizer's responsibilities already while creating the
   AST. Currently the design is that it is intentionally left open who is
   responsible for what, so that responsibilities can easily be moved between
   the two.*/

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
  RawAstDataDef(ErrorHandler& errorHandler);
  RawAstDataDef(ErrorHandler& errorHandler, const std::string& name,
    AstCtList* ctorArgs, ObjType* objType, StorageDuration storageDuration);

  void setName(const std::string& name);
  void setCtorArgs(AstCtList* ctorArgs);
  void setObjType(ObjType* objType);
  void setStorageDuration(StorageDuration storageDuration);

private:
  friend ParserExt;
  ErrorHandler& m_errorHandler;
  std::string m_name;
  AstCtList* m_ctorArgs;
  ObjType* m_objType;
  bool m_isStorageDurationDefined;
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

  AstFunDef* mkFunDef(const std::string name, std::list<AstDataDef*>* args,
    const ObjType* ret, AstValue* body);
  AstFunDef* mkFunDef(const std::string name, const ObjType* ret,
    AstValue* body);

  AstFunDef* mkMainFunDef(AstValue* body);

  ErrorHandler& errorHandler() { return m_errorHandler; }

private:
  Env& m_env;
  ErrorHandler& m_errorHandler;
};
