#include "driver.h"

#include "ast.h"
#include "env.h"
#include "errorhandler.h"
#include "executionengineadapter.h"
#include "irgen.h"
#include "parser.h"
#include "scanner.h"
#include "semanticanalizer.h"
#include "tokenfilter.h"

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/IR/Module.h"

using namespace std;

/** \param osstream caller keeps ownership */
Driver::Driver(string fileName, std::basic_ostream<char>* ostream)
  : m_errorHandler(make_unique<ErrorHandler>())
  , m_env(make_unique<Env>())
  , m_ostream(ostream ? *ostream : cerr)
  , m_scanner(Scanner::create(move(fileName), *m_errorHandler))
  , m_tokenFilter(make_unique<TokenFilter>(*m_scanner.get()))
  , m_parser(make_unique<Parser>(*m_tokenFilter.get(), *m_env, *m_errorHandler))
  , m_irGen(make_unique<IrGen>(*m_errorHandler))
  , m_semanticAnalizer(make_unique<SemanticAnalizer>(*m_env, *m_errorHandler)) {
  assert(m_errorHandler);
  assert(m_env);
  assert(m_scanner);
  assert(m_tokenFilter);
  assert(m_parser);
  assert(m_irGen);
  assert(m_semanticAnalizer);
}

Driver::~Driver() = default;

Scanner& Driver::scanner() {
  return *m_scanner;
}

ErrorHandler& Driver::errorHandler() {
  return *m_errorHandler;
}

/** Compile = scann & parse & do semantic analysis & generate IR. */
void Driver::compile() {
  try {
    Env::AutoLetLooseNodes dummy(*m_env);

    auto astAfterParse = scanAndParse();

    // It's currently implied that the module wants an implicit main method
    const auto astAfterImplicitMain =
      m_parser->addImplicitMain(move(astAfterParse));

    doSemanticAnalysis(*astAfterImplicitMain);

    generateIr(*astAfterImplicitMain);
  }
  catch (BuildError& e) {
    // nop -- BuildError exception is handled below by printing any errors
  }
  if (!m_errorHandler->errors().empty()) {
    m_ostream << *m_errorHandler << "\n";
  }
}

unique_ptr<AstNode> Driver::scanAndParse() {
  // the parser internally drives the scanner
  auto res = m_parser->parse_();
  if (res.m_errorCode) {
    Error::throwError(*m_errorHandler, Error::eScanOrParseFailed);
  }
  assert(res.m_astRoot);
  return move(res.m_astRoot);
}

void Driver::doSemanticAnalysis(AstNode& ast) {
  m_semanticAnalizer->analyze(ast);
}

void Driver::generateIr(AstNode& ast) {
  auto module = m_irGen->genIr(ast);
  m_executionEngine = std::make_unique<ExecutionEngineApater>(move(module));
}

int Driver::jitExecMain() {
  assert(m_executionEngine);
  return m_executionEngine->jitExecFunction(".main");
}
