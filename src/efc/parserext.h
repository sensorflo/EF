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
#include "declutils.h"
#include "objtype.h"
#include "storageduration.h"
#include <list>

#include "astforwards.h"
class Env;
class ErrorHandler;
class ParserExt;

class RawAstDataDef final {
public:
  RawAstDataDef(ErrorHandler& errorHandler);
  RawAstDataDef(ErrorHandler& errorHandler, const std::string& name,
    AstCtList* ctorArgs, std::shared_ptr<const ObjType> objType,
    StorageDuration storageDuration);
  ~RawAstDataDef() = default;

  void setName(const std::string& name);
  void setCtorArgs(AstCtList* ctorArgs);
  void setObjType(std::shared_ptr<const ObjType> objType);
  void setStorageDuration(StorageDuration storageDuration);

private:
  friend ParserExt;

  NEITHER_COPY_NOR_MOVEABLE(RawAstDataDef);

  ErrorHandler& m_errorHandler;
  std::string m_name;
  AstCtList* m_ctorArgs;
  std::shared_ptr<const ObjType> m_objType;
  bool m_isStorageDurationDefined;
  StorageDuration m_storageDuration;
};

/** See file's decription */
class ParserExt final {
public:
  ParserExt(Env& env, ErrorHandler& errorHandler);
  ~ParserExt() = default;

  AstOperator* mkOperatorTree(const std::string& op, AstCtList* args);
  AstOperator* mkOperatorTree(const std::string& op, AstObject* child1,
    AstObject* child2, AstObject* child3 = NULL, AstObject* child4 = NULL,
    AstObject* child5 = NULL, AstObject* child6 = NULL);

  AstDataDef* mkDataDef(ObjType::Qualifiers qualifiers,
    RawAstDataDef*& rawAstDataDef);

  AstFunDef* mkFunDef(const std::string name, std::list<AstDataDef*>* astArgs,
    std::shared_ptr<const ObjType> retObjType, AstObject* astBody);
  AstFunDef* mkFunDef(const std::string name, ObjTypeFunda::EType ret,
    AstObject* body);
  AstFunDef* mkFunDef(const std::string name, std::shared_ptr<const ObjType> ret,
    AstObject* body);

  AstFunDef* mkMainFunDef(AstObject* body);

  AstClassDef* mkClassDef(const std::string name,
    std::vector<AstDataDef*>* astDataMembers);

  ErrorHandler& errorHandler() { return m_errorHandler; }

private:
  NEITHER_COPY_NOR_MOVEABLE(ParserExt);

  Env& m_env;
  ErrorHandler& m_errorHandler;
};
