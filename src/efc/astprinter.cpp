#include "astprinter.h"

#include "ast.h"

#include <sstream>

using namespace std;

namespace {
string wrapName(const string& str) {
  if (!str.empty()) { return str; }
  return "<anonymous>";
}
}

AstPrinter::AstPrinter(basic_ostream<char>& os) : m_os(os) {
}

string AstPrinter::toStr(const AstNode& root) {
  ostringstream oss;
  AstPrinter printer(oss);
  root.accept(printer);
  return oss.str();
}

basic_ostream<char>& AstPrinter::printTo(
  const AstNode& root, basic_ostream<char>& os) {
  AstPrinter printer(os);
  root.accept(printer);
  return os;
}

void AstPrinter::visit(const AstNop& /*nop*/) {
  m_os << "nop";
}

void AstPrinter::visit(const AstBlock& block) {
  // Since AstBlock alwasy only has exactly one 'operand', and since it is
  // quite common, for the reader's convenience, only prefix body with ':',
  // opposed to enclose body in ':(...)'. See also class' comment.
  m_os << ":";
  block.body().accept(*this);
}

void AstPrinter::visit(const AstCast& cast) {
  cast.specifiedNewAstObjType().accept(*this);
  m_os << "(";
  cast.args().accept(*this);
  m_os << ")";
}

void AstPrinter::visit(const AstCtList& ctList) {
  auto& childs = ctList.childs();
  auto isFirstIter = true;
  for (const auto& child : childs) {
    if (!isFirstIter) { m_os << " "; }
    isFirstIter = false;
    child->accept(*this);
  }
}

void AstPrinter::visit(const AstOperator& op) {
  m_os << op.op() << '(';
  op.args().accept(*this);
  m_os << ')';
}

void AstPrinter::visit(const AstSeq& seq) {
  // For the readers convenience, the case of only one operand is printed
  // specially: only prefix it with ';', opposed to enclose it in
  // ';(...)'. See also class comment.
  m_os << ';';
  if (seq.operands().size() == 1) { seq.operands().front()->accept(*this); }
  else {
    m_os << "(";
    bool isFirstIter = true;
    for (const auto& op : seq.operands()) {
      if (!isFirstIter) { m_os << " "; }
      isFirstIter = false;
      op->accept(*this);
    }
    m_os << ')';
  }
}

void AstPrinter::visit(const AstNumber& number) {
  number.declaredAstObjType().printValueTo(m_os, number.value());
}

void AstPrinter::visit(const AstSymbol& symbol) {
  m_os << wrapName(symbol.name());
}

void AstPrinter::visit(const AstFunCall& funCall) {
  m_os << "call(";
  funCall.address().accept(*this);
  if (!funCall.args().childs().empty()) {
    m_os << " ";
    funCall.args().accept(*this);
  }
  m_os << ")";
}

void AstPrinter::visit(const AstFunDef& funDef) {
  m_os << "fun(" << wrapName(funDef.name()) << " (";
  auto isFirstIter = true;
  for (const auto& arg : funDef.declaredArgs()) {
    if (!isFirstIter) { m_os << " "; }
    isFirstIter = false;
    m_os << "(";
    printNakedDataDef(*arg, true);
    m_os << ")";
  }
  m_os << ") ";
  funDef.ret().accept(*this);
  m_os << " ";
  funDef.body().accept(*this);
  m_os << ")";
}

void AstPrinter::visit(const AstDataDef& dataDef) {
  m_os << "data(";
  printNakedDataDef(dataDef);
  m_os << ")";
}

void AstPrinter::visit(const AstIf& if_) {
  m_os << "if(";
  if_.condition().accept(*this);
  m_os << " ";
  if_.action().accept(*this);
  if (if_.elseAction() != nullptr) {
    m_os << " ";
    if_.elseAction()->accept(*this);
  }
  m_os << ")";
}

void AstPrinter::visit(const AstLoop& loop) {
  m_os << "while(";
  loop.condition().accept(*this);
  m_os << " ";
  loop.body().accept(*this);
  m_os << ")";
}

void AstPrinter::visit(const AstReturn& return_) {
  m_os << "return(";
  return_.ctorArgs().accept(*this);
  m_os << ")";
}

void AstPrinter::visit(const AstObjTypeSymbol& symbol) {
  m_os << wrapName(symbol.name());
}

void AstPrinter::visit(const AstObjTypeQuali& quali) {
  auto qualifiers = static_cast<int>(quali.qualifiers());
  if (qualifiers & ObjType::eMutable) {
    m_os << "mut-";
    qualifiers &= ~ObjType::eMutable;
  }
  assert(0 == qualifiers);
  quali.targetType().accept(*this);
}

void AstPrinter::visit(const AstObjTypePtr& ptr) {
  m_os << "raw*";
  ptr.pointee().accept(*this);
}

void AstPrinter::visit(const AstClassDef& class_) {
  m_os << "class(" << wrapName(class_.name());
  for (const auto& dataMember : class_.dataMembers()) {
    m_os << " ";
    dataMember->accept(*this);
  }
  m_os << ")";
}

void AstPrinter::printNakedDataDef(
  const AstDataDef& dataDef, bool ommitZeroArgInitializer) {
  m_os << wrapName(dataDef.name()) << " ";
  if (dataDef.declaredStorageDuration() != StorageDuration::eLocal) {
    m_os << dataDef.declaredStorageDuration() << "/";
  }
  dataDef.declaredAstObjType().accept(*this);

  if (dataDef.doNotInit()) { m_os << " noinit"; }
  else if (!ommitZeroArgInitializer || !dataDef.ctorArgs().childs().empty()) {
    m_os << " (";
    dataDef.ctorArgs().accept(*this);
    m_os << ")";
  }
};
