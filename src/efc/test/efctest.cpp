#include "../irgen.h"
#include "../parser.h"
#include "test.h"

using namespace testing;

int main(int argc, char** argv) {
  IrGen::staticOneTimeInit();
  Parser::initTokenAttrs();
  InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
