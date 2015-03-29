#include "test.h"
#include "../ast.h"
#include "../errorhandler.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_os_ostream.h"
#include <sstream>
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

string amendAst(const unique_ptr<AstValue>& ast) {
  return amendAst(ast.get());
}

string amend(llvm::Module* module) {
  stringstream ss;
  ss << "\n--- llvm module dump ------------\n";
  llvm::raw_os_ostream llvmss(ss);
  module->print(llvmss, NULL);
  ss << "-----------------------------------\n";
  return ss.str();
}

string amend(const ErrorHandler& errorHandler) {
  stringstream ss;
  ss << string("\nErrorHanlder's errors:");
  if (errorHandler.errors().empty()) {
    ss << " none\n";
  } else {
    ss << "\n" << errorHandler << '\n';
  }
  return ss.str();
}
