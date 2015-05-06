#include "test.h"
#include "gtestprinter.h"
#include "../irgen.h"
#include "../parserapiext.h"
using namespace testing;

int main(int argc, char** argv) {
  IrGen::staticOneTimeInit();
  ParserApiExt::initTokenAttrs();
  InitGoogleTest(&argc, argv);
  TestEventListeners& listeners = ::UnitTest::GetInstance()->listeners();
  delete listeners.Release(listeners.default_result_printer());
  listeners.Append(new GTestPrinter);
  return RUN_ALL_TESTS();
}
