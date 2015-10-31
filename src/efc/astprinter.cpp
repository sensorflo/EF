#include "astprinter.h"
#include "ast.h"
#include <list>
#include <sstream>
using namespace std;

AstPrinter::AstPrinter(basic_ostream<char>& os) : m_os(os) {
}

string AstPrinter::toStr(const AstNode& root) {
  ostringstream oss;
  AstPrinter printer(oss);
  root.accept(printer);
  return oss.str();
}

basic_ostream<char>& AstPrinter::printTo(const AstNode& root,
  basic_ostream<char>& os) {
  AstPrinter printer(os);
  root.accept(printer);
  return os;
}

void AstPrinter::visit(const AstNop& nop) {
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
  m_os << cast.objType() << "(";
  cast.child().accept(*this);
  m_os << ")";
}

void AstPrinter::visit(const AstCtList& ctList) {
  list<AstObject*>& childs = ctList.childs();
  for (list<AstObject*>::const_iterator i=childs.begin();
       i!=childs.end(); ++i) {
    if (i!=childs.begin()) { m_os << " "; }
    (*i)->accept(*this);
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
  if (seq.operands().size()==1) {
    seq.operands().front()->accept(*this);
  } else {
    m_os << "(";
    bool isFirstIter = true;
    for (const auto& op: seq.operands()) {
      if ( !isFirstIter ) {
        m_os << " ";
      }
      isFirstIter = false;
      op->accept(*this);
    }
    m_os << ')';
  }
}

void AstPrinter::visit(const AstNumber& number) {
  // if value is outside range of type, that is a topic that shall not
  // interest us at this point here
  if ( number.objType().type() == ObjTypeFunda::eChar) {
    m_os << "'" << char(number.value()) << "'";
  } else {
    m_os << number.value();
    switch (number.objType().type()) {
    case ObjTypeFunda::eBool: m_os << "bool"; break;
    case ObjTypeFunda::eInt: break;
    case ObjTypeFunda::eDouble: m_os << "d"; break;
    default: assert(false);
    }
  }
}

void AstPrinter::visit(const AstSymbol& symbol) {
  m_os << symbol.name();
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
  m_os << "fun(" << funDef.name() << " (";
  for (list<AstDataDef*>::const_iterator i=funDef.args().begin();
       i!=funDef.args().end(); ++i) {
    if (i!=funDef.args().begin()) { m_os << " "; }
    // as an exception, handle parameters directly, opposed to accept *i
    m_os << "(" << (*i)->name() << " " << (*i)->declaredObjType() << ")";
  }
  m_os << ") ";
  m_os << funDef.retObjType();
  m_os << " ";
  funDef.body().accept(*this);
  m_os << ")";
}

void AstPrinter::visit(const AstDataDef& dataDef) {
  m_os << "data(";

  m_os << dataDef.name() << " ";
  if ( dataDef.declaredStorageDuration()!=StorageDuration::eLocal ) {
    m_os << dataDef.declaredStorageDuration() << "/";
  }
  m_os << dataDef.declaredObjType();

  if ( dataDef.doNotInit() ) {
    m_os << " noinit";
  } else {
    m_os << " (";
    if ( !dataDef.m_ctorArgsWereImplicitelyDefined ) {
      dataDef.ctorArgs().accept(*this);
    }
    m_os << ")";
  }
  m_os << ")";
}

void AstPrinter::visit(const AstIf& if_) {
  m_os << "if(";
  if_.condition().accept(*this);
  m_os << " ";
  if_.action().accept(*this);
  if ( if_.elseAction() ) {
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
  return_.retVal().accept(*this);
  m_os << ")";
}

void AstPrinter::visit(const AstClassDef& class_) {
  m_os << "class(" << class_.name();
  for (const auto& dataMember: class_.dataMembers()) {
    m_os << " ";
    dataMember->accept(*this);
  }
  m_os << ")";
}

