#ifndef EXECUTION_ENGINE_ADAPTER_H
#define EXECUTION_ENGINE_ADAPTER_H
#include <string>
#include <memory>

namespace llvm {
  class ExecutionEngine;
  class Module;
  class Function;
}

/** Adapter to llvm::ExecutionEngine which makes JIT executing functions more
convenient. */
class ExecutionEngineApater {
public:
  ExecutionEngineApater(std::unique_ptr<llvm::Module> module);

  llvm::Module& module() { return m_module; }

  void jitExec(const std::string& funName);

  int jitExecFunction(const std::string& name);
  int jitExecFunction1Arg(const std::string& name, int arg1);
  int jitExecFunction2Arg(const std::string& name, int arg1, int arg2);
  void jitExecFunctionVoidRet(const std::string& name);


private:
  int jitExecFunction(llvm::Function* function);
  int jitExecFunction1Arg(llvm::Function* function, int arg1);
  int jitExecFunction2Arg(llvm::Function* function, int arg1, int arg2);
  void jitExecFunctionVoidRet(llvm::Function* function);

  llvm::Module& m_module;
  std::string m_errStr;
  std::unique_ptr<llvm::ExecutionEngine> m_executionEngine;
};

#endif
