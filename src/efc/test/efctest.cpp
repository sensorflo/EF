#include "test.h"
#include "gtestprinter.h"
#include "../irbuilderast.h"
using namespace testing;

// todo: run all tutorials. They must exit successfully and with the expected
// output which is specified in the tutorial itself, probably near the end

// todo: test that loc info is correct. for scanner and for parser,
// independend tests.

int main(int argc, char** argv) {
  IrBuilderAst::staticOneTimeInit();
  InitGoogleTest(&argc, argv);
  TestEventListeners& listeners = ::UnitTest::GetInstance()->listeners();
  delete listeners.Release(listeners.default_result_printer());
  listeners.Append(new GTestPrinter);
  return RUN_ALL_TESTS();
}
