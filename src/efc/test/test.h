#ifndef TEST_H
#define TEST_H
#include "gtest/gtest.h"

/** make test name */
#define MAKE_TEST_NAME4(given, when, then, sothat_or_because) \
  given ## __ ## when ## __ ## then ## __ ## sothat_or_because
#define MAKE_TEST_NAME3(clause1, clause2, clause3)  \
  clause1 ## __ ## clause2 ## __ ## clause3
#define MAKE_TEST_NAME(clause1, clause2, clause3) MAKE_TEST_NAME3(clause1, clause2, clause3)
#define MAKE_TEST_NAME2(clause1, clause2) clause1 ## __ ## clause2 
#define MAKE_TEST_NAME1(clause1) clause1

// The ENV_ASSERT family of macros assert that the test environment is as
// expected. It is technically equivalent to the ASSERT family of macros. The
// difference is that the semantic of ASSERT is to be in the verify section of
// the test, i.e. its about wether the test succeeded or failed. When an
// ENV_ASSERT fails, then it is unknown whether the test succeeded or failed,
// the test was aborted.
#define ENV_ASSERT_EQ(expected, actual) ASSERT_EQ(expected, actual)
#define ENV_ASSERT_NE(expected, actual) ASSERT_NE(expected, actual)
#define ENV_ASSERT_TRUE(actual) ASSERT_TRUE(actual)
#define ENV_ASSERT_FALSE(actual) ASSERT_FALSE(actual)

#endif
