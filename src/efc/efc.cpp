#include "driver.h"
#include "errorhandler.h"
#include "irgen.h"

#include <iostream>
#include <stdexcept>

using namespace std;

int main(int argc, char** argv) {
  if (argc != 2) {
    cerr << "Only exactly one argument, the EF program file name, is allowed.";
    exit(1);
  }
  try {
    IrGen::staticOneTimeInit();
    Driver driver(argv[1]);
    driver.compile();
    if (driver.errorHandler().hasErrors()) { exit(1); }
    cout << driver.jitExecMain() << "\n";
  }
  catch (const exception& e) {
    cerr << e.what() << endl;
    exit(1);
  }
  catch (...) {
    cerr << "unknown exception" << endl;
    exit(1);
  }
  return 0;
}
