#include "objtype2.h"
#include <array>
using namespace std;

// copy from ast.cpp
namespace {

  template<typename T>
  vector<unique_ptr<T>> toUniquePtrs(vector<T*>* src) {
    if ( !src ) {
      return {};
    }
    const unique_ptr<vector<T*>> dummy(src);
    vector<unique_ptr<T>> dst{};
    for ( const auto& element : *src ) {
      if ( element ) {
        dst.emplace_back(element);
      }
    }
    return dst;
  }

  template<typename T>
  vector<unique_ptr<T>> toUniquePtrs(T* src1 = nullptr, T* src2 = nullptr,
    T* src3 = nullptr, T* src4 = nullptr, T* src5 = nullptr) {
    return toUniquePtrs(new vector<T*>{src1, src2, src3, src4, src5});
  }

  /** Adorns a given object type with given qualifiers. */
  class QualiAdornedObjType2: public ObjType2 {
  public:
    QualiAdornedObjType2(const ObjType2& objTypeWithoutQualifiers,
      Qualifier qualifiers) :
      m_objTypeWithoutQualifiers(objTypeWithoutQualifiers),
      m_qualifiers(qualifiers) {
    }

    // -- overrides for TemplateParamType
    virtual void printTo(std::basic_ostream<char>& os) const override {
      if (m_qualifiers & eMutable) {
        os << "mut-";
      }
      m_objTypeWithoutQualifiers.printTo(os);
    }

    // -- overrides for ObjType2
    Qualifier qualifiers() const override {
      return m_qualifiers; }
    const ObjType2& canonicalObjTypeWithoutQualifiers() const override {
      return m_objTypeWithoutQualifiers;
    }
    const ObjType2& canonicalObjTypeInclQualifiers() const override {
      return *this;
    }
    const ObjType2& canonicalObjTypeWithTheseQualifiers(Qualifier qualifiers) const override {
      return m_objTypeWithoutQualifiers.canonicalObjTypeWithTheseQualifiers(qualifiers);
    }
    virtual bool hasConstructor(const ObjType2& param1ObjType) const override {
      return m_objTypeWithoutQualifiers.hasConstructor(param1ObjType);
    }

  private:
    const ObjType2& m_objTypeWithoutQualifiers;
    const Qualifier m_qualifiers;
  };

  /** Intended to be derived from. The derived class represents the
  unqualified canonical object type, while this base adds its canonical
  qualified variants. */
  class CanonicalObjTyp2 : public ObjType2 {
  public:
    CanonicalObjTyp2() {
      for ( auto qualifier = 1; qualifier<eQualifierCnt; ++qualifier ) {
        m_qualifiedVariants.emplace_back(
          make_unique<QualiAdornedObjType2>(
            *this,
            static_cast<Qualifier>(qualifier)));
      }
    }

    // -- overrides for ObjType2
    Qualifier qualifiers() const override {
      return eNoQualifier;
    }
    const ObjType2& canonicalObjTypeWithoutQualifiers() const override {
      return *this;
    }
    const ObjType2& canonicalObjTypeInclQualifiers() const override {
      return *this;
    }
    const ObjType2& canonicalObjTypeWithTheseQualifiers(Qualifier qualifiers) const override {
      return qualifiers==eNoQualifier ?
        static_cast<const ObjType2&>(*this) :
        static_cast<const ObjType2&>(*m_qualifiedVariants[qualifiers-1]);
    }

  private:
    /** Index is qualifier-1, that is the qualifier 'eNoQualifier' is _not_
    in this collection. Ptrs are guaranteed to be non-null. */
    vector<unique_ptr<QualiAdornedObjType2>> m_qualifiedVariants;
  };

  /** For ObjType(Template)Defs which have zero template arguments. That is
  the object type template defintion and its one single object type instance
  collapse into one. */
  class ZeroArgObjTypeTemplateDef: public ObjTypeTemplate, public CanonicalObjTyp2 {
  public:
    // -- overrides for Template
    const vector<TemplateParamType::Id>& paramTypeIds() const override {
      return m_paramTypeIds; }

    // -- overrides for TemplateParamType
    void printTo(std::basic_ostream<char>& os) const override {
      os << name();
    }

    // -- overrides for ObjTypeTemplate
    const ObjType2& instanceFor(const TemplateParams& templateParams) const override {
      assert(templateParams.empty());
      return *this;
    };

  private:
    static const vector<TemplateParamType::Id> m_paramTypeIds;
  };

  const vector<TemplateParamType::Id> ZeroArgObjTypeTemplateDef::m_paramTypeIds{};


  /** For all object type templates defining EF's fundamental scalar types */
  class FundaScalarDef: public ZeroArgObjTypeTemplateDef {
  public:
    FundaScalarDef(ObjTypeFunda::EType type) {
    }

