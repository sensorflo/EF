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
  EXPECT_EQ("void",     toStr(AstObjTypeRef(ObjTypeFunda::eVoid)));
  EXPECT_EQ("noreturn", toStr(AstObjTypeRef(ObjTypeFunda::eNoreturn)));
  EXPECT_EQ("char",     toStr(AstObjTypeRef(ObjTypeFunda::eChar)));
  EXPECT_EQ("int",      toStr(AstObjTypeRef(ObjTypeFunda::eInt)));
  EXPECT_EQ("bool",     toStr(AstObjTypeRef(ObjTypeFunda::eBool)));
  EXPECT_EQ("double",   toStr(AstObjTypeRef(ObjTypeFunda::eDouble)));
  EXPECT_EQ("Nullptr",  toStr(AstObjTypeRef(ObjTypeFunda::eNullptr)));

  // adorned with qualifiers
  EXPECT_EQ("mut-int", toStr(AstObjTypeRef(ObjTypeFunda::eInt, ObjType2::eMutable)));

  // pointer type
  EXPECT_EQ("raw*int",
    toStr(
      AstObjTypeRef(
        ObjTypeFunda::ePointer,
        ObjType2::eNoQualifier,
        new AstObjTypeRef(ObjTypeFunda::eInt))));
}

TEST(ObjType2Test, MAKE_TEST_NAME1(match)) {

  // fundamental types
  TEST_MATCH("type and qualifier fully match", ObjType2::eFullMatch,
    AstObjTypeRef(ObjTypeFunda::eInt),
    AstObjTypeRef(ObjTypeFunda::eInt));
  TEST_MATCH("type mismatch", ObjType2::eNoMatch,
    AstObjTypeRef(ObjTypeFunda::eInt),
    AstObjTypeRef(ObjTypeFunda::eDouble));
  TEST_MATCH("type matches, but dst has weaker qualifiers", ObjType2::eMatchButAllQualifiersAreWeaker,
    AstObjTypeRef(ObjTypeFunda::eInt),
    AstObjTypeRef(ObjTypeFunda::eInt, ObjType2::eMutable));
  TEST_MATCH("type matches, but dst has stronger qualifiers", ObjType2::eMatchButAnyQualifierIsStronger,
    AstObjTypeRef(ObjTypeFunda::eInt, ObjType2::eMutable),
    AstObjTypeRef(ObjTypeFunda::eInt));

  // pointer
  AstObjTypeRef uut_raw_ptr_1_to_int(ObjTypeFunda::ePointer, ObjType2::eNoQualifier,
    new AstObjTypeRef(ObjTypeFunda::eInt));
  AstObjTypeRef uut_raw_ptr_2_to_int(ObjTypeFunda::ePointer, ObjType2::eNoQualifier,
    new AstObjTypeRef(ObjTypeFunda::eInt));
  AstObjTypeRef uut_raw_ptr_3_to_double(ObjTypeFunda::ePointer, ObjType2::eNoQualifier,
    new AstObjTypeRef(ObjTypeFunda::eDouble));
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
      ObjTypeFunda::EType UUT;
      ObjTypeFunda::EType arg;
      bool hasConstructor;
    };
    vector<T> inputSpecs{
      // abstract types have no members
      {ObjTypeFunda::eVoid, ObjTypeFunda::eVoid, false},
      {ObjTypeFunda::eVoid, ObjTypeFunda::eNoreturn, false},
      {ObjTypeFunda::eVoid, ObjTypeFunda::eBool, false},
      {ObjTypeFunda::eVoid, ObjTypeFunda::eChar, false},
      {ObjTypeFunda::eVoid, ObjTypeFunda::eInt, false},
      {ObjTypeFunda::eVoid, ObjTypeFunda::eDouble, false},
      {ObjTypeFunda::eNoreturn, ObjTypeFunda::eVoid, false},
      {ObjTypeFunda::eNoreturn, ObjTypeFunda::eNoreturn, false},
      {ObjTypeFunda::eNoreturn, ObjTypeFunda::eBool, false},
      {ObjTypeFunda::eNoreturn, ObjTypeFunda::eChar, false},
      {ObjTypeFunda::eNoreturn, ObjTypeFunda::eInt, false},
      {ObjTypeFunda::eNoreturn, ObjTypeFunda::eDouble, false},
        // TODO: what about Nullptr?

        // all concrete fundamental types
        // - can _not_ be constructed with an abstract type
        // - can be constructed with a concrete fundamental type
      {ObjTypeFunda::eBool, ObjTypeFunda::eVoid, false},
      {ObjTypeFunda::eBool, ObjTypeFunda::eNoreturn, false},
      {ObjTypeFunda::eBool, ObjTypeFunda::eBool, true},
      {ObjTypeFunda::eBool, ObjTypeFunda::eChar, true},
      {ObjTypeFunda::eBool, ObjTypeFunda::eInt, true},
      {ObjTypeFunda::eBool, ObjTypeFunda::eDouble, true},
      {ObjTypeFunda::eChar, ObjTypeFunda::eVoid, false},
      {ObjTypeFunda::eChar, ObjTypeFunda::eNoreturn, false},
      {ObjTypeFunda::eChar, ObjTypeFunda::eBool, true},
      {ObjTypeFunda::eChar, ObjTypeFunda::eChar, true},
      {ObjTypeFunda::eChar, ObjTypeFunda::eInt, true},
      {ObjTypeFunda::eChar, ObjTypeFunda::eDouble, true},
      {ObjTypeFunda::eInt, ObjTypeFunda::eVoid, false},
      {ObjTypeFunda::eInt, ObjTypeFunda::eNoreturn, false},
      {ObjTypeFunda::eInt, ObjTypeFunda::eBool, true},
      {ObjTypeFunda::eInt, ObjTypeFunda::eChar, true},
      {ObjTypeFunda::eInt, ObjTypeFunda::eInt, true},
      {ObjTypeFunda::eInt, ObjTypeFunda::eDouble, true},
      {ObjTypeFunda::eInt, ObjTypeFunda::eDouble, true},
      {ObjTypeFunda::eDouble, ObjTypeFunda::eVoid, false},
      {ObjTypeFunda::eDouble, ObjTypeFunda::eNoreturn, false},
      {ObjTypeFunda::eDouble, ObjTypeFunda::eBool, true},
      {ObjTypeFunda::eDouble, ObjTypeFunda::eChar, true},
      {ObjTypeFunda::eDouble, ObjTypeFunda::eInt, true},
      {ObjTypeFunda::eDouble, ObjTypeFunda::eDouble, true}};

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
      ObjTypeFunda::EType arg;
      bool hasConstructor; // pointer has constructor for arg
      bool hasConstructorInverseDirection; // arg has constructor for pointer
    };
    vector<T> inputSpecs{
      {ObjTypeFunda::eVoid,     false, false},
      {ObjTypeFunda::eNoreturn, false, false},
      {ObjTypeFunda::eBool,     true , true},
      {ObjTypeFunda::eChar,     true , true},
      {ObjTypeFunda::eInt,      true , true},
      {ObjTypeFunda::eDouble,   true , true}};

    AstObjTypeRef UUT{ObjTypeFunda::ePointer, ObjType2::eNoQualifier,
        new AstObjTypeRef(ObjTypeFunda::eVoid)};

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
    AstObjTypeRef UUT{ObjTypeFunda::ePointer, ObjType2::eNoQualifier,
        new AstObjTypeRef(ObjTypeFunda::eVoid)};
    AstObjTypeRef arg{ObjTypeFunda::ePointer, ObjType2::eNoQualifier,
        new AstObjTypeRef(ObjTypeFunda::eVoid)};
    EXPECT_TRUE( UUT.hasConstructor(arg))
      << "arg: " << arg << "\n"
      << "UUT: " << UUT << "\n";
  }
  // pointee types differ
  {
    AstObjTypeRef UUT{ObjTypeFunda::ePointer, ObjType2::eNoQualifier,
        new AstObjTypeRef(ObjTypeFunda::eVoid)};
    AstObjTypeRef arg{ObjTypeFunda::ePointer, ObjType2::eNoQualifier,
        new AstObjTypeRef(ObjTypeFunda::eDouble)};
    EXPECT_FALSE( UUT.hasConstructor(arg))
      << "arg: " << arg << "\n"
      << "UUT: " << UUT << "\n";
  }
}
