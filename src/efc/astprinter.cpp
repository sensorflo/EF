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
  block.body().accept(*this);
}

void AstPrinter::visit(const AstCast& cast) {
  m_os << cast.objType() << "(";
  cast.child().accept(*this);
  m_os << ")";
}

void AstPrinter::visit(const AstCtList& ctList) {
  list<AstValue*>& childs = ctList.childs();
  for (list<AstValue*>::const_iterator i=childs.begin();
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
  funCall.address().accept(*this);
  m_os << "(";
  funCall.args().accept(*this);
  m_os << ")";
}

void AstPrinter::visit(const AstFunDef& funDef) {
  m_os << "fun(" << funDef.name() << " (";
  for (list<AstArgDecl*>::const_iterator i=funDef.args().begin();
       i!=funDef.args().end(); ++i) {
    if (i!=funDef.args().begin()) { m_os << " "; }
    (*i)->accept(*this);
  }
  m_os << ") ";
  m_os << funDef.retObjType();
  m_os << " ";
  funDef.body().accept(*this);
  m_os << ")";
}

void AstPrinter::visit(const AstDataDecl& dataDecl) {
  m_os << "decldata(" << dataDecl.name() << " ";
  if ( dataDecl.declaredStorageDuration()!=StorageDuration::eLocal ) {
    m_os << dataDecl.declaredStorageDuration() << "/";
  }
  m_os << dataDecl.declaredObjType() << ")";
}

void AstPrinter::visit(const AstArgDecl& argDecl) {
  m_os << "(" << argDecl.name() << " " << argDecl.declaredObjType() << ")";
}

void AstPrinter::visit(const AstDataDef& dataDef) {
  m_os << "data(";
  dataDef.decl().accept(*this);
  m_os << " (";
  dataDef.ctorArgs().accept(*this);
  m_os << "))";
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
