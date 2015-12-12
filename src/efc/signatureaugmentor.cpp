#include "signatureaugmentor.h"
#include "ast.h"
#include "env.h"
#include "errorhandler.h"
#include "astdefaultiterator.h"
#include "object.h"
using namespace std;

SignatureAugmentor::SignatureAugmentor(Env& env, ErrorHandler& errorHandler) {
}

void SignatureAugmentor::augmentEntities(AstNode& root) {
  AstDefaultIterator iter(*this);
  root.accept(iter);
}

void SignatureAugmentor::visit(AstCast& cast) {}
void SignatureAugmentor::visit(AstNop& nop) {}
void SignatureAugmentor::visit(AstBlock& block) {}
void SignatureAugmentor::visit(AstCtList& ctList) {}
void SignatureAugmentor::visit(AstOperator& op) {}
void SignatureAugmentor::visit(AstSeq& seq) {}
void SignatureAugmentor::visit(AstNumber& number) {}
void SignatureAugmentor::visit(AstSymbol& symbol) {}
void SignatureAugmentor::visit(AstFunCall& funCall) {}
void SignatureAugmentor::visit(AstFunDef& funDef) {}
void SignatureAugmentor::visit(AstDataDef& dataDef) {}
void SignatureAugmentor::visit(AstIf& if_) {}
void SignatureAugmentor::visit(AstLoop& loop) {}
void SignatureAugmentor::visit(AstReturn& return_) {}
void SignatureAugmentor::visit(AstObjTypeSymbol& symbol) {}
void SignatureAugmentor::visit(AstObjTypeQuali& quali) {}
void SignatureAugmentor::visit(AstObjTypePtr& ptr) {}
void SignatureAugmentor::visit(AstClassDef& class_) {}
