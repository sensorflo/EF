#include "test.h"
#include "../irbuilderast.h"
using namespace testing;

int main(int argc, char** argv) {
  IrBuilderAst::staticOneTimeInit();
  InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
