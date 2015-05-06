#include "executionengineadapter.h"
#include "llvm/ExecutionEngine/MCJIT.h"
using namespace std;
using namespace llvm;

ExecutionEngineApater::ExecutionEngineApater(unique_ptr<Module> module) :
  m_module(*module.get()),
  m_executionEngine(EngineBuilder(move(module)).create()) {
}

