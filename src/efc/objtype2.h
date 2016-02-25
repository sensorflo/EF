#pragma once
#include "envnode.h"
#include <vector>

class ObjType2;

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
    funParamList,
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

protected:
  TemplateParamType() = default;

private:
  NEITHER_COPY_NOR_MOVEABLE(TemplateParamType);
};

std::basic_ostream<char>& operator<<(std::basic_ostream<char>& os,
  TemplateParamType::MatchType mt);

std::basic_ostream<char>& operator<<(std::basic_ostream<char>& os,
  const TemplateParamType& templateParamType);

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
  virtual std::string name() const =0;

protected:
  ObjTypeTemplate() = default;
};

class BuiltinObjTypeTemplateDef: public ObjTypeTemplate, public EnvNode {
public:
  BuiltinObjTypeTemplateDef(std::string name,
    std::vector<TemplateParamType::Id> paramTypeIds = {});

  // -- overrides for Template
  const std::vector<TemplateParamType::Id>& paramTypeIds() const override {
    return m_paramTypeIds; }
  std::string name() const override { return EnvNode::name(); }

private:
  const std::vector<TemplateParamType::Id> m_paramTypeIds;
};

class ObjType2: public TemplateParamType {
public:
  enum Qualifier {
    eNoQualifier = 0,
    eMutable = 1
  };

  // -- overrides for TemplateParamType
  Id id() const override { return objType2; }
  void printTo(std::basic_ostream<char>& os) const override;
  MatchType match(const TemplateParamType& rhs) const override;
  MatchType match2(const ObjType2& other) const override;

  // -- new virtual methods
  virtual Qualifier qualifiers() const =0;
  virtual const std::vector<std::unique_ptr<const TemplateParamType>>& templateArgs() const =0;
  virtual const ObjTypeTemplate& def() const =0;

protected:
  ObjType2() = default;
};

class InstanciatedObjType: public ObjType2 {
public:

};

class AstObjTypeRef: public ObjType2 {
public:
  AstObjTypeRef(std::string name, Qualifier qualifiers = eNoQualifier,
    std::vector<const TemplateParamType*>* templateArgs = nullptr);

  /** Convenience ctor for tests. Note that in production code, def is not
  known at construction time, but is set later with setDef. */
  AstObjTypeRef(const ObjTypeTemplate& def,
    Qualifier qualifiers = eNoQualifier,
    const TemplateParamType* templateArg1 = nullptr,
    const TemplateParamType* templateArg2 = nullptr,
    const TemplateParamType* templateArg3 = nullptr);

  // -- overrides for ObjType2
  const std::vector<std::unique_ptr<const TemplateParamType>>& templateArgs() const override;
  Qualifier qualifiers() const override { return m_qualifiers; }
  const ObjTypeTemplate& def() const override;

  // -- childs of this node
  virtual const std::string& name() const { return m_name; }
  // qualifiers and templateArgs are in 'overrides for ObjType2'

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
  const ObjTypeTemplate* m_def;
};
