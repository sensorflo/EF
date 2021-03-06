#pragma once

#include "../access.h"
#include "../errorhandler.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <memory>
#include <string>
#include <tuple>

class AstNode;
class AstObject;
class ErrorHandler;
class Env;
namespace llvm {
class Module;
}

/** Helper macros to make a test name, which really is a specification in
prose in the "given when then sothat/because" DSL. Using the testdox tool
(https://github.com/sensorflo/testdox) you can create a pretty printed summary
of all your specifications aka tests. The makefile target doc creates
out/specs.html using the tools testdox and asciidoc.*/
#define MAKE_TEST_NAME4(given, when, then, sothat_or_because) \
  given##__##when##__##then##__##sothat_or_because
#define MAKE_TEST_NAME3(clause1, clause2, clause3) \
  clause1##__##clause2##__##clause3
#define MAKE_TEST_NAME(clause1, clause2, clause3) \
  MAKE_TEST_NAME3(clause1, clause2, clause3)
#define MAKE_TEST_NAME2(clause1, clause2) clause1##__##clause2
#define MAKE_TEST_NAME1(clause1) clause1

#define MAKE_DISABLED_TEST_NAME4(given, when, then, sothat_or_because) \
  DISABLED_##given##__##when##__##then##__##sothat_or_because
#define MAKE_DISABLED_TEST_NAME3(clause1, clause2, clause3) \
  DISABLED_##clause1##__##clause2##__##clause3
#define MAKE_DISABLED_TEST_NAME(clause1, clause2, clause3) \
  MAKE_DISABLED_TEST_NAME3(clause1, clause2, clause3)
#define MAKE_DISABLED_TEST_NAME2(clause1, clause2) \
  DISABLED_##clause1##__##clause2
#define MAKE_DISABLED_TEST_NAME1(clause1) DISABLED_##clause1

// The ENV_ASSERT family of macros assert that the test environment is as
// expected. It is technically equivalent to the ASSERT family of macros. The
// difference is that the semantic of ASSERT is to be in the verify section of
// the test, i.e. its about wether the test succeeded or failed. When an
// ENV_ASSERT fails, then it is unknown whether the test succeeded or failed,
// the test was aborted. When a test fails, than the defect is most likely in
// what was exercised of the unit under test. When a test aborts, the defect
// is somewhere else, and given you have a unit test for everything, there
// should be at least one other test which fails (not aborts), and that
// failing test will lead you to the cause of the defect.
#define ENV_ASSERT_EQ(expected, actual) ASSERT_EQ(expected, actual) << ABORT_MSG
#define ENV_ASSERT_NE(expected, actual) ASSERT_NE(expected, actual) << ABORT_MSG
#define ENV_ASSERT_TRUE(actual) ASSERT_TRUE(actual) << ABORT_MSG
#define ENV_ASSERT_FALSE(actual) ASSERT_FALSE(actual) << ABORT_MSG
#define ABORT_MSG \
  "test aborted due to failing assertion in test's environment\\n"

std::string amendSpec(const std::string& spec);
std::string amendAst(const AstNode* ast);
std::string amendAst(const std::unique_ptr<AstNode>& ast);
std::string amendAst(const std::unique_ptr<AstObject>& ast);
std::string amend(llvm::Module* module);
std::string amend(const std::unique_ptr<llvm::Module>& module);
std::string amend(const ErrorHandler& errorHandler);
std::string amend(const Env& env);

#define EXPECT_MATCHES_FULLY(expected_obj_type, actual_obj_type) \
  EXPECT_PRED2(ObjType::matchesFully_, expected_obj_type, actual_obj_type)

#define EXPECT_MATCHES_SAUF_QUALIFIERS(expected_obj_type, actual_obj_type) \
  EXPECT_PRED2(                                                            \
    ObjType::matchesExceptQualifiers, expected_obj_type, actual_obj_type)

template<typename T>
std::basic_ostream<char>& operator<<(
  std::basic_ostream<char>& os, const std::shared_ptr<T>& sp) {
  os << "{" << sp.get();
  if (sp) { os << "->" << *sp; }
  os << "}";
  return os;
}

template<typename T>
std::basic_ostream<char>& operator<<(
  std::basic_ostream<char>& os, const std::unique_ptr<T>& up) {
  os << "{" << up.get();
  if (up) { os << "->" << *up; }
  os << "}";
  return os;
}

/** Creates a new'ed AST tree which imposes the given access on obj. It is
assumed that the ObjType of obj is ObjTypeFunda::eInt. The ObjType of the
returned AstObject is ObjTypeFunda::eInt */
AstObject* createAccessTo(AstObject* obj, Access access);

/** as the above, whereas obj is 'new AstSymbol(symbolName)' */
AstObject* createAccessTo(const std::string& symbolName, Access access);

/** see tryCatch */
struct TryCatchResult {
  bool m_foreignCatches = false;
  std::string m_excptionWhat;
};

/** Execute f in in a try block and catch any exception. If its of type 'const
BuildError&', set m_foreignCatches of the result value to false. As always you
can access the error via the error handler. Otherwise, set it to true. If its of
type 'const exception&' or 'const exception*', additionally set m_excptionWhat
to the return value of the what() method of the catched exception. */
TryCatchResult tryCatch(const std::function<void()>& f);

/** Verifies with gtest assertions that errorHandler contains either no error if
expectedErrorNo is eNone, or exactly one error which is described by
(expectedErrorNo, expectedMsgParam1, expectedMsgParam2, expectedMsgParam3). Also
verifies that foreignCatches is false. Set foreignCatches to true if something
other than BuildError was thrown (i.e. catched) and exceptionWhat to the result
of its std::exception::what() if available. 'details' is appended to the output
if an gtest assertion fires. */
void verifyErrorHandlerHasExpectedError(const ErrorHandler& errorHandler,
  const std::string& details, bool foreignCatches,
  const std::string& exceptionWhat, Error::No expectedErrorNo = Error::eNone,
  const std::string& expectedMsgParam1 = "",
  const std::string& expectedMsgParam2 = "",
  const std::string& expectedMsgParam3 = "");
