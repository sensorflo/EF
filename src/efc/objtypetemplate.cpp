#include "objtypetemplate.h"

using namespace std;

namespace {
string mangleName(string name, const ObjTypes& templateArgs) {
  size_t pos = name.find('_');
  while (pos != string::npos) {
    name.replace(pos, 1, "__");
    pos = name.find('_', pos + 1); // or +2?
  }
  if (!templateArgs.empty()) {
    name += "_begin_";
    auto isFirstIter = false;
    for (const auto& arg : templateArgs) {
      if (!isFirstIter) { name += "_c_"; }
      isFirstIter = false;
      name += arg.get().fqName();
    }
    name += "_end_";
  }
  return name;
}
}

vector<ObjTypeTemplate::UP_UnqualifiedObjType>
  ObjTypeTemplate::m_UnqualifiedObjTypePool;
vector<ObjTypeTemplate::UP_ObjType> ObjTypeTemplate::m_ObjTypePool;

ObjTypeTemplate::ObjTypeTemplate(string name) : EnvNode(move(name)) {
}

ObjTypeTemplate::~ObjTypeTemplate() = default;

ObjType& ObjTypeTemplate::instanciate(
  ObjTypes args, ObjType::Qualifiers qualifiers) {
  auto&& instanceName = mangleName(name(), args);
  return instanciate(
    move(instanceName), m_size, move(args), nullptr /*@todo*/, qualifiers);
}

ObjType& ObjTypeTemplate::instanciate(string name, size_t size,
  ObjTypes templateArgs, llvm::Type* llvmType, ObjType::Qualifiers qualifiers) {
  auto& otr = instanciateUnqualifiedObjType(
    move(name), size, move(templateArgs), llvmType);
  return instanciate(otr, qualifiers);
}

ObjType& ObjTypeTemplate::instanciateFunObjType(
  const ObjType& retObjType, ObjTypes paramObjTypes) {
  auto& otr = instanciateFunUnqualifiedObjType(retObjType, move(paramObjTypes));
  return instanciate(otr, ObjType::eNoQualifier);
}

ObjType& ObjTypeTemplate::instanciateRawPtrObjType(
  const ObjType& pointeeObjType, EPtrCanBeNull canBeNull, EPtrInit init,
  ObjType::Qualifiers qualifiers) {
  auto& otr =
    instanciateRawPtrUnqualifiedObjType(pointeeObjType, canBeNull, init);
  return instanciate(otr, qualifiers);
}

UnqualifiedObjType& ObjTypeTemplate::instanciateUnqualifiedObjType(
  string name, size_t size, ObjTypes templateArgs, llvm::Type* llvmType) {
  auto&& otr = UP_UnqualifiedObjType(
    new UnqualifiedObjType(move(name), size, move(templateArgs), llvmType));
  m_UnqualifiedObjTypePool.emplace_back(move(otr));
  return *m_UnqualifiedObjTypePool.back();
}

UnqualifiedObjType& ObjTypeTemplate::instanciateFunUnqualifiedObjType(
  const ObjType& retObjType, ObjTypes paramObjTypes) {
  auto&& otr = UP_UnqualifiedObjType(
    new FunUnqualifiedObjType(retObjType, move(paramObjTypes)));
  m_UnqualifiedObjTypePool.emplace_back(move(otr));
  return *m_UnqualifiedObjTypePool.back();
}

UnqualifiedObjType& ObjTypeTemplate::instanciateRawPtrUnqualifiedObjType(
  const ObjType& pointeeObjType, EPtrCanBeNull canBeNull, EPtrInit init) {
  auto&& otr = UP_UnqualifiedObjType(
    new RawPtrUnqualifiedObjType(pointeeObjType, canBeNull, init));
  m_UnqualifiedObjTypePool.emplace_back(move(otr));
  return *m_UnqualifiedObjTypePool.back();
}

ObjType& ObjTypeTemplate::instanciate(
  UnqualifiedObjType& otr, ObjType::Qualifiers qualifiers) {
  auto&& ot = UP_ObjType(new ObjType(otr, qualifiers));
  m_ObjTypePool.emplace_back(move(ot));
  return *m_ObjTypePool.back();
}
