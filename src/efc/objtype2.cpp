#include "objtype2.h"
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

BuiltinObjTypeTemplateDef::BuiltinObjTypeTemplateDef(string name,
  vector<TemplateParamType::Id> paramTypeIds) :
  EnvNode(move(name)),
  m_paramTypeIds(move(paramTypeIds)) {
}

ObjType2::MatchType ObjType2::match(const TemplateParamType& lhs) const {
  return lhs.match2(*this);
}

ObjType2::MatchType ObjType2::match2(const ObjType2& rhs) const {
  if (&def() != &rhs.def()) {
    return eNoMatch;
  }
  else if (!(qualifiers()&eMutable) && (rhs.qualifiers()&eMutable)) {
    return eMatchButAnyQualifierIsStronger;
  }
  else if ((qualifiers()&eMutable) && !(rhs.qualifiers()&eMutable)) {
    return eMatchButAllQualifiersAreWeaker;
  }
  else if (!def().paramTypeIds().empty()) {
    auto rhsTemplateArg = rhs.templateArgs().begin();
    for (const auto& templateArg: templateArgs()) {
      const auto argMatch = templateArg->match(**rhsTemplateArg);
      if ( argMatch!=eFullMatch ) {
        return argMatch;
      }
      ++rhsTemplateArg;
    }
    return eFullMatch;
  } else {
    return eFullMatch;
  }
}

void ObjType2::printTo(basic_ostream<char>& os) const {
  if ( qualifiers()&eMutable ) {
    os << "mut-";
  }
  // os << def().name();
  if ( !templateArgs().empty() ) {
    os << "{";
    auto isFirtIteration = true;
    for (const auto& templateArg : templateArgs()) {
      assert(templateArg);
      if ( !isFirtIteration ) {
        os << ", ";
      }
      isFirtIteration = false;
      os << *templateArg;
    }
    os << "}";
  }
}

AstObjTypeRef::AstObjTypeRef(string name, Qualifier qualifiers,
  vector<const TemplateParamType*>* templateArgs) :
  m_name{move(name)},
  m_qualifiers{qualifiers},
  m_templateArgs{toUniquePtrs(templateArgs)},
  m_def{} {
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

const ObjTypeTemplate& AstObjTypeRef::def() const {
  assert(m_def);
  return *m_def;
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

const vector<unique_ptr<const TemplateParamType>>& AstObjTypeRef::templateArgs() const {
  return m_templateArgs;
}
