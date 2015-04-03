#include "executionengineadapter.h"
#include "llvm/ExecutionEngine/JIT.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/IR/Module.h"
using namespace std;
using namespace llvm;

ExecutionEngineApater::ExecutionEngineApater(unique_ptr<Module> module) :
  m_module(*module.get()),
  m_executionEngine(EngineBuilder(module.release()).setErrorStr(&m_errStr).create()) {
}

int ExecutionEngineApater::jitExecFunction(const string& name) {
  return jitExecFunction(m_module.getFunction(name));
}

int ExecutionEngineApater::jitExecFunction1Arg(const string& name, int arg1) {
  return jitExecFunction1Arg(m_module.getFunction(name), arg1);
}

int ExecutionEngineApater::jitExecFunction2Arg(const string& name, int arg1, int arg2) {
  return jitExecFunction2Arg(m_module.getFunction(name), arg1, arg2);
}

void ExecutionEngineApater::jitExecFunctionVoidRet(const string& name) {
  jitExecFunctionVoidRet(m_module.getFunction(name));
}

int ExecutionEngineApater::jitExecFunction(Function* function) {
  void* functionVoidPtr =
    m_executionEngine->getPointerToFunction(function);
  assert(functionVoidPtr);
  int (*functionPtr)() = (int (*)())(intptr_t)functionVoidPtr;
  assert(functionPtr);
  return functionPtr();
}

int ExecutionEngineApater::jitExecFunction1Arg(Function* function, int arg1) {
  void* functionVoidPtr =
    m_executionEngine->getPointerToFunction(function);
  assert(functionVoidPtr);
  int (*functionPtr)(int) = (int (*)(int))(intptr_t)functionVoidPtr;
  assert(functionPtr);
  return functionPtr(arg1);
}

int ExecutionEngineApater::jitExecFunction2Arg(Function* function, int arg1, int arg2) {
  void* functionVoidPtr =
    m_executionEngine->getPointerToFunction(function);
  assert(functionVoidPtr);
  int (*functionPtr)(int, int) = (int (*)(int, int))(intptr_t)functionVoidPtr;
  assert(functionPtr);
  return functionPtr(arg1, arg2);
}

void ExecutionEngineApater::jitExecFunctionVoidRet(Function* function) {
  void* functionVoidPtr =
    m_executionEngine->getPointerToFunction(function);
  assert(functionVoidPtr);
  void (*functionPtr)() = (void (*)())(intptr_t)functionVoidPtr;
  assert(functionPtr);
  functionPtr();
}

