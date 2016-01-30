#pragma once
#include "declutils.h"
#include "llvm/IR/Module.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include <string>
#include <memory>

namespace llvm {
  class Function;
}

/** Adapter to llvm::ExecutionEngine which makes JIT executing functions more
convenient. */
class ExecutionEngineApater final {
public:
  ExecutionEngineApater(std::unique_ptr<llvm::Module> module);
  ~ExecutionEngineApater() = default;

  llvm::Module& module() { return m_module; }

  template<typename TRet = int, typename...TArgs>
  TRet jitExecFunction(const std::string& name, TArgs...args) {
    m_executionEngine->finalizeObject();
    const auto function = m_module.getFunction(name);
    assert(function);
    void* functionVoidPtr = m_executionEngine->getPointerToFunction(function);
    assert(functionVoidPtr);
    TRet (*functionPtr)(TArgs...) = (TRet (*)(TArgs...))(intptr_t)functionVoidPtr;
    assert(functionPtr);
    return functionPtr(args...);
  }

private:
  NEITHER_COPY_NOR_MOVEABLE(ExecutionEngineApater);

  llvm::Module& m_module;
  /** Guaranteed to be non-null. We're _not_ the owner. */
  llvm::ExecutionEngine* m_executionEngine;
};
