#include "test.h"
#include "../objtype.h"
#include <string>
#include <memory>
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
  EXPECT_EQ("void", ObjTypeFunda(ObjType::eVoid).toStr());
  EXPECT_EQ("noreturn", ObjTypeFunda(ObjType::eNoreturn).toStr());
  EXPECT_EQ("char", ObjTypeFunda(ObjType::eChar).toStr());
  EXPECT_EQ("int", ObjTypeFunda(ObjType::eInt).toStr());
  EXPECT_EQ("bool", ObjTypeFunda(ObjType::eBool).toStr());
  EXPECT_EQ("double", ObjTypeFunda(ObjType::eDouble).toStr());

  // qualifiers
  EXPECT_EQ("mut-int", ObjTypeQuali(ObjType::eMutable,
    make_shared<ObjTypeFunda>(ObjType::eInt)).toStr());

  // pointer type
  EXPECT_EQ("raw*int", ObjTypePtr(make_shared<ObjTypeFunda>(ObjType::eInt)).toStr());

  // function type
  EXPECT_EQ("fun(() int)",
    ObjTypeFun(ObjTypeFun::createArgs(), make_shared<ObjTypeFunda>(ObjType::eInt)).toStr());
  EXPECT_EQ("fun((int, int) int)",
    ObjTypeFun(
      ObjTypeFun::createArgs(
        new ObjTypeFunda(ObjType::eInt),
        new ObjTypeFunda(ObjType::eInt)),
      make_shared<ObjTypeFunda>(ObjType::eInt)
      ).toStr());
  EXPECT_EQ("fun((bool, int) bool)",
    ObjTypeFun(
      ObjTypeFun::createArgs(
        new ObjTypeFunda(ObjType::eBool),
        new ObjTypeFunda(ObjType::eInt)),
      make_shared<ObjTypeFunda>(ObjType::eBool)
      ).toStr());
}

