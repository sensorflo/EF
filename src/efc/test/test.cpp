#include "test.h"
#include "../ast.h"
#include "../errorhandler.h"
#include "../env.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_os_ostream.h"
#include <sstream>
using namespace std;
using namespace testing;

string amendSpec(const string& spec) {
  string amendedSpec;
  if (spec.empty()) { /*nop*/
  }
  else {
    amendedSpec +=
      "\nAdditional sub specification or additional description of the "
      "example:\n";
    amendedSpec += spec;
    if (spec[spec.length() - 1] != '\n') { amendedSpec += '\n'; }
  }
  return amendedSpec;
}

string amendAst(const AstNode* ast) {
  assert(ast);
  return string("\nInput AST in its canonical form:\n") + ast->toStr() + "\n";
}

string amendAst(const unique_ptr<AstNode>& ast) {
  return amendAst(ast.get());
}

string amendAst(const unique_ptr<AstObject>& ast) {
  return amendAst(ast.get());
}

string amend(llvm::Module* module) {
  stringstream ss;
  ss << "\n--- llvm module dump ------------\n";
  llvm::raw_os_ostream llvmss(ss);
  module->print(llvmss, nullptr);
  ss << "-----------------------------------\n";
  return ss.str();
}

string amend(const std::unique_ptr<llvm::Module>& module) {
  return amend(module.get());
}

string amend(const ErrorHandler& errorHandler) {
  stringstream ss;
  ss << string("\nErrorHandler's errors:");
  if (errorHandler.errors().empty()) { ss << " none\n"; }
  else {
    ss << "\n" << errorHandler << '\n';
  }
  return ss.str();
}

string amend(const Env& env) {
  stringstream ss;
  ss << "\nEnvironment:\n" << env;
  return ss.str();
}

AstObject* createAccessTo(AstObject* obj, Access access) {
  switch (access) {
  case Access::eRead:
    return new AstDataDef(
      Env::makeUniqueInternalName(), ObjTypeFunda::eInt, obj);
  case Access::eWrite: return new AstOperator('=', obj, new AstNumber(0));
  case Access::eTakeAddress: return new AstOperator('&', obj);
  case Access::eIgnoreValueAndAddr: return new AstSeq(obj, new AstNumber(0));
  case Access::eYetUndefined: assert(false); return nullptr;
  default: assert(false); return nullptr;
  }
}

AstObject* createAccessTo(const string& symbolName, Access access) {
  return createAccessTo(new AstSymbol(symbolName), access);
}
