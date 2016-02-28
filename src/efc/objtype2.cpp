#include "objtype2.h"
#include "template.h"
#include <array>
using namespace std;

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

  class PtrObjType;

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
  class CanonicalObjType : public ObjType2 {
  public:
    CanonicalObjType() {
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
  collapse into one entity. */
  class ZeroArgObjTypeTemplateDef: public ObjTypeTemplate, public CanonicalObjType {
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

  /** For all object type templates defining EF's fundamental types */
  class FundaDef: public ZeroArgObjTypeTemplateDef {
  public:
    FundaDef(ObjTypeFunda::EType type) :
      m_type{type},
      m_name{AstObjTypeSymbol::toName(type)} {
    }

    // -- overrides for ObjTypeTemplate
    const string& name() const override { return m_name; }

    // -- overrides for ObjType2
    bool hasConstructor(const ObjType2& param1ObjType) const override;

    // -- misc
    ObjTypeFunda::EType type() const {
      return m_type;
    }

  private:
    const ObjTypeFunda::EType m_type;
    const string& m_name;
  };

  /** Builtin raw pointer object type. */
  class PtrObjType: public CanonicalObjType {
  public:
    PtrObjType(const ObjType2& templateArg) :
      m_templateArg(templateArg) {}

    // -- overrides for TemplateParamType
    void printTo(std::basic_ostream<char>& os) const override {
      os << "raw*";
      m_templateArg.printTo(os);
    }

    // -- overrides for ObjType2
    bool hasConstructor(const ObjType2& param1ObjType) const override;

  private:
    const ObjType2& m_templateArg;
  };

  /** Builtin raw pointer object type template */
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
          make_unique<PtrObjType>(canonicalObjTypeOfTemplateParam));
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
    mutable unordered_map<const ObjType2*, unique_ptr<PtrObjType>> m_instances;
    const string m_name;
    const vector<TemplateParamType::Id> m_paramTypeIds;
    /** Singleton instance */
    static unique_ptr<PtrTemplateDef> m_instance;
  };

  unique_ptr<PtrTemplateDef> PtrTemplateDef::m_instance;

  vector<unique_ptr<FundaDef>> makeFundas() {
    vector<unique_ptr<FundaDef>> fundas{ObjTypeFunda::eTypeCnt};
    for ( auto type = 0; type<ObjTypeFunda::eTypeCnt; ++type ) {
      if ( type==ObjTypeFunda::ePointer ) {
        continue;
      }
      fundas[type] = make_unique<FundaDef>(
        static_cast<ObjTypeFunda::EType>(type));
    }
    return fundas;
  }
  const auto fundaScalarTemplateDefs = makeFundas();

  const ObjTypeTemplate& ObjTypeTemplateDefOf(ObjTypeFunda::EType type) {
    if ( type==ObjTypeFunda::ePointer ) {
      return PtrTemplateDef::instance();
    } else {
      const auto& def = fundaScalarTemplateDefs.at(type);
      assert(def);
      return *def;
    }
  }

  bool FundaDef::hasConstructor(const ObjType2& param1ObjType) const {
    const auto& param1Canonical = param1ObjType.canonicalObjTypeInclQualifiers();

    const auto& typeidOfParam1 = typeid(param1Canonical);
    if (typeid(FundaDef) == typeidOfParam1) {
      const auto& param1 = static_cast<const FundaDef&>(param1Canonical);
      switch (m_type) {
      case ObjTypeFunda::eVoid: // fall through
      case ObjTypeFunda::eNoreturn: return false;

      case ObjTypeFunda::eBool: // fall through
      case ObjTypeFunda::eChar: // fall through
      case ObjTypeFunda::eDouble: // fall through
      case ObjTypeFunda::eInt:
        if ( (param1.m_type == ObjTypeFunda::eVoid) ||
          (param1.m_type == ObjTypeFunda::eNoreturn) ) {
          return false;
        } else {
          return param1.m_type != ObjTypeFunda::eNullptr;
        }

      case ObjTypeFunda::eNullptr:
        return param1.m_type == ObjTypeFunda::eNullptr;

      case ObjTypeFunda::ePointer:
      case ObjTypeFunda::eTypeCnt:
        assert(false);
      }
      return false;
    }

    else if (typeid(PtrObjType) == typeidOfParam1) {
      switch (m_type) {
      case ObjTypeFunda::eVoid:
      case ObjTypeFunda::eNoreturn:
      case ObjTypeFunda::eNullptr: return false;
      default: return true;
      }
    }

    return false;
  }

  bool PtrObjType::hasConstructor(const ObjType2& param1ObjType) const {
    const auto& param1Canonical = param1ObjType.canonicalObjTypeInclQualifiers();

    const auto& typeidOfParam1 = typeid(param1Canonical);
    if (typeid(PtrObjType) == typeidOfParam1) {
      return eNoMatch!=match(param1Canonical);
    }

    else if (typeid(FundaDef) == typeidOfParam1) {
      const auto param1 = static_cast<const FundaDef*>(&param1Canonical);
      assert(param1);
      switch (param1->type()) {
      case ObjTypeFunda::eVoid: return false;
      case ObjTypeFunda::eNoreturn: return false;
      case ObjTypeFunda::eBool: return true;
      case ObjTypeFunda::eChar: return true;
      case ObjTypeFunda::eInt: return true;
      case ObjTypeFunda::eDouble: return true;
      default: assert(false);
      }
    }

    return false;
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
  AstObjTypeRef{ObjTypeTemplateDefOf(type), qualifiers, templateArgs} {
}

AstObjTypeRef::AstObjTypeRef(ObjTypeFunda::EType type, Qualifier qualifiers,
  const TemplateParamType* templateArg1,
  const TemplateParamType* templateArg2,
  const TemplateParamType* templateArg3) :
  AstObjTypeRef{ObjTypeTemplateDefOf(type), qualifiers,
    templateArg1, templateArg2, templateArg3} {
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

bool AstObjTypeRef::hasConstructor(const ObjType2& param1ObjType) const {
  return canonicalObjTypeWithoutQualifiers().hasConstructor(param1ObjType);
}
