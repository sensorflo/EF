#include "executionengineadapter.h"
#include "llvm/ExecutionEngine/JIT.h"
using namespace std;
using namespace llvm;

ExecutionEngineApater::ExecutionEngineApater(unique_ptr<Module> module) :
  m_module(*module.get()),
  m_executionEngine(EngineBuilder(module.release()).setErrorStr(&m_errStr).create()) {
}

