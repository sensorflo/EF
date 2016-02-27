#include "test.h"
#include "../objtype2.h"
#include "../env.h"

using namespace testing;
using namespace std;

/** class BuiltinObjTypeTemplateDef: public ObjTypeTemplate, public EnvNode { */
// public:
//   BuiltinObjTypeTemplateDef(std::string name,
//     std::vector<TemplateParamType::Id> paramTypeIds = {})  :
//     EnvNode(move(name)),
//     m_paramTypeIds(move(paramTypeIds)) {
//   }
// 
//   // -- overrides for Template
//   const std::vector<TemplateParamType::Id>& paramTypeIds() const override {
//     return m_paramTypeIds; }
// 
//   // -- overrides for ObjTypeTemplate
//   const std::string& name() const override { return EnvNode::name(); }
// 
// private:
//   const std::vector<TemplateParamType::Id> m_paramTypeIds;
// };

#define AMEND_2_OBJ_TYPES(objType1, objType2)               \
  amend2ObjTypes(#objType1, objType1, #objType2, objType2)

string amend2ObjTypes(const string& objType1name, const ObjType2& objType1,
  const string& objType2name, const ObjType2& objType2) {
  ostringstream oss;
  oss << objType1name << " = " << objType1 << "\n" <<
    objType2name << " = " << objType2;
  return oss.str();
}

#define TEST_MATCH(spec, expectedMatchType, objType1, objType2)   \
  { SCOPED_TRACE("testMatch called from here (via TEST_MATCH)");  \
    testMatch(spec, expectedMatchType, objType1, objType2);       \
  }

void testMatch(const string& spec, ObjType2::MatchType expectedMatchType,
  const ObjType2& objType1, const ObjType2& objType2 ) {
  EXPECT_EQ( expectedMatchType, objType1.match(objType2) ) <<
    amendSpec(spec) << AMEND_2_OBJ_TYPES(objType1, objType2);
}


#if 0
TEST(ObjType2Test, MAKE_TEST_NAME1(match)) {

  // -- setup
  AstObjTypeRef uut_int_1(ObjTypeFunda::eInt);
  AstObjTypeRef uut_int_2(ObjTypeFunda::eInt);
  AstObjTypeRef uut_int_mut(ObjTypeFunda::eInt, ObjType2::eMutable);
  AstObjTypeRef uut_double1(ObjTypeFunda::eDouble);

  AstObjTypeRef uut_raw_ptr_1_to_int(rawPtrDef, ObjType2::eNoQualifier,
    new AstObjTypeRef(intDef));
  AstObjTypeRef uut_raw_ptr_2_to_int(rawPtrDef, ObjType2::eNoQualifier,
    new AstObjTypeRef(intDef));
  AstObjTypeRef uut_raw_ptr_3_to_double(rawPtrDef, ObjType2::eNoQualifier,
    new AstObjTypeRef(doubleDef));

  // vector<unique_ptr<ObjType2>> noParams;
  // vector<unique_ptr<ObjType2>> doubleParam;
  // doubleParam.emplace_back(make_unique<AstObjTypeRef>(doubleDef));
  // AstObjTypeRef uut_fun_1(funDef, ObjType2::eNoQualifier,
  //   new AstObjTypeRef(voidDef),
  //   new ObjType2List(noParams.begin(), noParams.end()));
  // AstObjTypeRef uut_fun_2(funDef, ObjType2::eNoQualifier,
  //   new AstObjTypeRef(voidDef),
  //   new ObjType2List(noParams.begin(), noParams.end()));
  // AstObjTypeRef uut_fun_int(funDef, ObjType2::eNoQualifier,
  //   new AstObjTypeRef(intDef),
  //   new ObjType2List(noParams.begin(), noParams.end()));
  // AstObjTypeRef uut_fun_double(funDef, ObjType2::eNoQualifier,
  //   new AstObjTypeRef(doubleDef),
  //   new ObjType2List(noParams.begin(), noParams.end()));
  // 
  // AstObjTypeRef uut_fun_int_double(funDef, ObjType2::eNoQualifier,
  //   new AstObjTypeRef(intDef),
  //   new ObjType2List(doubleParam.begin(), doubleParam.end()));

  // -- exercise & verify
  TEST_MATCH( "type and qualifier fully match", ObjType2::eFullMatch,
    uut_int_1, uut_int_2);
  TEST_MATCH( "type mismatch", ObjType2::eNoMatch,
    uut_int_1, uut_double1);
  TEST_MATCH( "type matches, but dst has weaker qualifiers", ObjType2::eMatchButAllQualifiersAreWeaker,
    uut_int_1, uut_int_mut);
  TEST_MATCH( "type matches, but dst has stronger qualifiers", ObjType2::eMatchButAnyQualifierIsStronger,
    uut_int_mut, uut_int_1);

  TEST_MATCH( "type and qualifier fully match", ObjType2::eFullMatch,
    uut_raw_ptr_1_to_int, uut_raw_ptr_2_to_int);
  TEST_MATCH( "type mismatch", ObjType2::eNoMatch,
    uut_raw_ptr_1_to_int, uut_raw_ptr_3_to_double);

  // TEST_MATCH( "Fun type: return type and types of fun params all match",
  //   ObjType2::eFullMatch,
  //   uut_fun_1, uut_fun_2);
  // TEST_MATCH( "Fun type: return type and types of fun params all match",
  //   ObjType2::eFullMatch,
  //   uut_fun_int_double, uut_fun_int_double);
  // TEST_MATCH( "Fun type: return types don't match, while fun params do match",
  //   ObjType2::eNoMatch,
  //   uut_fun_int, uut_fun_int_double);


  // bug: if template args dont don't match the qualifiers don't matter

}
#endif

#if 0
TEST(ObjType2Test, MAKE_TEST_NAME1(match)) {
  // -- setup

  BuiltinObjTypeTemplateDef voidDef("Void");
  BuiltinObjTypeTemplateDef intDef("Int");
  BuiltinObjTypeTemplateDef doubleDef("Double");
  BuiltinObjTypeTemplateDef rawPtrDef("RawPtr",
    {TemplateParamType::objType2});
  BuiltinObjTypeTemplateDef funDef("Fun",
    {TemplateParamType::objType2, TemplateParamType::objType2List});

  AstObjTypeRef uut_int_1(intDef);
  AstObjTypeRef uut_int_2(intDef);
  AstObjTypeRef uut_int_mut(intDef, ObjType2::eMutable);
  AstObjTypeRef uut_double1(doubleDef);

  AstObjTypeRef uut_raw_ptr_1_to_int(rawPtrDef, ObjType2::eNoQualifier,
    new AstObjTypeRef(intDef));
  AstObjTypeRef uut_raw_ptr_2_to_int(rawPtrDef, ObjType2::eNoQualifier,
    new AstObjTypeRef(intDef));
  AstObjTypeRef uut_raw_ptr_3_to_double(rawPtrDef, ObjType2::eNoQualifier,
    new AstObjTypeRef(doubleDef));

  vector<unique_ptr<ObjType2>> noParams;
  vector<unique_ptr<ObjType2>> doubleParam;
  doubleParam.emplace_back(make_unique<AstObjTypeRef>(doubleDef));

  AstObjTypeRef uut_fun_1(funDef, ObjType2::eNoQualifier,
    new AstObjTypeRef(voidDef),
    new ObjType2List(noParams.begin(), noParams.end()));
  AstObjTypeRef uut_fun_2(funDef, ObjType2::eNoQualifier,
    new AstObjTypeRef(voidDef),
    new ObjType2List(noParams.begin(), noParams.end()));
  AstObjTypeRef uut_fun_int(funDef, ObjType2::eNoQualifier,
    new AstObjTypeRef(intDef),
    new ObjType2List(noParams.begin(), noParams.end()));
  AstObjTypeRef uut_fun_double(funDef, ObjType2::eNoQualifier,
    new AstObjTypeRef(doubleDef),
    new ObjType2List(noParams.begin(), noParams.end()));

  AstObjTypeRef uut_fun_int_double(funDef, ObjType2::eNoQualifier,
    new AstObjTypeRef(intDef),
    new ObjType2List(doubleParam.begin(), doubleParam.end()));

  // -- exercise & verify
  TEST_MATCH( "type and qualifier fully match", ObjType2::eFullMatch,
    uut_int_1, uut_int_2);
  TEST_MATCH( "type mismatch", ObjType2::eNoMatch,
    uut_int_1, uut_double1);
  TEST_MATCH( "type matches, but dst has weaker qualifiers", ObjType2::eMatchButAllQualifiersAreWeaker,
    uut_int_1, uut_int_mut);
  TEST_MATCH( "type matches, but dst has stronger qualifiers", ObjType2::eMatchButAnyQualifierIsStronger,
    uut_int_mut, uut_int_1);

  TEST_MATCH( "type and qualifier fully match", ObjType2::eFullMatch,
    uut_raw_ptr_1_to_int, uut_raw_ptr_2_to_int);
  TEST_MATCH( "type mismatch", ObjType2::eNoMatch,
    uut_raw_ptr_1_to_int, uut_raw_ptr_3_to_double);

  TEST_MATCH( "Fun type: return type and types of fun params all match",
    ObjType2::eFullMatch,
    uut_fun_1, uut_fun_2);
  TEST_MATCH( "Fun type: return type and types of fun params all match",
    ObjType2::eFullMatch,
    uut_fun_int_double, uut_fun_int_double);
  TEST_MATCH( "Fun type: return types don't match, while fun params do match",
    ObjType2::eNoMatch,
    uut_fun_int, uut_fun_int_double);


  // bug: if template args dont don't match the qualifiers don't matter

}
#endif
