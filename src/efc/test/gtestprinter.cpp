#include "gtestprinter.h"
#include <stdio.h>
using namespace testing;
using namespace testing::internal;
using namespace std;

namespace testing {
  enum GTestColor {
    COLOR_DEFAULT,
    COLOR_RED,
    COLOR_GREEN,
    COLOR_YELLOW
  };

  // Constants.

  // A test filter that matches everything.
  static const char kUniversalFilter[] = "*";

  // Formats a countable noun.  Depending on its quantity, either the
  // singular form or the plural form is used. e.g.
  //
  // FormatCountableNoun(1, "formula", "formuli") returns "1 formula".
  // FormatCountableNoun(5, "book", "books") returns "5 books".
  static string FormatCountableNoun(int count,
    const char * singular_form,
    const char * plural_form) {
    return internal::StreamableToString(count) + " " +
      (count == 1 ? singular_form : plural_form);
  }

  // Formats the count of tests.
  static string FormatTestCount(int test_count) {
    return FormatCountableNoun(test_count, "test", "tests");
  }

  // Formats the count of test cases.
  static string FormatTestCaseCount(int test_case_count) {
    return FormatCountableNoun(test_case_count, "test case", "test cases");
  }

  // Text printed in Google Test's text output and --gunit_list_tests
  // output to label the type parameter and value parameter for a test.
  static const char kTypeParamLabel[] = "TypeParam";

  // Converts a TestPartResult::Type enum to human-friendly string
  // representation.  Both kNonFatalFailure and kFatalFailure are translated
  // to "Failure", as the user usually doesn't care about the difference
  // between the two when viewing the test result.
  static const char * TestPartResultTypeToString(TestPartResult::Type type) {
    switch (type) {
    case TestPartResult::kSuccess:
      return "Success";

    case TestPartResult::kNonFatalFailure:
    case TestPartResult::kFatalFailure:
      return
        #ifdef _MSC_VER
          "error: "
        #endif
        "Failed to meet the following specification:\n";
    default:
      return "Unknown result type";
    }
  }

  namespace internal {
    // Returns true iff Google Test should use colors in the output.
    GTEST_API_ bool ShouldUseColor(bool stdout_is_tty);

    // Returns the ANSI color code for the given color.  COLOR_DEFAULT is
    // an invalid input.
    const char* GetAnsiColorCode(GTestColor color) {
      switch (color) {
      case COLOR_RED:     return "1";
      case COLOR_GREEN:   return "2";
      case COLOR_YELLOW:  return "3";
      default:            return NULL;
      };
    }

    void ColoredPrintf(GTestColor color, const char* fmt, ...) {
      va_list args;
      va_start(args, fmt);

      #if GTEST_OS_WINDOWS_MOBILE || GTEST_OS_SYMBIAN || GTEST_OS_ZOS || GTEST_OS_IOS
      const bool use_color = false;
      #else
      static const bool in_color_mode =
        ShouldUseColor(posix::IsATTY(posix::FileNo(stdout)) != 0);
      const bool use_color = in_color_mode && (color != COLOR_DEFAULT);
      #endif  // GTEST_OS_WINDOWS_MOBILE || GTEST_OS_SYMBIAN || GTEST_OS_ZOS
      // The '!= 0' comparison is necessary to satisfy MSVC 7.1.

      if (!use_color) {
        vprintf(fmt, args);
        va_end(args);
        return;
      }

      #if GTEST_OS_WINDOWS && !GTEST_OS_WINDOWS_MOBILE
      const HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);

      // Gets the current text color.
      CONSOLE_SCREEN_BUFFER_INFO buffer_info;
      GetConsoleScreenBufferInfo(stdout_handle, &buffer_info);
      const WORD old_color_attrs = buffer_info.wAttributes;

      // We need to flush the stream buffers into the console before each
      // SetConsoleTextAttribute call lest it affect the text that is already
      // printed but has not yet reached the console.
      fflush(stdout);
      SetConsoleTextAttribute(stdout_handle,
        GetColorAttribute(color) | FOREGROUND_INTENSITY);
      vprintf(fmt, args);

      fflush(stdout);
      // Restores the text color.
      SetConsoleTextAttribute(stdout_handle, old_color_attrs);
      #else
      printf("\033[0;3%sm", GetAnsiColorCode(color));
      vprintf(fmt, args);
      printf("\033[m");  // Resets the terminal to default.
      #endif  // GTEST_OS_WINDOWS && !GTEST_OS_WINDOWS_MOBILE
      va_end(args);
    };

