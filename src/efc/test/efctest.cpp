#include "test.h"
#include "../irgen.h"
#include "../parserapiext.h"
using namespace testing;

int main(int argc, char** argv) {
  IrGen::staticOneTimeInit();
  ParserApiExt::initTokenAttrs();
  InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
