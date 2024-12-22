#pragma once

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <string>

//////////////////////////////////////////////////////////////////////////
/// A matcher analogous to testing::StrEq, but
/// - Prints the expected string exactly below the actual, so the two strings are easily comparable. The strings are
///   printed using Google test's printer, which adds surrounding quotes and which uses escape sequences for some
///   characters such as tabs or newlines.
/// - If either string contains newlines:
///   - Additionally prints the strings literally, each starting on a new line. That is convenient for example when
///     the strings are file contents.
///   - Prints a unified diff, just as EXPECT_EQ with string arguments does.
///
/// See also @ref print_to_string for alternate solutions when the default google test output for strings is not
/// helpful enough.
///
/// We actually would like to use std::string_view opposed to std::string, but due to a bug in google test
/// (https://github.com/google/googletest/issues/4357), that is currently apparently not possible. You might want to use
/// EXPECT_LITERAL_STR_EQ as a workaround if the argument or the value of the matcher is a string_view.
::testing::Matcher<const std::wstring&> LiteralStrEq(std::wstring expected);

//////////////////////////////////////////////////////////////////////////
/// Use in place of EXPECT_THAT(a, LiteralStrEq(b)) if a or b is a string_view. Intended only as a workaround for the
/// bug described in LiteralStrEq.
#define EXPECT_LITERAL_STR_EQ(actual, expected) \
	EXPECT_THAT(std::wstring{actual}, LiteralStrEq(std::wstring{expected}))

#define ASSERT_LITERAL_STR_EQ(actual, expected) \
	ASSERT_THAT(std::wstring{actual}, LiteralStrEq(std::wstring{expected}))