    // Prints a TestPartResult to an string.
    static string PrintTestPartResultToString(
      const TestPartResult& test_part_result,
      const string& test_name) {

      FILE* stream = popen((string("testdox -t -n ") + test_name).c_str() , "r");
      assert(stream);
      string prettyTestName;
      while (!feof(stream)) {
        char buf[1024];
        if (fgets(buf, sizeof(buf), stream) != NULL) {
          prettyTestName += buf;
        }
      }  
      pclose(stream);

      return (Message()
        << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n"
        << internal::FormatFileLocation(test_part_result.file_name(),
          test_part_result.line_number())
        << " " << TestPartResultTypeToString(test_part_result.type())
        << prettyTestName << "\n"
        << test_part_result.message()
        << "\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<"
        ).GetString();
    }

    // Prints a TestPartResult.
    static void PrintTestPartResult(const TestPartResult& test_part_result,
      const string& testName) {
      const string& result =
        PrintTestPartResultToString(test_part_result, testName);
      printf("%s\n", result.c_str());
      fflush(stdout);
      // If the test program runs in Visual Studio or a debugger, the
      // following statements add the test part result message to the Output
      // window such that the user can double-click on it to jump to the
      // corresponding source code location; otherwise they do nothing.
      #if GTEST_OS_WINDOWS && !GTEST_OS_WINDOWS_MOBILE
      // We don't call OutputDebugString*() on Windows Mobile, as printing
      // to stdout is done by OutputDebugString() there already - we don't
      // want the same message printed twice.
      ::OutputDebugStringA(result.c_str());
      ::OutputDebugStringA("\n");
      #endif
    }

    void PrintFullTestCommentIfPresent(const TestInfo& test_info);
  }
}

// Fired before each iteration of tests starts.
void GTestPrinter::OnTestIterationStart(
  const UnitTest& unit_test, int iteration) {
  if (GTEST_FLAG(repeat) != 1)
    printf("\nRepeating all tests (iteration %d) . . .\n\n", iteration + 1);

  const char* const filter = GTEST_FLAG(filter).c_str();

  // Prints the filter if it's not *.  This reminds the user that some
  // tests may be skipped.
  if (!String::CStringEquals(filter, kUniversalFilter)) {
    ColoredPrintf(COLOR_YELLOW,
      "Note: %s filter = %s\n", GTEST_NAME_, filter);
  }

  // if (internal::ShouldShard(kTestTotalShards, kTestShardIndex, false)) {
  //   const Int32 shard_index = Int32FromEnvOrDie(kTestShardIndex, -1);
  //   ColoredPrintf(COLOR_YELLOW,
  //     "Note: This is test shard %d of %s.\n",
  //     static_cast<int>(shard_index) + 1,
  //     internal::posix::GetEnv(kTestTotalShards));
  // }

  if (GTEST_FLAG(shuffle)) {
    ColoredPrintf(COLOR_YELLOW,
      "Note: Randomizing tests' orders with a seed of %d .\n",
      unit_test.random_seed());
  }

  ColoredPrintf(COLOR_GREEN,  "[==========] ");
  printf("Running %s from %s.\n",
    FormatTestCount(unit_test.test_to_run_count()).c_str(),
    FormatTestCaseCount(unit_test.test_case_to_run_count()).c_str());
  fflush(stdout);
}

void GTestPrinter::OnEnvironmentsSetUpStart(
  const UnitTest& /*unit_test*/) {
  ColoredPrintf(COLOR_GREEN,  "[----------] ");
  printf("Global test environment set-up.\n");
  fflush(stdout);
}

void GTestPrinter::OnTestCaseStart(const TestCase& test_case) {
  const string counts =
    FormatCountableNoun(test_case.test_to_run_count(), "test", "tests");
  ColoredPrintf(COLOR_GREEN, "[----------] ");
  printf("%s from %s", counts.c_str(), test_case.name());
  if (test_case.type_param() == NULL) {
    printf("\n");
  } else {
    printf(", where %s = %s\n", kTypeParamLabel, test_case.type_param());
  }
  fflush(stdout);
}

void GTestPrinter::OnTestStart(const TestInfo& test_info) {
  ColoredPrintf(COLOR_GREEN,  "[ RUN      ] ");
  PrintTestName(test_info.test_case_name(), test_info.name());
  printf("\n");
  fflush(stdout);
  m_testName = test_info.name();
}

