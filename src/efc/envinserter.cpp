#include "envinserter.h"
#include "ast.h"
#include "env.h"
#include "errorhandler.h"
#include "astdefaultiterator.h"
using namespace std;

EnvInserter::EnvInserter(Env& env, ErrorHandler& errorHandler) {
}

void EnvInserter::insertIntoEnv(AstNode& root) {
  AstDefaultIterator iter(*this);
  root.accept(iter);
}

void EnvInserter::visit(AstCast& cast) {}
void EnvInserter::visit(AstNop& nop) {}
void EnvInserter::visit(AstBlock& block) {}
void EnvInserter::visit(AstCtList& ctList) {}
void EnvInserter::visit(AstOperator& op) {}
void EnvInserter::visit(AstSeq& seq) {}
void EnvInserter::visit(AstNumber& number) {}
void EnvInserter::visit(AstSymbol& symbol) {}
void EnvInserter::visit(AstFunCall& funCall) {}
void EnvInserter::visit(AstFunDef& funDef) {}
void EnvInserter::visit(AstDataDef& dataDef) {}
void EnvInserter::visit(AstIf& if_) {}
void EnvInserter::visit(AstLoop& loop) {}
void EnvInserter::visit(AstReturn& return_) {}
void EnvInserter::visit(AstObjTypeSymbol& symbol) {}
void EnvInserter::visit(AstObjTypeQuali& quali) {}
void EnvInserter::visit(AstObjTypePtr& ptr) {}
void EnvInserter::visit(AstClassDef& class_) {}
