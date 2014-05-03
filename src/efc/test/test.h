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

#endif