// Called after an assertion failure.
void GTestPrinter::OnTestPartResult(
  const TestPartResult& result) {
  // If the test part succeeded, we don't need to do anything.
  if (result.type() == TestPartResult::kSuccess)
    return;

  // Print failure message from the assertion (e.g. expected this and got that).
  PrintTestPartResult(result, m_testName);
  fflush(stdout);
}

void GTestPrinter::OnTestEnd(const TestInfo& test_info) {
  if (test_info.result()->Passed()) {
    ColoredPrintf(COLOR_GREEN, "[       OK ] ");
  } else {
    ColoredPrintf(COLOR_RED, "[  FAILED  ] ");
  }
  PrintTestName(test_info.test_case_name(), test_info.name());
  if (test_info.result()->Failed())
    PrintFullTestCommentIfPresent(test_info);

  if (GTEST_FLAG(print_time)) {
    printf(" (%s ms)\n", internal::StreamableToString(
        test_info.result()->elapsed_time()).c_str());
  } else {
    printf("\n");
  }
  fflush(stdout);
}

void GTestPrinter::OnTestCaseEnd(const TestCase& test_case) {
  if (!GTEST_FLAG(print_time)) return;

  const string counts =
    FormatCountableNoun(test_case.test_to_run_count(), "test", "tests");
  ColoredPrintf(COLOR_GREEN, "[----------] ");
  printf("%s from %s (%s ms total)\n\n",
    counts.c_str(), test_case.name(),
    internal::StreamableToString(test_case.elapsed_time()).c_str());
  fflush(stdout);
}

void GTestPrinter::OnEnvironmentsTearDownStart(
  const UnitTest& /*unit_test*/) {
  ColoredPrintf(COLOR_GREEN,  "[----------] ");
  printf("Global test environment tear-down\n");
  fflush(stdout);
}

// Internal helper for printing the list of failed tests.
void GTestPrinter::PrintFailedTests(const UnitTest& unit_test) {
  const int failed_test_count = unit_test.failed_test_count();
  if (failed_test_count == 0) {
    return;
  }

  for (int i = 0; i < unit_test.total_test_case_count(); ++i) {
    const TestCase& test_case = *unit_test.GetTestCase(i);
    if (!test_case.should_run() || (test_case.failed_test_count() == 0)) {
      continue;
    }
    for (int j = 0; j < test_case.total_test_count(); ++j) {
      const TestInfo& test_info = *test_case.GetTestInfo(j);
      if (!test_info.should_run() || test_info.result()->Passed()) {
        continue;
      }
      ColoredPrintf(COLOR_RED, "[  FAILED  ] ");
      printf("%s.%s", test_case.name(), test_info.name());
      PrintFullTestCommentIfPresent(test_info);
      printf("\n");
    }
  }
}

void GTestPrinter::OnTestIterationEnd(const UnitTest& unit_test,
  int /*iteration*/) {
  ColoredPrintf(COLOR_GREEN,  "[==========] ");
  printf("%s from %s ran.",
    FormatTestCount(unit_test.test_to_run_count()).c_str(),
    FormatTestCaseCount(unit_test.test_case_to_run_count()).c_str());
  if (GTEST_FLAG(print_time)) {
    printf(" (%s ms total)",
      internal::StreamableToString(unit_test.elapsed_time()).c_str());
  }
  printf("\n");
  ColoredPrintf(COLOR_GREEN,  "[  PASSED  ] ");
  printf("%s.\n", FormatTestCount(unit_test.successful_test_count()).c_str());

  int num_failures = unit_test.failed_test_count();
  if (!unit_test.Passed()) {
    const int failed_test_count = unit_test.failed_test_count();
    ColoredPrintf(COLOR_RED,  "[  FAILED  ] ");
    printf("%s, listed below:\n", FormatTestCount(failed_test_count).c_str());
    PrintFailedTests(unit_test);
    printf("\n%2d FAILED %s\n", num_failures,
      num_failures == 1 ? "TEST" : "TESTS");
  }

  int num_disabled = unit_test.disabled_test_count();
  if (num_disabled && !GTEST_FLAG(also_run_disabled_tests)) {
    if (!num_failures) {
      printf("\n");  // Add a spacer if no FAILURE banner is displayed.
    }
    ColoredPrintf(COLOR_YELLOW,
      "  YOU HAVE %d DISABLED %s\n\n",
      num_disabled,
      num_disabled == 1 ? "TEST" : "TESTS");
  }
  // Ensure that Google Test output is printed before, e.g., heapchecker output.
  fflush(stdout);
}