TEST(ObjTypeTest, MAKE_TEST_NAME1(
    match)) {

  // fundamental type <-> fundamental type
  // -------------------------------------
  TEST_MATCH( "type and qualifier fully match", ObjType::eFullMatch,
    ObjTypeFunda(ObjType::eInt), ObjTypeFunda(ObjType::eInt) );

  TEST_MATCH( "type and qualifier fully match", ObjType::eFullMatch,
    ObjTypeFunda(ObjType::eBool), ObjTypeFunda(ObjType::eBool) );

  TEST_MATCH( "type mismatch", ObjType::eNoMatch,
    ObjTypeFunda(ObjType::eBool), ObjTypeFunda(ObjType::eInt) );

  TEST_MATCH( "type matches, but dst has weaker qualifiers", ObjType::eMatchButAllQualifiersAreWeaker,
    ObjTypeFunda(ObjType::eInt),
    ObjTypeQuali(ObjType::eMutable, make_shared<ObjTypeFunda>(ObjType::eInt)));

  TEST_MATCH( "type matches, but dst has stronger qualifiers", ObjType::eMatchButAnyQualifierIsStronger,
    ObjTypeQuali(ObjType::eMutable, make_shared<ObjTypeFunda>(ObjType::eInt)),
    ObjTypeFunda(ObjType::eInt));

  // pointer <-> pointer
  // -------------------
  TEST_MATCH( "full match", ObjType::eFullMatch,
    ObjTypePtr(make_shared<ObjTypeFunda>(ObjType::eInt)),
    ObjTypePtr(make_shared<ObjTypeFunda>(ObjType::eInt)));

  TEST_MATCH( "pointee type differs", ObjType::eNoMatch,
    ObjTypePtr(make_shared<ObjTypeFunda>(ObjType::eInt)),
    ObjTypePtr(make_shared<ObjTypeFunda>(ObjType::eBool)));

  TEST_MATCH( "qualifier of level0 differs", ObjType::eMatchButAnyQualifierIsStronger,
    ObjTypeQuali(ObjType::eMutable, make_shared<ObjTypePtr>(make_shared<ObjTypeFunda>(ObjType::eInt))),
    ObjTypePtr(make_shared<ObjTypeFunda>(ObjType::eInt)));

  TEST_MATCH( "qualifier of level0 differs", ObjType::eMatchButAllQualifiersAreWeaker,
    ObjTypePtr(make_shared<ObjTypeFunda>(ObjType::eInt)),
    ObjTypeQuali(ObjType::eMutable, make_shared<ObjTypePtr>(make_shared<ObjTypeFunda>(ObjType::eInt))));

  TEST_MATCH( "qualifier of level1+ differs", ObjType::eNoMatch,
    ObjTypePtr(make_shared<ObjTypeQuali>(ObjType::eMutable, make_shared<ObjTypeFunda>(ObjType::eInt))),
    ObjTypePtr(make_shared<ObjTypeFunda>(ObjType::eInt)));

  TEST_MATCH( "qualifier of level1+ differs", ObjType::eNoMatch,
    ObjTypePtr(make_shared<ObjTypeFunda>(ObjType::eInt)),
    ObjTypePtr(make_shared<ObjTypeQuali>(ObjType::eMutable, make_shared<ObjTypeFunda>(ObjType::eInt))));


  // fundamental type <-> pointer <-> function
  // -----------------------------------------
  TEST_MATCH( "any fundamental type mismatches any pointer type", ObjType::eNoMatch,
    ObjTypeFunda(ObjType::eInt),
    ObjTypePtr(make_shared<ObjTypeFunda>(ObjType::eInt)));

  TEST_MATCH( "any fundamental type mismatches any pointer type", ObjType::eNoMatch,
    ObjTypePtr(make_shared<ObjTypeFunda>(ObjType::eInt)),
    ObjTypeFunda(ObjType::eInt));

  TEST_MATCH( "any fundamental type mismatches any function type", ObjType::eNoMatch,
    ObjTypeFunda(ObjType::eInt),
    ObjTypeFun(ObjTypeFun::createArgs(), make_shared<ObjTypeFunda>(ObjType::eInt)));


  // function <-> function
  // ---------------------
  TEST_MATCH( "", ObjType::eFullMatch,
    ObjTypeFun(ObjTypeFun::createArgs(), make_shared<ObjTypeFunda>(ObjType::eInt)),
    ObjTypeFun(ObjTypeFun::createArgs(), make_shared<ObjTypeFunda>(ObjType::eInt)));

  TEST_MATCH( "", ObjType::eNoMatch,
    ObjTypeFun(
      ObjTypeFun::createArgs(new ObjTypeFunda(ObjType::eInt)),
      make_shared<ObjTypeFunda>(ObjType::eInt)),
    ObjTypeFun(
      ObjTypeFun::createArgs(),
      make_shared<ObjTypeFunda>(ObjType::eInt)));

  TEST_MATCH( "Only one argument differs in type", ObjType::eNoMatch,
    ObjTypeFun(
      ObjTypeFun::createArgs(
        new ObjTypeFunda(ObjType::eBool)),
      make_shared<ObjTypeFunda>(ObjType::eInt)),
    ObjTypeFun(
      ObjTypeFun::createArgs(
        new ObjTypeFunda(ObjType::eInt)),
      make_shared<ObjTypeFunda>(ObjType::eInt)));

  TEST_MATCH( "Only return type differs", ObjType::eNoMatch,
    ObjTypeFun(
      ObjTypeFun::createArgs(
        new ObjTypeFunda(ObjType::eInt)),
      make_shared<ObjTypeFunda>(ObjType::eBool)),
    ObjTypeFun(
      ObjTypeFun::createArgs(
        new ObjTypeFunda(ObjType::eInt)),
      make_shared<ObjTypeFunda>(ObjType::eInt)));
}

