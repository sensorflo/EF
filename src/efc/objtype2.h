#pragma once
#include "templateparamtype.h"

class ObjTypeTemplate;

class ObjType2: public TemplateParamType {
public:
  enum Qualifier {
    eNoQualifier = 0,
    eMutable = 1,
    eQualifierCnt
  };

  // -- overrides for TemplateParamType
  Id id() const override { return objType2; }

  // match = is same (canonical) type, potentially different
  // qualifiers. E.g. whether pointer can point to it.
  // ??? pointers with these special qualifiers rules???
  // later used ofr overload resoltion: which candidate is a better match for
  // a given ObjType2
  MatchType match(const TemplateParamType& rhs) const override;
  MatchType match2(const ObjType2& other) const override;

  // -- new virtual methods
  virtual Qualifier qualifiers() const =0;
  // TODO: depending on the ObjType, it is the template wich instanciated me
  // or the template I am refering to.
  virtual const ObjType2& canonicalObjTypeWithoutQualifiers() const =0;
  virtual const ObjType2& canonicalObjTypeInclQualifiers() const =0;
  virtual const ObjType2& canonicalObjTypeWithTheseQualifiers(Qualifier qualifiers) const;
  // intermediate solution until ObjTypeTemplate / ObjType2 properly can have
  // member functions and ctors.
  // match vs hasconstructor. match:
  // 1)? is exactly the same type. Why should I want to know that, what really
  // matters is the following, no?
  // 2)? match is a trivial copy ctor. I.e. no casts needed. &T1 can be
  // initialized with T2.
  // currently wrongly only used to check whether cast is eligible. It should
  // e.g. also be used to check if initializer is eligible, but it currently
  // isn't. Or from another view, all constructors are currently explicit and
  // can be 'called' with an explicit cast only.
  virtual bool hasConstructor(const ObjType2& param1ObjType) const =0;

protected:
  ObjType2() = default;
};

/** Refers to an specific ObjType2 via the name of an ObjTypeTemplate, a list
of template parameters and added qualifiers. */
class AstObjTypeRef: public ObjType2 {
public:
  using TemplateParamsRaw = std::vector<const TemplateParamType*>;
  AstObjTypeRef(std::string name, Qualifier qualifiers = eNoQualifier,
    TemplateParamsRaw* templateArgs = nullptr);
  AstObjTypeRef(ObjTypeFunda::EType type, Qualifier qualifiers = eNoQualifier,
    TemplateParamsRaw* templateArgs = nullptr);
  AstObjTypeRef(ObjTypeFunda::EType type, Qualifier qualifiers,
    const TemplateParamType* templateArg1,
    const TemplateParamType* templateArg2 = nullptr,
    const TemplateParamType* templateArg3 = nullptr);
  /** Convenience ctor for tests. Note that in production code, def is not
  known at construction time, but is set later with setDef. */
  AstObjTypeRef(const ObjTypeTemplate& def,
    Qualifier qualifiers = eNoQualifier,
    TemplateParamsRaw* templateArgs = nullptr);
  AstObjTypeRef(const ObjTypeTemplate& def,
    Qualifier qualifiers = eNoQualifier,
    const TemplateParamType* templateArg1 = nullptr,
    const TemplateParamType* templateArg2 = nullptr,
    const TemplateParamType* templateArg3 = nullptr);

  // -- overrides for TemplateParamType
  void printTo(std::basic_ostream<char>& os) const override;

  // -- overrides for ObjType2
  Qualifier qualifiers() const override { return m_qualifiers; }
  const ObjType2& canonicalObjTypeWithoutQualifiers() const override;
  const ObjType2& canonicalObjTypeInclQualifiers() const override;
  virtual bool hasConstructor(const ObjType2& param1ObjType) const override;

  // -- childs of this node
  virtual const std::string& name() const { return m_name; }
  // qualifiers is 'overrides for ObjType2'

  // -- misc
  /** May not be virtual since its called from ctor. */
  void setDef(const ObjTypeTemplate& def);

private:
  // -- childs of this node
  const std::string m_name;
  const Qualifier m_qualifiers;
  /** Pointees are guaranteed to be non-null */
  const std::vector<std::unique_ptr<const TemplateParamType>> m_templateArgs;

  // -- misc
  /** We're not the owner */
  ObjTypeTemplate const * m_def;
  mutable ObjType2 const * m_canonicalObjType;
  mutable ObjType2 const * m_canonicalObjTypeWithMyQualifiers;
};
