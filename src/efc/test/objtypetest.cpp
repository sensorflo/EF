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
  EXPECT_EQ("void", ObjTypeFunda(ObjTypeFunda::eVoid).toStr());
  EXPECT_EQ("int", ObjTypeFunda(ObjTypeFunda::eInt).toStr());
  EXPECT_EQ("int-mut", ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable).toStr());
  EXPECT_EQ("bool", ObjTypeFunda(ObjTypeFunda::eBool).toStr());
  EXPECT_EQ("bool-mut", ObjTypeFunda(ObjTypeFunda::eBool, ObjType::eMutable).toStr());
  EXPECT_EQ("double", ObjTypeFunda(ObjTypeFunda::eDouble).toStr());

  // function type
  EXPECT_EQ("fun((), int)",
    ObjTypeFun(ObjTypeFun::createArgs(), make_shared<ObjTypeFunda>(ObjTypeFunda::eInt)).toStr());
  EXPECT_EQ("fun((int, int), int)",
    ObjTypeFun(
      ObjTypeFun::createArgs(
        new ObjTypeFunda(ObjTypeFunda::eInt),
        new ObjTypeFunda(ObjTypeFunda::eInt)),
      make_shared<ObjTypeFunda>(ObjTypeFunda::eInt)
      ).toStr());
  EXPECT_EQ("fun((bool, int), bool)",
    ObjTypeFun(
      ObjTypeFun::createArgs(
        new ObjTypeFunda(ObjTypeFunda::eBool),
        new ObjTypeFunda(ObjTypeFunda::eInt)),
      make_shared<ObjTypeFunda>(ObjTypeFunda::eBool)
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
    removeQualifiers)) {
  EXPECT_EQ(
    ObjTypeFunda(ObjTypeFunda::eInt).qualifiers(),
    ObjTypeFunda(ObjTypeFunda::eInt, ObjType::eMutable).removeQualifiers(ObjType::eMutable).qualifiers());

  EXPECT_EQ(
    ObjTypeFunda(ObjTypeFunda::eInt).qualifiers(),
    ObjTypeFunda(ObjTypeFunda::eInt).removeQualifiers(ObjType::eMutable).qualifiers());
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
    ObjTypeFun(ObjTypeFun::createArgs(), make_shared<ObjTypeFunda>(ObjTypeFunda::eInt)));

  TEST_MATCH( "", ObjType::eFullMatch,
    ObjTypeFun(ObjTypeFun::createArgs(), make_shared<ObjTypeFunda>(ObjTypeFunda::eInt)),
    ObjTypeFun(ObjTypeFun::createArgs(), make_shared<ObjTypeFunda>(ObjTypeFunda::eInt)));

  TEST_MATCH( "", ObjType::eNoMatch,
    ObjTypeFun(
      ObjTypeFun::createArgs(new ObjTypeFunda(ObjTypeFunda::eInt)),
      make_shared<ObjTypeFunda>(ObjTypeFunda::eInt)),
    ObjTypeFun(
      ObjTypeFun::createArgs(),
      make_shared<ObjTypeFunda>(ObjTypeFunda::eInt)));

  TEST_MATCH( "Only one argument differs in type", ObjType::eNoMatch,
    ObjTypeFun(
      ObjTypeFun::createArgs(
        new ObjTypeFunda(ObjTypeFunda::eBool)),
      make_shared<ObjTypeFunda>(ObjTypeFunda::eInt)),
    ObjTypeFun(
      ObjTypeFun::createArgs(
        new ObjTypeFunda(ObjTypeFunda::eInt)),
      make_shared<ObjTypeFunda>(ObjTypeFunda::eInt)));

  TEST_MATCH( "Only return type differs", ObjType::eNoMatch,
    ObjTypeFun(
      ObjTypeFun::createArgs(
        new ObjTypeFunda(ObjTypeFunda::eInt)),
      make_shared<ObjTypeFunda>(ObjTypeFunda::eBool)),
    ObjTypeFun(
      ObjTypeFun::createArgs(
        new ObjTypeFunda(ObjTypeFunda::eInt)),
      make_shared<ObjTypeFunda>(ObjTypeFunda::eInt)));
}

