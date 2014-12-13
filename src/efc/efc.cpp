#include "irbuilderast.h"
#include "driver.h"
#include "ast.h"
#include "env.h"
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
    Env env;
    Driver driver(env, argv[1]);
    AstValue* ast = NULL;
    if (driver.parse(ast)) {
      exit(1);
    }
    IrBuilderAst irBuilderAst(env);
    cout << irBuilderAst.buildAndRunModule(*ast) << "\n";
  }
  catch (const exception& e) {
    cerr << e.what() << endl;
    exit(1);
  }
  return 0;
}
