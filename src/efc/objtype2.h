#pragma once
#include "envnode.h"
#include <vector>

class ObjType2;
class ObjType2List;

/** This is an own kind of type. As ObjType is an own kind of type. That is
it's _not_ about the ObjType of a template parameter. */
class TemplateParamType {
public:
  /** The list is known at compile time. This is ok, since the parser, given a
  reference to a template instance, needs to know the TemplateArg::Type of
  each template argument anyway. */
  enum Id {
    /** see class ObjType2 */
    objType2,
    /** A list of objType2 */
    objType2List,
    /** An Object which is biuniquely mappable to an integral */
    integralObj
  };
  enum MatchType {
    eNoMatch,
    eMatchButAllQualifiersAreWeaker,
    eMatchButAnyQualifierIsStronger,
    eFullMatch
  };

  virtual ~TemplateParamType() = default;

  // -- new virtual methods
  virtual Id id() const =0;
  virtual void printTo(std::basic_ostream<char>& os) const =0;
  virtual MatchType match(const TemplateParamType& lhs) const =0;
  virtual MatchType match2(const ObjType2& rhs) const { return eNoMatch; }
  virtual MatchType match2(const ObjType2List& rhs) const { return eNoMatch; }

protected:
  TemplateParamType() = default;

private:
  NEITHER_COPY_NOR_MOVEABLE(TemplateParamType);
};

std::basic_ostream<char>& operator<<(std::basic_ostream<char>& os,
  TemplateParamType::MatchType mt);

std::basic_ostream<char>& operator<<(std::basic_ostream<char>& os,
  const TemplateParamType& templateParamType);

using TemplateParams = std::vector<std::unique_ptr<const TemplateParamType>>;

class Template {
public:
  virtual ~Template() = default;
  virtual const std::vector<TemplateParamType::Id>& paramTypeIds() const =0;

protected:
  Template() = default;

private:
  NEITHER_COPY_NOR_MOVEABLE(Template);
};

class ObjTemplate: public Template {
};

class ObjTypeTemplate: public Template {
public:
  // -- new virtual methods
  virtual const std::string& name() const =0;
  virtual const ObjType2& instanceFor(const TemplateParams&) const =0;

protected:
  ObjTypeTemplate() = default;
};

class ObjType2List: public TemplateParamType {
public:
  using ObjTypes2Iter = std::vector<std::unique_ptr<ObjType2>>::iterator;

  ObjType2List(ObjTypes2Iter begin, ObjTypes2Iter end);

  // -- overrides for TemplateParamType
  Id id() const override { return objType2List; }
  void printTo(std::basic_ostream<char>& os) const override;
  MatchType match(const TemplateParamType& rhs) const override;
  MatchType match2(const ObjType2List& other) const override;

private:
  const ObjTypes2Iter m_begin;
  const ObjTypes2Iter m_end;
};

class ObjType2: public TemplateParamType {
public:
  enum Qualifier {
    eNoQualifier = 0,
    eMutable = 1,
    eQualifierCnt
  };

  // -- overrides for TemplateParamType
  Id id() const override { return objType2; }
  MatchType match(const TemplateParamType& rhs) const override;
  MatchType match2(const ObjType2& other) const override;

  // -- new virtual methods
  virtual Qualifier qualifiers() const =0;
  // TODO: depending on the ObjType, it is the template wich instanciated me
  // or the template I am refering to.
  virtual const ObjType2& canonicalObjTypeWithoutQualifiers() const =0;
  virtual const ObjType2& canonicalObjTypeInclQualifiers() const =0;
  virtual const ObjType2& canonicalObjTypeWithTheseQualifiers(Qualifier qualifiers) const =0;
  // intermediate solution until ObjTypeTemplate / ObjType2 properly can have
  // member functions and ctors.
  // match vs hasconstructor. match:
  // 1)? is exactly the same type. Why should I want to know that, what really
  // matters is the following, no?
  // 2)? match is a trivial copy ctor. I.e. no casts needed. &T1 can be
  // initialized with T2.
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
