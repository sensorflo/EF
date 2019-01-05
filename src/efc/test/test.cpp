#include "test.h"

#include "../ast.h"
#include "../env.h"
#include "../errorhandler.h"

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

string amend(const unique_ptr<llvm::Module>& module) {
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
  ss << "\nEnvironment:\n" << env << "\n";
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

TryCatchResult tryCatch(const function<void()>& f) {
  TryCatchResult res;
  try {
    f();
  }
  catch (const BuildError&) { /* nop, implicitely added to error handler */
  }
  catch (const exception& e) {
    res.m_foreignCatches = true;
    res.m_excptionWhat = e.what();
  }
  catch (const exception* e) {
    res.m_foreignCatches = true;
    if (e) { res.m_excptionWhat = e->what(); }
  }
  catch (...) {
    res.m_foreignCatches = true;
  }
  return res;
}

void verifyErrorHandlerHasExpectedError(const ErrorHandler& errorHandler,
  const string& details, bool foreignCatches, const string& exceptionWhat,
  Error::No expectedErrorNo, const string& expectedMsgParam1,
  const string& expectedMsgParam2, const string& expectedMsgParam3) {
  // verify no foreign exception was thrown
  ASSERT_FALSE(foreignCatches) << "\nA foreign exception with description '"
                               << exceptionWhat << "' was thrown.\n"
                               << details;

  // verify that as expected no error was reported
  if (expectedErrorNo == Error::eNone) {
    EXPECT_FALSE(errorHandler.hasErrors()) << "\nUnexpectedly errors occured:\n"
                                           << errorHandler << "\n"
                                           << details;
  }

  // verify that the actual error is as expected
  else {
    const auto& errors = errorHandler.errors();

    ASSERT_TRUE(errorHandler.hasErrors())
      << "\nExpected the error '" << expectedErrorNo
      << "', but no error at all occured.\n"
      << details;

    ASSERT_EQ(1U, errors.size())
      << "\nExpected the single error '" << expectedErrorNo
      << "', but more errors occured:" << errorHandler << "\n"
      << details;

    const auto& error = *errors.front();
    EXPECT_EQ(expectedErrorNo, error.no()) << details;
    if (expectedMsgParam1 != "<dont_verify>") {
      EXPECT_EQ(expectedMsgParam1, error.msgParam1()) << details;
    }
    if (expectedMsgParam2 != "<dont_verify>") {
      EXPECT_EQ(expectedMsgParam2, error.msgParam2()) << details;
    }
    if (expectedMsgParam3 != "<dont_verify>") {
      EXPECT_EQ(expectedMsgParam3, error.msgParam3()) << details;
    }
  }
}
