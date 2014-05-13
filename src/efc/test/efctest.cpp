#include "test.h"
#include "gtestprinter.h"
#include "../irbuilderast.h"
using namespace testing;

int main(int argc, char** argv) {
  IrBuilderAst::staticOneTimeInit();
  InitGoogleTest(&argc, argv);
  TestEventListeners& listeners = ::UnitTest::GetInstance()->listeners();
  delete listeners.Release(listeners.default_result_printer());
  listeners.Append(new GTestPrinter);
  return RUN_ALL_TESTS();
}
