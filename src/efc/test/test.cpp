#include "test.h"
#include "../ast.h"
using namespace std;
using namespace testing;

string amendSpec(const string& spec) {
  string amendedSpec;
  if (spec.empty()) {
    /*nop*/
  } else {
    amendedSpec += "\nAdditional sub specification or additional description of the example:\n";
    amendedSpec += spec;
    if ( spec[spec.length()-1]!='\n' ) {
      amendedSpec += '\n';
    }
  }
  return amendedSpec;
}

string amendAst(const AstNode* ast) {
  assert(ast);
  return string("\nInput AST in its canonical form:\n") + ast->toStr() + "\n";
}

string amendAst(const auto_ptr<AstValue>& ast) {
  return amendAst(ast.get());
}
