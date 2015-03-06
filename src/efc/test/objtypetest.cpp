#include "test.h"
#include "../objtype.h"
#include <string>
using namespace testing;
using namespace std;

#define AMEND_2_OBJ_TYPES(objType1, objType2)   \
  amend2ObjTypes(#objType1, objType1, #objType2, objType2)

string amend2ObjTypes(const string& objType1name, const ObjType& objType1,
  const string& objType2name, const ObjType& objType2) {
  ostringstream oss;
  oss << objType1name << " = " << objType1 << "\n" <<
    objType2name << " = " << objType2;
  return oss.str();
}

#define TEST_MATCH(spec, expectedMatchType, objType1, objType2)         \
  { SCOPED_TRACE("testMatch called from here (via TEST_MATCH)");       \
    testMatch(spec, expectedMatchType, objType1, objType2);             \
  }

void testMatch(const string& spec, ObjType::MatchType expectedMatchType,
  const ObjType& objType1, const ObjType& objType2 ) {
   EXPECT_EQ( expectedMatchType, objType1.match(objType2) ) <<
    amendSpec(spec) << AMEND_2_OBJ_TYPES(objType1, objType2);
}

TEST(ObjTypeTest, MAKE_TEST_NAME1(toStr)) {
  // fundamental types
  EXPECT_EQ("int", ObjTypeFunda(ObjTypeFunda::eInt).toStr());
  EXPECT_EQ("int-mut", ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable).toStr());
  EXPECT_EQ("bool", ObjTypeFunda(ObjTypeFunda::eBool).toStr());
  EXPECT_EQ("bool-mut", ObjTypeFunda(ObjTypeFunda::eBool, ObjType::eMutable).toStr());

  // function type
  EXPECT_EQ("fun((), int)",
    ObjTypeFun(ObjTypeFun::createArgs(), new ObjTypeFunda(ObjTypeFunda::eInt)).toStr());
  EXPECT_EQ("fun((int, int), int)",
    ObjTypeFun(
      ObjTypeFun::createArgs(
        new ObjTypeFunda(ObjTypeFunda::eInt),
        new ObjTypeFunda(ObjTypeFunda::eInt)),
      new ObjTypeFunda(ObjTypeFunda::eInt)
      ).toStr());
  EXPECT_EQ("fun((bool, int), bool)",
    ObjTypeFun(
      ObjTypeFun::createArgs(
        new ObjTypeFunda(ObjTypeFunda::eBool),
        new ObjTypeFunda(ObjTypeFunda::eInt)),
      new ObjTypeFunda(ObjTypeFunda::eBool)
      ).toStr());
}

TEST(ObjTypeTest, MAKE_TEST_NAME1(
    addQualifiers)) {
  EXPECT_EQ(
    ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable).qualifiers(),
    ObjTypeFunda(ObjTypeFunda::eInt).addQualifiers(ObjType::eMutable).qualifiers());

  EXPECT_EQ(
    ObjTypeFunda(ObjTypeFunda::eBool, ObjType::eMutable).qualifiers(),
    ObjTypeFunda(ObjTypeFunda::eBool).addQualifiers(ObjType::eMutable).qualifiers());

  EXPECT_EQ(
    ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable).qualifiers(),
    ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable).addQualifiers(ObjType::eMutable).qualifiers());
}

TEST(ObjTypeTest, MAKE_TEST_NAME1(
    match)) {

  TEST_MATCH( "", ObjType::eFullMatch,
    ObjTypeFunda(ObjTypeFunda::eInt), ObjTypeFunda(ObjTypeFunda::eInt) );

  TEST_MATCH( "", ObjType::eFullMatch,
    ObjTypeFunda(ObjTypeFunda::eBool), ObjTypeFunda(ObjTypeFunda::eBool) );

  TEST_MATCH( "", ObjType::eNoMatch,
    ObjTypeFunda(ObjTypeFunda::eBool), ObjTypeFunda(ObjTypeFunda::eInt) );

  TEST_MATCH( "", ObjType::eOnlyQualifierMismatches,
    ObjTypeFunda(ObjTypeFunda::eInt), ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable));

  TEST_MATCH( "", ObjType::eNoMatch,
    ObjTypeFunda(ObjTypeFunda::eInt),
    ObjTypeFun(ObjTypeFun::createArgs(), new ObjTypeFunda(ObjTypeFunda::eInt)));

  TEST_MATCH( "", ObjType::eFullMatch,
    ObjTypeFun(ObjTypeFun::createArgs(), new ObjTypeFunda(ObjTypeFunda::eInt)),
    ObjTypeFun(ObjTypeFun::createArgs(), new ObjTypeFunda(ObjTypeFunda::eInt)));

  TEST_MATCH( "", ObjType::eNoMatch,
    ObjTypeFun(ObjTypeFun::createArgs(new ObjTypeFunda(ObjTypeFunda::eInt)), new ObjTypeFunda(ObjTypeFunda::eInt)),
    ObjTypeFun(ObjTypeFun::createArgs(), new ObjTypeFunda(ObjTypeFunda::eInt)));

  TEST_MATCH( "Only one argument differs in type", ObjType::eNoMatch,
    ObjTypeFun(
      ObjTypeFun::createArgs(
        new ObjTypeFunda(ObjTypeFunda::eBool)),
      new ObjTypeFunda(ObjTypeFunda::eInt)),
    ObjTypeFun(
      ObjTypeFun::createArgs(
        new ObjTypeFunda(ObjTypeFunda::eInt)),
      new ObjTypeFunda(ObjTypeFunda::eInt)));

  TEST_MATCH( "Only return type differs", ObjType::eNoMatch,
    ObjTypeFun(
      ObjTypeFun::createArgs(
        new ObjTypeFunda(ObjTypeFunda::eInt)),
      new ObjTypeFunda(ObjTypeFunda::eBool)),
    ObjTypeFun(
      ObjTypeFun::createArgs(
        new ObjTypeFunda(ObjTypeFunda::eInt)),
      new ObjTypeFunda(ObjTypeFunda::eInt)));
}
