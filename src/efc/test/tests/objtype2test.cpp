#include "test.h"
#include "../objtype2.h"
#include "../env.h"
#include <string>
#include <sstream>

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


TEST(ObjType2Test, MAKE_TEST_NAME1(printTo)) {
  const auto toStr = [](const AstObjTypeRef& UUT) -> string {
    stringstream ss;
    ss << UUT;
    return ss.str();
  };

  // fundamental types
  EXPECT_EQ("void",     toStr(AstObjTypeRef(ObjType2::eVoid)));
  EXPECT_EQ("noreturn", toStr(AstObjTypeRef(ObjType2::eNoreturn)));
  EXPECT_EQ("char",     toStr(AstObjTypeRef(ObjType2::eChar)));
  EXPECT_EQ("int",      toStr(AstObjTypeRef(ObjType2::eInt)));
  EXPECT_EQ("bool",     toStr(AstObjTypeRef(ObjType2::eBool)));
  EXPECT_EQ("double",   toStr(AstObjTypeRef(ObjType2::eDouble)));
  EXPECT_EQ("Nullptr",  toStr(AstObjTypeRef(ObjType2::eNullptr)));

  // adorned with qualifiers
  EXPECT_EQ("mut-int", toStr(AstObjTypeRef(ObjType2::eInt, ObjType2::eMutable)));

  // pointer type
  EXPECT_EQ("raw*int",
    toStr(
      AstObjTypeRef(
        ObjType2::ptrTypeName,
        ObjType2::eNoQualifier,
        new AstObjTypeRef(ObjType2::eInt))));
}

TEST(ObjType2Test, MAKE_TEST_NAME1(match)) {

  // fundamental types
  TEST_MATCH("type and qualifier fully match", ObjType2::eFullMatch,
    AstObjTypeRef(ObjType2::eInt),
    AstObjTypeRef(ObjType2::eInt));
  TEST_MATCH("type mismatch", ObjType2::eNoMatch,
    AstObjTypeRef(ObjType2::eInt),
    AstObjTypeRef(ObjType2::eDouble));
  TEST_MATCH("type matches, but dst has weaker qualifiers", ObjType2::eMatchButAllQualifiersAreWeaker,
    AstObjTypeRef(ObjType2::eInt),
    AstObjTypeRef(ObjType2::eInt, ObjType2::eMutable));
  TEST_MATCH("type matches, but dst has stronger qualifiers", ObjType2::eMatchButAnyQualifierIsStronger,
    AstObjTypeRef(ObjType2::eInt, ObjType2::eMutable),
    AstObjTypeRef(ObjType2::eInt));

  // pointer
  AstObjTypeRef uut_raw_ptr_1_to_int(ObjType2::ptrTypeName, ObjType2::eNoQualifier,
    new AstObjTypeRef(ObjType2::eInt));
  AstObjTypeRef uut_raw_ptr_2_to_int(ObjType2::ptrTypeName, ObjType2::eNoQualifier,
    new AstObjTypeRef(ObjType2::eInt));
  AstObjTypeRef uut_raw_ptr_3_to_double(ObjType2::ptrTypeName, ObjType2::eNoQualifier,
    new AstObjTypeRef(ObjType2::eDouble));
  TEST_MATCH("type and qualifier fully match", ObjType2::eFullMatch,
    uut_raw_ptr_1_to_int, uut_raw_ptr_2_to_int);
  TEST_MATCH("type mismatch", ObjType2::eNoMatch,
    uut_raw_ptr_1_to_int, uut_raw_ptr_3_to_double);

  // functions
  // its not important to match two funs yet (will be needed for pointer to fun).
  //   Yet not needed since fun objects cant be assigned to each other (because inmutable)
  //   Also function pointers cannot yet be used to call functions
}