    // -- overrides for ObjTypeTemplate
    const string& name() const override { return m_name; }

    // -- overrides for ObjType2
    bool hasConstructor(const ObjType2& param1ObjType) const override {
      assert(false); // not implemented yet
      return false;
    }

  private:
    //const ObjTypeFunda::EType m_type; // not used yet
    const string m_name; // todo: initialize it
  };

  FundaScalarDef intDef(ObjTypeFunda::eInt);
  FundaScalarDef doubleDef(ObjTypeFunda::eDouble);

  class PtrObjType2: public CanonicalObjTyp2 {
  public:
    PtrObjType2(const ObjType2& templateArg) :
      m_templateArg(templateArg) {}

    // -- overrides for TemplateParamType
    void printTo(std::basic_ostream<char>& os) const override {
      os << "Ptr{";
      m_templateArg.printTo(os);
      os << "}";
    }

    // -- overrides for ObjType2
    virtual bool hasConstructor(const ObjType2& param1ObjType) const override {
      assert(false); // not tested yet
      return eNoMatch != match(param1ObjType);
    }

  private:
    const ObjType2& m_templateArg;
  };

  class PtrTemplateDef: public ObjTypeTemplate {
  public:
    static const PtrTemplateDef& instance() {
      if ( !m_instance ) {
        m_instance.reset(new PtrTemplateDef());
      }
      return *m_instance;
    }

    // -- overrides for Template
    const vector<TemplateParamType::Id>& paramTypeIds() const override {
      return m_paramTypeIds; }

    // -- overrides for ObjTypeTemplate
    const string& name() const override { return m_name; }
    const ObjType2& instanceFor(const TemplateParams& templateParams) const override {
      assert(templateParams.size()==1);
      const auto& templateParam = templateParams[0];
      assert(templateParam);
      assert(templateParam->id()==TemplateParamType::objType2);
      const auto temlateParamAsObjTtype =
        dynamic_cast<const ObjType2*>(templateParam.get());
      assert(temlateParamAsObjTtype);
      const auto& canonicalObjTypeOfTemplateParam =
        temlateParamAsObjTtype->canonicalObjTypeInclQualifiers();
      const auto foundInstance = m_instances.find(&canonicalObjTypeOfTemplateParam);
      if ( foundInstance!=m_instances.end() ) {
        return *(foundInstance->second);
      }
      else {
        const auto res = m_instances.emplace(
          &canonicalObjTypeOfTemplateParam,
          make_unique<PtrObjType2>(canonicalObjTypeOfTemplateParam));
        return *(res.first->second);
      }
    };

  private:
    PtrTemplateDef() :
      m_name{"Ptr"},
      m_paramTypeIds{TemplateParamType::objType2} {
      }

    /** Key is the template argument, that is the ObjType2 of the pointee
    (incl qualifiers), and the value is the associated canonical Ptr-ObjType2. */
    mutable unordered_map<const ObjType2*, unique_ptr<PtrObjType2>> m_instances;
    const string m_name;
    const vector<TemplateParamType::Id> m_paramTypeIds;
    /** Singleton instance */
    static unique_ptr<PtrTemplateDef> m_instance;
  };

  unique_ptr<PtrTemplateDef> PtrTemplateDef::m_instance;

  vector<FundaScalarDef*> makeFundas() {
    vector<FundaScalarDef*> fundas{ObjTypeFunda::eTypeCnt};
    for ( auto&& funda: fundas ) {
      funda = nullptr;
    }
    fundas[ObjTypeFunda::eInt] = &intDef;
    fundas[ObjTypeFunda::eDouble] = &doubleDef;
    return fundas;
  }
  const auto fundaScalarTemplateDefs = makeFundas();

  const FundaScalarDef& defOf(ObjTypeFunda::EType type) {
    const auto def = fundaScalarTemplateDefs.at(type);
    assert(def);
    return *def;
  }
}

basic_ostream<char>& operator<<(basic_ostream<char>& os, TemplateParamType::MatchType mt) {
  switch (mt) {
  case ObjType2::eNoMatch: return os << "NoMatch";
  case ObjType2::eMatchButAllQualifiersAreWeaker: return os << "MatchButAllQualifiersAreWeaker";
  case ObjType2::eMatchButAnyQualifierIsStronger: return os << "MatchButAnyQualifierIsStronger";
  case ObjType2::eFullMatch: return os << "FullMatch";
  default: assert(false); return os;
  }
}

basic_ostream<char>& operator<<(basic_ostream<char>& os,
  const TemplateParamType& templateParamType) {
  templateParamType.printTo(os);
  return os;
}

ObjType2List::ObjType2List(ObjTypes2Iter begin, ObjTypes2Iter end) :
  m_begin(begin),
  m_end(end) {

}