TEST(ObjTypeTest, MAKE_TEST_NAME1(
    hasConstructor)) {

  // between fundamental types
  // =========================
  struct T {
    ObjType::EType UUT;
    ObjType::EType arg;
    bool hasConstructor;
  };
  vector<T> inputSpecs{
    // abstract types have no members
    {ObjType::eVoid, ObjType::eVoid, false},
    {ObjType::eVoid, ObjType::eNoreturn, false},
    {ObjType::eVoid, ObjType::eBool, false},
    {ObjType::eVoid, ObjType::eChar, false},
    {ObjType::eVoid, ObjType::eInt, false},
    {ObjType::eVoid, ObjType::eDouble, false},
    {ObjType::eVoid, ObjType::ePointer, false},
    {ObjType::eNoreturn, ObjType::eVoid, false},
    {ObjType::eNoreturn, ObjType::eNoreturn, false},
    {ObjType::eNoreturn, ObjType::eBool, false},
    {ObjType::eNoreturn, ObjType::eChar, false},
    {ObjType::eNoreturn, ObjType::eInt, false},
    {ObjType::eNoreturn, ObjType::eDouble, false},
    {ObjType::eNoreturn, ObjType::ePointer, false},

    // all concrete fundamental types
    // - can _not_ be constructed with an abstract type
    // - can be constructed with a concrete fundamental type
    {ObjType::eBool, ObjType::eVoid, false},
    {ObjType::eBool, ObjType::eNoreturn, false},
    {ObjType::eBool, ObjType::eBool, true},
    {ObjType::eBool, ObjType::eChar, true},
    {ObjType::eBool, ObjType::eInt, true},
    {ObjType::eBool, ObjType::eDouble, true},
    {ObjType::eBool, ObjType::ePointer, true},
    {ObjType::eChar, ObjType::eVoid, false},
    {ObjType::eChar, ObjType::eNoreturn, false},
    {ObjType::eChar, ObjType::eBool, true},
    {ObjType::eChar, ObjType::eChar, true},
    {ObjType::eChar, ObjType::eInt, true},
    {ObjType::eChar, ObjType::eDouble, true},
    {ObjType::eChar, ObjType::ePointer, true},
    {ObjType::eInt, ObjType::eVoid, false},
    {ObjType::eInt, ObjType::eNoreturn, false},
    {ObjType::eInt, ObjType::eBool, true},
    {ObjType::eInt, ObjType::eChar, true},
    {ObjType::eInt, ObjType::eInt, true},
    {ObjType::eInt, ObjType::eDouble, true},
    {ObjType::eInt, ObjType::eDouble, true},
    {ObjType::eDouble, ObjType::eVoid, false},
    {ObjType::eDouble, ObjType::eNoreturn, false},
    {ObjType::eDouble, ObjType::eBool, true},
    {ObjType::eDouble, ObjType::eChar, true},
    {ObjType::eDouble, ObjType::eInt, true},
    {ObjType::eDouble, ObjType::eDouble, true},
    {ObjType::eDouble, ObjType::ePointer, true},
    {ObjType::ePointer, ObjType::eVoid, false},
    {ObjType::ePointer, ObjType::eNoreturn, false},
    {ObjType::ePointer, ObjType::eBool, true},
    {ObjType::ePointer, ObjType::eChar, true},
    {ObjType::ePointer, ObjType::eInt, true},
    {ObjType::ePointer, ObjType::eDouble, true},
    {ObjType::ePointer, ObjType::ePointer, true}};

  for ( const auto& inputSpec : inputSpecs ) {
    ObjTypeFunda UUT{ inputSpec.UUT };
    ObjTypeFunda arg{ inputSpec.arg };
    EXPECT_EQ( inputSpec.hasConstructor, UUT.hasConstructor(arg))
      << "UUT: " << UUT << "\n"
      << "arg: " << arg << "\n";
  }

  // one type is not fundamental type
  // ================================
  {
    ObjTypeFunda fundamentaltype{ObjType::eInt};
    ObjTypeFun functiontype{
      ObjTypeFun::createArgs(),
      make_shared<ObjTypeFunda>(ObjType::eInt)};
    EXPECT_FALSE( fundamentaltype.hasConstructor(functiontype) );
    EXPECT_FALSE( functiontype.hasConstructor(fundamentaltype) );
    EXPECT_FALSE( functiontype.hasConstructor(functiontype) );
  }
}

