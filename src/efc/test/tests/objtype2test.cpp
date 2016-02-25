#include "test.h"
#include "../objtype2.h"
#include "../env.h"

using namespace testing;
using namespace std;

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


TEST(ObjType2Test, MAKE_TEST_NAME1(match)) {
  // -- setup

  BuiltinObjTypeTemplateDef intDef("int");
  BuiltinObjTypeTemplateDef doubleDef("double");
  BuiltinObjTypeTemplateDef rawPtrDef("raw_ptr", {TemplateParamType::objType2});

  AstObjTypeRef uut_int1(intDef);
  AstObjTypeRef uut_int2(intDef);
  AstObjTypeRef uut_int_mut(intDef, ObjType2::eMutable);
  AstObjTypeRef uut_double1(doubleDef);

  AstObjTypeRef uut_raw_ptr_1_to_int(rawPtrDef, ObjType2::eNoQualifier,
    new AstObjTypeRef(intDef));
  AstObjTypeRef uut_raw_ptr_2_to_int(rawPtrDef, ObjType2::eNoQualifier,
    new AstObjTypeRef(intDef));
  AstObjTypeRef uut_raw_ptr_3_to_double(rawPtrDef, ObjType2::eNoQualifier,
    new AstObjTypeRef(doubleDef));

  // -- exercise & verify
  TEST_MATCH( "type and qualifier fully match", ObjType2::eFullMatch,
    uut_int1, uut_int1);
  TEST_MATCH( "type mismatch", ObjType2::eNoMatch,
    uut_int1, uut_double1);
  TEST_MATCH( "type matches, but dst has weaker qualifiers", ObjType2::eMatchButAllQualifiersAreWeaker,
    uut_int1, uut_int_mut);
  TEST_MATCH( "type matches, but dst has stronger qualifiers", ObjType2::eMatchButAnyQualifierIsStronger,
    uut_int_mut, uut_int1);

  TEST_MATCH( "type and qualifier fully match", ObjType2::eFullMatch,
    uut_raw_ptr_1_to_int, uut_raw_ptr_2_to_int);
  TEST_MATCH( "type mismatch", ObjType2::eNoMatch,
    uut_raw_ptr_1_to_int, uut_raw_ptr_3_to_double);
}