void ObjType2List::printTo(basic_ostream<char>& os) const {
  os << "(";
  for (auto objType = m_begin; objType!=m_end; ++objType) {
    if ( objType!=m_begin ) {
        os << ", ";
    }
    os << **objType;
  }
  os << ")";
}

TemplateParamType::MatchType ObjType2List::match(const TemplateParamType& rhs) const {
  return rhs.match2(*this);
}

TemplateParamType::MatchType ObjType2List::match2(const ObjType2List& lhs) const {
  auto lhsObjType = lhs.m_begin;
  auto objType = m_begin;
  for ( /*nop*/;
       objType!=m_end && lhsObjType!=lhs.m_end;
       ++objType, ++lhsObjType) {
    if ( eFullMatch!=(*lhsObjType)->match(**objType) ) {
      return eNoMatch;
    }
  }
  return objType==m_end && lhsObjType==lhs.m_end ? eFullMatch : eNoMatch;
}

TemplateParamType::MatchType ObjType2::match(const TemplateParamType& lhs) const {
  return lhs.match2(*this);
}

TemplateParamType::MatchType ObjType2::match2(const ObjType2& rhs) const {
  if (&canonicalObjTypeWithoutQualifiers() !=
    &rhs.canonicalObjTypeWithoutQualifiers()) {
    return eNoMatch;
  }
  const auto lhsQualifiers = canonicalObjTypeInclQualifiers().qualifiers();
  const auto rhsQualifiers = rhs.canonicalObjTypeInclQualifiers().qualifiers();
  if (!(lhsQualifiers&eMutable) && (rhsQualifiers&eMutable)) {
    return eMatchButAnyQualifierIsStronger;
  }
  else if ((lhsQualifiers&eMutable) && !(rhsQualifiers&eMutable)) {
    return eMatchButAllQualifiersAreWeaker;
  } else {
    return eFullMatch;
  }
}

const ObjType2& ObjType2::canonicalObjTypeWithTheseQualifiers(Qualifier qualifiers) const {
  return canonicalObjTypeWithoutQualifiers().
    canonicalObjTypeWithTheseQualifiers(qualifiers);
}

AstObjTypeRef::AstObjTypeRef(string name, Qualifier qualifiers,
  TemplateParamsRaw* templateArgs) :
  m_name{move(name)},
  m_qualifiers{qualifiers},
  m_templateArgs{toUniquePtrs(templateArgs)},
  m_def{},
  m_canonicalObjType{},
  m_canonicalObjTypeWithMyQualifiers{} {
}

AstObjTypeRef::AstObjTypeRef(ObjTypeFunda::EType type, Qualifier qualifiers,
  TemplateParamsRaw* templateArgs) :
  AstObjTypeRef{defOf(type), qualifiers, templateArgs} {
}

AstObjTypeRef::AstObjTypeRef(const ObjTypeTemplate& def,
  Qualifier qualifiers,
  TemplateParamsRaw* templateArgs) :
  AstObjTypeRef{def.name(), qualifiers, templateArgs} {
  m_def = &def;
}

AstObjTypeRef::AstObjTypeRef(const ObjTypeTemplate& def,
  Qualifier qualifiers,
  const TemplateParamType* templateArg1,
  const TemplateParamType* templateArg2,
  const TemplateParamType* templateArg3) :
  AstObjTypeRef{def.name(), qualifiers,
    new vector<const TemplateParamType*>{templateArg1, templateArg2, templateArg3}} {
  m_def = &def;
}

void AstObjTypeRef::printTo(std::basic_ostream<char>& os) const {
  canonicalObjTypeInclQualifiers().printTo(os);
}

void AstObjTypeRef::setDef(const ObjTypeTemplate& def) {
  assert(m_name==def.name());

  assert(!m_def); // it doesn't make sense to set it twice
  m_def = &def;

  // test that the template arguments match the required template param types
  // required by the object type template definition.
  // should be part of semantic analyzis, where a proper error is reported
  const auto& paramIds = def.paramTypeIds();
  assert(m_templateArgs.size() == paramIds.size());
  auto arg = m_templateArgs.begin();
  for ( const auto& paramId : paramIds ) {
    assert((*arg)->id() == paramId);
    ++arg;
  }
}

const ObjType2& AstObjTypeRef::canonicalObjTypeWithoutQualifiers() const {
  if ( !m_canonicalObjType ) {
    assert(m_def);
    m_canonicalObjType = &m_def->instanceFor(m_templateArgs);
  }
  return *m_canonicalObjType;
}

const ObjType2& AstObjTypeRef::canonicalObjTypeInclQualifiers() const {
  if ( !m_canonicalObjTypeWithMyQualifiers ) {
    assert(m_def);
    m_canonicalObjTypeWithMyQualifiers = &m_def->instanceFor(m_templateArgs).
      canonicalObjTypeWithTheseQualifiers(m_qualifiers);
  }
  return *m_canonicalObjTypeWithMyQualifiers;
}