TEST(ObjTypeTest, MAKE_TEST_NAME1(
    class_)) {

  struct T{
    bool m_isMemberOfClass;
    ObjType::EType m_fundaType;
    ObjType::EClass m_typeClass;
  };

  vector<T> inputs{
    {true,  ObjType::eVoid, ObjType::eAbstract},
    {false, ObjType::eVoid, ObjType::eScalar},
    {false, ObjType::eVoid, ObjType::eArithmetic},
    {false, ObjType::eVoid, ObjType::eIntegral},
    {false, ObjType::eVoid, ObjType::eFloatingPoint},
    {false, ObjType::eVoid, ObjType::eStoredAsIntegral},
    {false, ObjType::eVoid, ObjType::eFunction},

    {true,  ObjType::eNoreturn, ObjType::eAbstract},
    {false, ObjType::eNoreturn, ObjType::eScalar},
    {false, ObjType::eNoreturn, ObjType::eArithmetic},
    {false, ObjType::eNoreturn, ObjType::eIntegral},
    {false, ObjType::eNoreturn, ObjType::eFloatingPoint},
    {false, ObjType::eNoreturn, ObjType::eStoredAsIntegral},
    {false, ObjType::eNoreturn, ObjType::eFunction},

    {false, ObjType::eBool, ObjType::eAbstract},
    {true,  ObjType::eBool, ObjType::eScalar},
    {false, ObjType::eBool, ObjType::eArithmetic},
    {false, ObjType::eBool, ObjType::eIntegral},
    {false, ObjType::eBool, ObjType::eFloatingPoint},
    {true,  ObjType::eBool, ObjType::eStoredAsIntegral},
    {false, ObjType::eBool, ObjType::eFunction},

    {false, ObjType::eChar, ObjType::eAbstract},
    {true,  ObjType::eChar, ObjType::eScalar},
    {false, ObjType::eChar, ObjType::eArithmetic},
    {false, ObjType::eChar, ObjType::eIntegral},
    {false, ObjType::eChar, ObjType::eFloatingPoint},
    {true,  ObjType::eChar, ObjType::eStoredAsIntegral},
    {false, ObjType::eChar, ObjType::eFunction},

    {false, ObjType::eInt, ObjType::eAbstract},
    {true,  ObjType::eInt, ObjType::eScalar},
    {true,  ObjType::eInt, ObjType::eArithmetic},
    {true,  ObjType::eInt, ObjType::eIntegral},
    {false, ObjType::eInt, ObjType::eFloatingPoint},
    {true,  ObjType::eInt, ObjType::eStoredAsIntegral},
    {false, ObjType::eInt, ObjType::eFunction},

    {false, ObjType::eDouble, ObjType::eAbstract},
    {true,  ObjType::eDouble, ObjType::eScalar},
    {true,  ObjType::eDouble, ObjType::eArithmetic},
    {false, ObjType::eDouble, ObjType::eIntegral},
    {true,  ObjType::eDouble, ObjType::eFloatingPoint},
    {false, ObjType::eDouble, ObjType::eStoredAsIntegral},
    {false, ObjType::eDouble, ObjType::eFunction},

    {false, ObjType::ePointer, ObjType::eAbstract},
    {true,  ObjType::ePointer, ObjType::eScalar},
    {false, ObjType::ePointer, ObjType::eArithmetic},
    {false, ObjType::ePointer, ObjType::eIntegral},
    {false, ObjType::ePointer, ObjType::eFloatingPoint},
    {true,  ObjType::ePointer, ObjType::eStoredAsIntegral},
    {false, ObjType::ePointer, ObjType::eFunction}};

  for (const auto& i : inputs) {
    EXPECT_EQ( i.m_isMemberOfClass, ObjTypeFunda(i.m_fundaType).is(i.m_typeClass))
      << "fundamental type: " << i.m_fundaType << "\n"
      << "type class: " << i.m_typeClass;
  }

  ObjTypePtr objTypePtr(make_shared<ObjTypeFunda>(ObjType::eInt)); 
  EXPECT_FALSE(objTypePtr.is(ObjType::eAbstract));
  EXPECT_TRUE (objTypePtr.is(ObjType::eScalar));
  EXPECT_FALSE(objTypePtr.is(ObjType::eArithmetic));
  EXPECT_FALSE(objTypePtr.is(ObjType::eIntegral));
  EXPECT_FALSE(objTypePtr.is(ObjType::eFloatingPoint));
  EXPECT_TRUE (objTypePtr.is(ObjType::eStoredAsIntegral));
  EXPECT_FALSE(objTypePtr.is(ObjType::eFunction));

  EXPECT_TRUE( ObjTypeFun(ObjTypeFun::createArgs()).is(ObjType::eFunction));
}

TEST(ObjTypeTest, MAKE_TEST_NAME1(
    size)) {
  EXPECT_EQ( -1, ObjTypeFunda(ObjType::eVoid).size());
  EXPECT_EQ( -1, ObjTypeFunda(ObjType::eNoreturn).size());
  EXPECT_EQ( 1, ObjTypeFunda(ObjType::eBool).size());
  EXPECT_EQ( 8, ObjTypeFunda(ObjType::eChar).size());
  EXPECT_EQ( 32, ObjTypeFunda(ObjType::eInt).size());
  EXPECT_EQ( 64, ObjTypeFunda(ObjType::eDouble).size());

  EXPECT_EQ( -1, ObjTypeFun(ObjTypeFun::createArgs()).size());
}
