#ifndef TEST_H
#define TEST_H
#include "gtest/gtest.h"
#include <string>

/** Helper macros to make a test name, which really is a specification in
prose in the "given when then sothat/because" DSL. Using the testdox tool
(https://github.com/sensorflo/testdox) you can create a pretty printed summary
of all your specifications aka tests. The makefile target doc creates
out/specs.html using the tools testdox and asciidoc.*/
#define MAKE_TEST_NAME4(given, when, then, sothat_or_because) \
  given ## __ ## when ## __ ## then ## __ ## sothat_or_because
#define MAKE_TEST_NAME3(clause1, clause2, clause3)  \
  clause1 ## __ ## clause2 ## __ ## clause3
#define MAKE_TEST_NAME(clause1, clause2, clause3) MAKE_TEST_NAME3(clause1, clause2, clause3)
#define MAKE_TEST_NAME2(clause1, clause2) clause1 ## __ ## clause2 
#define MAKE_TEST_NAME1(clause1) clause1

#define MAKE_DISABLED_TEST_NAME4(given, when, then, sothat_or_because) \
  DISABLED_ ## given ## __ ## when ## __ ## then ## __ ## sothat_or_because
#define MAKE_DISABLED_TEST_NAME3(clause1, clause2, clause3)  \
  DISABLED_ ## clause1 ## __ ## clause2 ## __ ## clause3
#define MAKE_DISABLED_TEST_NAME(clause1, clause2, clause3) MAKE_DISABLED_TEST_NAME3(clause1, clause2, clause3)
#define MAKE_DISABLED_TEST_NAME2(clause1, clause2) DISABLED_ ## clause1 ## __ ## clause2 
#define MAKE_DISABLED_TEST_NAME1(clause1) DISABLED_ ## clause1

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
#define ABORT_MSG "test aborted due to failing assertion in test's environment\\n"

std::string amendSpec(const std::string& spec);

#endif