TEST(ObjTypeTest, MAKE_TEST_NAME1(
    hasConstructor)) {

  // between fundamental types
  // =========================
  struct T {
    ObjTypeFunda::EType UUT;
    ObjTypeFunda::EType arg;
    bool hasConstructor;
  };
  vector<T> inputSpecs{
    // abstract types have no members
    {ObjTypeFunda::eVoid, ObjTypeFunda::eVoid, false},
    {ObjTypeFunda::eVoid, ObjTypeFunda::eNoreturn, false},
    {ObjTypeFunda::eVoid, ObjTypeFunda::eBool, false},
    {ObjTypeFunda::eVoid, ObjTypeFunda::eInt, false},
    {ObjTypeFunda::eVoid, ObjTypeFunda::eDouble, false},
    {ObjTypeFunda::eNoreturn, ObjTypeFunda::eVoid, false},
    {ObjTypeFunda::eNoreturn, ObjTypeFunda::eNoreturn, false},
    {ObjTypeFunda::eNoreturn, ObjTypeFunda::eBool, false},
    {ObjTypeFunda::eNoreturn, ObjTypeFunda::eInt, false},
    {ObjTypeFunda::eNoreturn, ObjTypeFunda::eDouble, false},

    // all concrete fundamental types
    // - can _not_ be constructed with an abstract type
    // - can be constructed with a concrete fundamental type
    {ObjTypeFunda::eBool, ObjTypeFunda::eVoid, false},
    {ObjTypeFunda::eBool, ObjTypeFunda::eNoreturn, false},
    {ObjTypeFunda::eBool, ObjTypeFunda::eBool, true},
    {ObjTypeFunda::eBool, ObjTypeFunda::eInt, true},
    {ObjTypeFunda::eBool, ObjTypeFunda::eDouble, true},
    {ObjTypeFunda::eInt, ObjTypeFunda::eVoid, false},
    {ObjTypeFunda::eInt, ObjTypeFunda::eNoreturn, false},
    {ObjTypeFunda::eInt, ObjTypeFunda::eBool, true},
    {ObjTypeFunda::eInt, ObjTypeFunda::eInt, true},
    {ObjTypeFunda::eInt, ObjTypeFunda::eDouble, true},
    {ObjTypeFunda::eDouble, ObjTypeFunda::eVoid, false},
    {ObjTypeFunda::eDouble, ObjTypeFunda::eNoreturn, false},
    {ObjTypeFunda::eDouble, ObjTypeFunda::eBool, true},
    {ObjTypeFunda::eDouble, ObjTypeFunda::eInt, true},
    {ObjTypeFunda::eDouble, ObjTypeFunda::eDouble, true}};

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
    ObjTypeFunda fundamentaltype{ObjTypeFunda::eInt};
    ObjTypeFun functiontype{
      ObjTypeFun::createArgs(),
      make_shared<ObjTypeFunda>(ObjTypeFunda::eInt)};
    EXPECT_FALSE( fundamentaltype.hasConstructor(functiontype) );
    EXPECT_FALSE( functiontype.hasConstructor(fundamentaltype) );
    EXPECT_FALSE( functiontype.hasConstructor(functiontype) );
  }
}

TEST(ObjTypeTest, MAKE_TEST_NAME1(
    clone)) {

  {
    ObjTypeFunda src{ ObjTypeFunda::eInt };
    unique_ptr<ObjType> clone{src.clone()};
    EXPECT_MATCHES_FULLY( src, *clone );
  }

  {
    ObjTypeFunda src{ ObjTypeFunda::eBool, ObjType::eMutable };
    unique_ptr<ObjType> clone{src.clone()};
    EXPECT_MATCHES_FULLY( src, *clone );
  }

  {
    ObjTypeFun src{
      ObjTypeFun::createArgs(),
      make_shared<ObjTypeFunda>(ObjTypeFunda::eInt) };
    unique_ptr<ObjType> clone{src.clone()};
    EXPECT_MATCHES_FULLY( src, *clone );
  }

  {
    ObjTypeFun src{
      ObjTypeFun::createArgs(
        new ObjTypeFunda(ObjTypeFunda::eInt),
        new ObjTypeFunda(ObjTypeFunda::eBool)),
        make_shared<ObjTypeFunda>(ObjTypeFunda::eInt) };
    unique_ptr<ObjType> clone{src.clone()};
    EXPECT_MATCHES_FULLY( src, *clone );
  }
}

