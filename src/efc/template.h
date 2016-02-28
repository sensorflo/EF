#pragma once
#include "declutils.h"
#include "templateparamtype.h"
#include <string>
#include <vector>

class ObjType2;

class Template {
public:
  virtual ~Template() = default;
  virtual const std::vector<TemplateParamType::Id>& paramTypeIds() const =0;

protected:
  Template() = default;

private:
  NEITHER_COPY_NOR_MOVEABLE(Template);
};

class ObjTypeTemplate: public Template {
public:
  // -- new virtual methods
  virtual const std::string& name() const =0;
  virtual const ObjType2& instanceFor(const TemplateParams&) const =0;

protected:
  ObjTypeTemplate() = default;
};
