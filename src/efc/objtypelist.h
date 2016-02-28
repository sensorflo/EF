#pragma once
#include "templateparamtype.h"
#include <ostream>
#include <memory>

class ObjTypeList: public TemplateParamType {
public:
  using ObjTypes2Iter = std::vector<std::unique_ptr<ObjType2>>::iterator;

  ObjTypeList(ObjTypes2Iter begin, ObjTypes2Iter end);

  // -- overrides for TemplateParamType
  Id id() const override { return objType2List; }
  void printTo(std::basic_ostream<char>& os) const override;
  MatchType match(const TemplateParamType& rhs) const override;
  MatchType match2(const ObjTypeList& other) const override;

private:
  const ObjTypes2Iter m_begin;
  const ObjTypes2Iter m_end;
};