TEST(ObjTypeTest, MAKE_TEST_NAME1(
    class_)) {

  struct T{
    bool m_isMember;
    ObjTypeFunda::EType m_fundaType;
    ObjType::EClass m_typeClass;
  };

  vector<T> inputs{
    {true,  ObjTypeFunda::eVoid, ObjType::eAbstract},
    {false, ObjTypeFunda::eVoid, ObjType::eScalar},
    {false, ObjTypeFunda::eVoid, ObjType::eArithmetic},
    {false, ObjTypeFunda::eVoid, ObjType::eIntegral},
    {false, ObjTypeFunda::eVoid, ObjType::eFloatingPoint},
    {false, ObjTypeFunda::eVoid, ObjType::eStoredAsIntegral},
    {false, ObjTypeFunda::eVoid, ObjType::eFunction},

    {true,  ObjTypeFunda::eNoreturn, ObjType::eAbstract},
    {false, ObjTypeFunda::eNoreturn, ObjType::eScalar},
    {false, ObjTypeFunda::eNoreturn, ObjType::eArithmetic},
    {false, ObjTypeFunda::eNoreturn, ObjType::eIntegral},
    {false, ObjTypeFunda::eNoreturn, ObjType::eFloatingPoint},
    {false, ObjTypeFunda::eNoreturn, ObjType::eStoredAsIntegral},
    {false, ObjTypeFunda::eNoreturn, ObjType::eFunction},

    {false, ObjTypeFunda::eBool, ObjType::eAbstract},
    {true,  ObjTypeFunda::eBool, ObjType::eScalar},
    {false, ObjTypeFunda::eBool, ObjType::eArithmetic},
    {false, ObjTypeFunda::eBool, ObjType::eIntegral},
    {false, ObjTypeFunda::eBool, ObjType::eFloatingPoint},
    {true,  ObjTypeFunda::eBool, ObjType::eStoredAsIntegral},
    {false, ObjTypeFunda::eBool, ObjType::eFunction},

    {false, ObjTypeFunda::eInt, ObjType::eAbstract},
    {true,  ObjTypeFunda::eInt, ObjType::eScalar},
    {true,  ObjTypeFunda::eInt, ObjType::eArithmetic},
    {true,  ObjTypeFunda::eInt, ObjType::eIntegral},
    {false, ObjTypeFunda::eInt, ObjType::eFloatingPoint},
    {true,  ObjTypeFunda::eInt, ObjType::eStoredAsIntegral},
    {false, ObjTypeFunda::eInt, ObjType::eFunction},

    {false, ObjTypeFunda::eDouble, ObjType::eAbstract},
    {true,  ObjTypeFunda::eDouble, ObjType::eScalar},
    {true,  ObjTypeFunda::eDouble, ObjType::eArithmetic},
    {false, ObjTypeFunda::eDouble, ObjType::eIntegral},
    {true,  ObjTypeFunda::eDouble, ObjType::eFloatingPoint},
    {false, ObjTypeFunda::eDouble, ObjType::eStoredAsIntegral},
    {false, ObjTypeFunda::eDouble, ObjType::eFunction}};

  for (const auto& i : inputs) {
    EXPECT_EQ( i.m_isMember, ObjTypeFunda(i.m_fundaType).is(i.m_typeClass))
      << "fundamental type: " << i.m_fundaType << "\n"
      << "type class: " << i.m_typeClass;
  }


  EXPECT_TRUE( ObjTypeFun(ObjTypeFun::createArgs()).is(ObjType::eFunction));
}

TEST(ObjTypeTest, MAKE_TEST_NAME1(
    size)) {
  EXPECT_EQ( -1, ObjTypeFunda(ObjTypeFunda::eVoid).size());
  EXPECT_EQ( -1, ObjTypeFunda(ObjTypeFunda::eNoreturn).size());
  EXPECT_EQ( 1, ObjTypeFunda(ObjTypeFunda::eBool).size());
  EXPECT_EQ( 32, ObjTypeFunda(ObjTypeFunda::eInt).size());
  EXPECT_EQ( 64, ObjTypeFunda(ObjTypeFunda::eDouble).size());

  EXPECT_EQ( -1, ObjTypeFun(ObjTypeFun::createArgs()).size());
}

TEST(ObjTypeTest, MAKE_TEST_NAME1(
    createDefaultAstValue)) {

  vector<ObjTypeFunda::EType> inputTypes = {
    ObjTypeFunda::eBool, ObjTypeFunda::eChar, ObjTypeFunda::eInt,
    ObjTypeFunda::eDouble};

  for (const auto type : inputTypes ) {
    unique_ptr<AstNumber> number{dynamic_cast<AstNumber*>(
        ObjTypeFunda(type).createDefaultAstValue())};
    EXPECT_TRUE( number != nullptr );
    EXPECT_EQ( 0.0, number->value());
    EXPECT_TRUE( number->objType().matchesFully(ObjTypeFunda(type)));
  }
}
