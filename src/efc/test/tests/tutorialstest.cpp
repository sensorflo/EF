#include "test.h"

#include <string>

using namespace testing;
using namespace std;

#define TEST_TUTORIAL(expectedOutput, executable)                      \
  {                                                                    \
    SCOPED_TRACE("testTutorial called from here (via TEST_TUTORIAL)"); \
    testTutorial(expectedOutput, executable);                          \
  }

void testTutorial(const string& expectedOutput, const string& executable) {
  // run the tutorial which really is an executable EF program thanks to
  // shebang (#!). The program really run (and thus noted in the shebang line)
  // is efc (the EF compiler).
  const string executableDir = "../../../doc/tutorial/";
  FILE* stream = popen((executableDir + executable).c_str(), "r");
  assert(stream);
  string actualOutput;
  while (feof(stream)==0) {
    char buf[1024];
    if (fgets(buf, sizeof(buf), stream) != nullptr) { actualOutput += buf; }
  }
  pclose(stream);

  // remove trailing newline of output
  if (!actualOutput.empty() && actualOutput[actualOutput.size() - 1] == '\n') {
    actualOutput.resize(actualOutput.size() - 1);
  }

  // verify
  EXPECT_EQ(expectedOutput, actualOutput)
    // In format of an compiler error to be able to jump to offending tutorial
    << executableDir << executable << ":1: error: Somewhere in this tutorial";
}

// The intention of the test is just to have a rough test that the turials are
// more or less ok. The test is too weak as to really being able to say 'the
// tutorials are 100% ok'.
TEST(TutorialsTest, MAKE_TEST_NAME(
    one_of_the_EF_tutorials_which_really_is_a_proper_EF_program,
    it_is_executed_via_the_shebang_mechanism_which_ultimatively_runs_efc,
    it_has_the_expected_output)) {
  TEST_TUTORIAL("42", "tutorial1.ef");
  TEST_TUTORIAL("4", "tutorial2.ef");
  TEST_TUTORIAL("5", "tutorial3.ef");
  TEST_TUTORIAL("6", "tutorial4.ef");
  TEST_TUTORIAL("3", "tutorial5.ef");
}