TEST(ObjType2Test, MAKE_TEST_NAME1(
    hasConstructor)) {

  // between fundamental types
  // =========================
  {
    struct T {
      ObjType2::EFundaType UUT;
      ObjType2::EFundaType arg;
      bool hasConstructor;
    };
    vector<T> inputSpecs{
      // abstract types have no members
      {ObjType2::eVoid, ObjType2::eVoid, false},
      {ObjType2::eVoid, ObjType2::eNoreturn, false},
      {ObjType2::eVoid, ObjType2::eBool, false},
      {ObjType2::eVoid, ObjType2::eChar, false},
      {ObjType2::eVoid, ObjType2::eInt, false},
      {ObjType2::eVoid, ObjType2::eDouble, false},
      {ObjType2::eNoreturn, ObjType2::eVoid, false},
      {ObjType2::eNoreturn, ObjType2::eNoreturn, false},
      {ObjType2::eNoreturn, ObjType2::eBool, false},
      {ObjType2::eNoreturn, ObjType2::eChar, false},
      {ObjType2::eNoreturn, ObjType2::eInt, false},
      {ObjType2::eNoreturn, ObjType2::eDouble, false},
        // TODO: what about Nullptr?

        // all concrete fundamental types
        // - can _not_ be constructed with an abstract type
        // - can be constructed with a concrete fundamental type
      {ObjType2::eBool, ObjType2::eVoid, false},
      {ObjType2::eBool, ObjType2::eNoreturn, false},
      {ObjType2::eBool, ObjType2::eBool, true},
      {ObjType2::eBool, ObjType2::eChar, true},
      {ObjType2::eBool, ObjType2::eInt, true},
      {ObjType2::eBool, ObjType2::eDouble, true},
      {ObjType2::eChar, ObjType2::eVoid, false},
      {ObjType2::eChar, ObjType2::eNoreturn, false},
      {ObjType2::eChar, ObjType2::eBool, true},
      {ObjType2::eChar, ObjType2::eChar, true},
      {ObjType2::eChar, ObjType2::eInt, true},
      {ObjType2::eChar, ObjType2::eDouble, true},
      {ObjType2::eInt, ObjType2::eVoid, false},
      {ObjType2::eInt, ObjType2::eNoreturn, false},
      {ObjType2::eInt, ObjType2::eBool, true},
      {ObjType2::eInt, ObjType2::eChar, true},
      {ObjType2::eInt, ObjType2::eInt, true},
      {ObjType2::eInt, ObjType2::eDouble, true},
      {ObjType2::eInt, ObjType2::eDouble, true},
      {ObjType2::eDouble, ObjType2::eVoid, false},
      {ObjType2::eDouble, ObjType2::eNoreturn, false},
      {ObjType2::eDouble, ObjType2::eBool, true},
      {ObjType2::eDouble, ObjType2::eChar, true},
      {ObjType2::eDouble, ObjType2::eInt, true},
      {ObjType2::eDouble, ObjType2::eDouble, true}};

    for ( const auto& inputSpec : inputSpecs ) {
      AstObjTypeRef UUT{inputSpec.UUT};
      AstObjTypeRef arg{inputSpec.arg};
      EXPECT_EQ( inputSpec.hasConstructor, UUT.hasConstructor(arg))
        << "UUT: " << UUT << "\n"
        << "arg: " << arg << "\n";
    }
  }

  // !!!!! what about qualifiers ??????!!!!!!


  // between fundamental types and pointer
  // =====================================
  {
    struct T {
      ObjType2::EFundaType arg;
      bool hasConstructor; // pointer has constructor for arg
      bool hasConstructorInverseDirection; // arg has constructor for pointer
    };
    vector<T> inputSpecs{
      {ObjType2::eVoid,     false, false},
      {ObjType2::eNoreturn, false, false},
      {ObjType2::eBool,     true , true},
      {ObjType2::eChar,     true , true},
      {ObjType2::eInt,      true , true},
      {ObjType2::eDouble,   true , true}};

    AstObjTypeRef UUT{ObjType2::ptrTypeName, ObjType2::eNoQualifier,
        new AstObjTypeRef(ObjType2::eVoid)};

    for ( const auto& inputSpec : inputSpecs ) {
      AstObjTypeRef arg{inputSpec.arg};
      EXPECT_EQ( inputSpec.hasConstructor, UUT.hasConstructor(arg))
        << "UUT: " << UUT << "\n"
        << "arg: " << arg << "\n";
      EXPECT_EQ( inputSpec.hasConstructorInverseDirection, arg.hasConstructor(UUT))
        << "arg: " << arg << "\n"
        << "UUT: " << UUT << "\n";
    }
  }

  // between pointer
  // ================================
  // same types
  {
    AstObjTypeRef UUT{ObjType2::ptrTypeName, ObjType2::eNoQualifier,
        new AstObjTypeRef(ObjType2::eVoid)};
    AstObjTypeRef arg{ObjType2::ptrTypeName, ObjType2::eNoQualifier,
        new AstObjTypeRef(ObjType2::eVoid)};
    EXPECT_TRUE( UUT.hasConstructor(arg))
      << "arg: " << arg << "\n"
      << "UUT: " << UUT << "\n";
  }
  // pointee types differ
  {
    AstObjTypeRef UUT{ObjType2::ptrTypeName, ObjType2::eNoQualifier,
        new AstObjTypeRef(ObjType2::eVoid)};
    AstObjTypeRef arg{ObjType2::ptrTypeName, ObjType2::eNoQualifier,
        new AstObjTypeRef(ObjType2::eDouble)};
    EXPECT_FALSE( UUT.hasConstructor(arg))
      << "arg: " << arg << "\n"
      << "UUT: " << UUT << "\n";
  }
}
