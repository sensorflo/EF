#include "irbuilderast.h"
#include "driver.h"
#include "ast.h"
#include <iostream>
#include <stdexcept>
using namespace std;

int main(int argc, char** argv) {
  if (argc!=2) {
    cerr << "Only exactly one argument, the EF program file name, is allowed.";
    exit(1);
  }
  try {
    IrBuilderAst::staticOneTimeInit();
    Driver driver(argv[1]);
    AstSeq* ast = NULL;
    if (driver.parse(ast)) {
      exit(1);
    }
    IrBuilderAst irBuilderAst;
    cout << irBuilderAst.buildAndRunModule(*ast) << "\n";
  }
  catch (const exception& e) {
    cerr << e.what() << endl;
    exit(1);
  }
  return 0;
}
