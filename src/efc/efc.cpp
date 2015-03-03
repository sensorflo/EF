#include "irgen.h"
#include "driver.h"
#include <iostream>
#include <stdexcept>
using namespace std;

int main(int argc, char** argv) {
  if (argc!=2) {
    cerr << "Only exactly one argument, the EF program file name, is allowed.";
    exit(1);
  }
  try {
    IrGen::staticOneTimeInit();
    Driver driver(argv[1]);
    driver.buildAndRunModule();
  }
  catch (const exception& e) {
    cerr << e.what() << endl;
    exit(1);
  }
  return 0;
}
