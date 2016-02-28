#pragma once
#include "declutils.h"
#include <vector>
#include <ostream>

class ObjType2;
class ObjTypeList;

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

  // -- new virtual methods
  virtual Id id() const =0;
  virtual void printTo(std::basic_ostream<char>& os) const =0;
  virtual MatchType match(const TemplateParamType& lhs) const =0;
  virtual MatchType match2(const ObjType2& rhs) const { return eNoMatch; }
  virtual MatchType match2(const ObjTypeList& rhs) const { return eNoMatch; }

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
