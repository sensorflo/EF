#pragma once
/** \file
GenParser implementation extension. Has two related responsibilities:

1) So Bison's input file genparser.yy can only contain tiny fragments of
   code. Anything larger than tiny is implemented here and genparser.yy should
   make only the call.

2) So SemanticAnalizer can make less traversals of the AST, GenParserExt does
   some of SemanticAnalizer's responsibilities already while creating the
   AST. Currently the design is that it is intentionally left open who is
   responsible for what, so that responsibilities can easily be moved between
   the two until the design is more stable.*/
#include "astforwards.h"
#include "declutils.h"
#include "location.h"
#include "objtype.h"
#include "storageduration.h"

#include <list>

class Env;
class ErrorHandler;
class GenParserExt;

class RawAstDataDef final {
public:
  RawAstDataDef(ErrorHandler& errorHandler, std::string name,
    AstCtList* ctorArgs1, AstCtList* ctorArgs2, AstObjType* astObjType,
    StorageDuration storageDuration, const Location& loc);
  ~RawAstDataDef() = default;

private:
  friend GenParserExt;

  NEITHER_COPY_NOR_MOVEABLE(RawAstDataDef);

  ErrorHandler& m_errorHandler;
  std::string m_name;
  AstCtList* m_ctorArgs;
  AstObjType* m_astObjType;
  StorageDuration m_storageDuration;
};

/** See file's decription */
class GenParserExt final {
public:
  GenParserExt(Env& env, ErrorHandler& errorHandler);
  ~GenParserExt() = default;

  AstObjType* mkDefaultType(Location loc = s_nullLoc);
  StorageDuration mkDefaultStorageDuration();
  ObjType::Qualifiers mkDefaultObjectTypeQualifier();

  AstOperator* mkOperatorTree(
    const std::string& op, AstCtList* args, Location loc = s_nullLoc);
  AstOperator* mkOperatorTree(const std::string& op, AstObject* child1,
    AstObject* child2, AstObject* child3 = nullptr, AstObject* child4 = nullptr,
    AstObject* child5 = nullptr, AstObject* child6 = nullptr);

  AstDataDef* mkDataDef(ObjType::Qualifiers qualifiers,
    RawAstDataDef*& rawAstDataDef, Location loc = s_nullLoc);

  AstFunDef* mkFunDef(const std::string name, std::vector<AstDataDef*>* astArgs,
    AstObjType* retAstObjType, AstObject* astBody, Location loc = s_nullLoc);
  AstFunDef* mkFunDef(const std::string name, ObjTypeFunda::EType ret,
    AstObject* body, Location loc = s_nullLoc);
  AstFunDef* mkFunDef(const std::string name, AstObjType* ret, AstObject* body,
    Location loc = s_nullLoc);

  AstFunDef* mkMainFunDef(AstObject* body);

  ErrorHandler& errorHandler() { return m_errorHandler; }

private:
  NEITHER_COPY_NOR_MOVEABLE(GenParserExt);

  ErrorHandler& m_errorHandler;
};
