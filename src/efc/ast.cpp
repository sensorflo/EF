#include "ast.h"
#include "env.h"
#include "astvisitor.h"
#include "astprinter.h"
#include "errorhandler.h"
#include <cassert>
#include <stdexcept>
using namespace std;

namespace {

template<typename T>
vector<unique_ptr<T>> toUniquePtrs(vector<T*>* src) {
  if ( !src ) {
    return {};
  }
  const unique_ptr<vector<T*>> dummy(src);
  vector<unique_ptr<T>> dst{};
  for ( const auto& element : *src ) {
    if ( element ) {
      dst.emplace_back(element);
    }
  }
  return dst;
}

template<typename T>
vector<unique_ptr<T>> toUniquePtrs(T* src1 = nullptr, T* src2 = nullptr,
  T* src3 = nullptr, T* src4 = nullptr, T* src5 = nullptr) {
  return toUniquePtrs(new vector<T*>{src1, src2, src3, src4, src5});
}

thread_local std::shared_ptr<const ObjTypeFunda> objTypeFundaVoid =
  make_shared<ObjTypeFunda>(ObjType::eVoid);

thread_local std::shared_ptr<const ObjTypeFunda> objTypeFundaNoreturn =
  make_shared<ObjTypeFunda>(ObjType::eNoreturn);

}

string AstNode::toStr() const {
  return AstPrinter::toStr(*this);
}

bool AstObject::isObjTypeNoReturn() const {
  return object().objType().isNoreturn();
}

AstObjInstance::AstObjInstance() :
  m_accessFromAstParent{Access::eYetUndefined} {
}

void AstObjInstance::setAccessFromAstParent(Access access) {
  assert(Access::eYetUndefined!=access);
  assert(Access::eYetUndefined==m_accessFromAstParent);
  m_accessFromAstParent = access;
  addAccess(access);
}

Access AstObjInstance::accessFromAstParent() const {
  assert(Access::eYetUndefined!=m_accessFromAstParent);
  return m_accessFromAstParent;
}

AstObjDef::AstObjDef(string name) :
  EnvNode(move(name)) {
}

basic_ostream<char>& AstObjDef::printTo(basic_ostream<char>& os) const {
  const auto storageDuration_ = storageDuration();
  if ( storageDuration_!=StorageDuration::eLocal ) {
    os << storageDuration_ << "/";
  }
  os << objType();
  return os;
}

const ObjType& AstNop::objType() const {
  return *objTypeFundaVoid;
}

std::shared_ptr<const ObjType> AstNop::objTypeAsSp() const {
  return objTypeFundaVoid;
}

StorageDuration AstNop::storageDuration() const {
  return StorageDuration::eLocal;
}

AstBlock::AstBlock(AstObject* body) :
  EnvNode(Env::makeUniqueInternalName("$block")),
  m_body(body) {
  assert(m_body);
}

const ObjType& AstBlock::objType() const {
  return *objTypeAsSp();
}

std::shared_ptr<const ObjType> AstBlock::objTypeAsSp() const {
  return m_body->object().objType().unqualifiedObjType();
}

StorageDuration AstBlock::storageDuration() const {
  return StorageDuration::eLocal;
}

AstCast::AstCast(AstObjType* specifiedNewAstObjType, AstObject* child) :
  m_specifiedNewAstObjType(
    specifiedNewAstObjType ?
    unique_ptr<AstObjType>(specifiedNewAstObjType) :
    make_unique<AstObjTypeSymbol>(ObjType::eInt)),
  m_child(child ? unique_ptr<AstObject>(child) : make_unique<AstNumber>(0)) {
  assert(m_child);
  assert(m_specifiedNewAstObjType);
}

AstCast::AstCast(ObjType::EType specifiedNewOjType, AstObject* child) :
  AstCast{new AstObjTypeSymbol(specifiedNewOjType), child} {
}

AstObjType& AstCast::specifiedNewAstObjType() const {
  return *m_specifiedNewAstObjType;
}

const ObjType& AstCast::objType() const {
  return m_specifiedNewAstObjType->objType();
}

std::shared_ptr<const ObjType> AstCast::objTypeAsSp() const {
  return m_specifiedNewAstObjType->objTypeAsSp();
}

StorageDuration AstCast::storageDuration() const {
  return StorageDuration::eLocal;
}

AstFunDef::AstFunDef(const string& name,
  std::vector<AstDataDef*>* args,
  AstObjType* ret,
  AstObject* body) :
  AstObjDef(name),
  m_args(toUniquePtrs(args)),
  m_ret(ret),
  m_body(body) {
  assert(m_ret);
  assert(m_body);
  for ( const auto& arg : m_args ) {
    assert(arg);
  }
}

vector<AstDataDef*>* AstFunDef::createArgs(AstDataDef* arg1,
  AstDataDef* arg2, AstDataDef* arg3) {
  auto args = new vector<AstDataDef*>;
  if (arg1) { args->push_back(arg1); }
  if (arg2) { args->push_back(arg2); }
  if (arg3) { args->push_back(arg3); }
  return args;
}

const ObjType& AstFunDef::objType() const {
  if ( !m_objType ) {
    objTypeAsSp(); // internally sets m_objType
  }
  return *m_objType;
}

std::shared_ptr<const ObjType> AstFunDef::objTypeAsSp() const {
  if ( !m_objType ) {
    const auto& argsObjType = new vector<shared_ptr<const ObjType>>;
    for (const auto& astArg: m_args) {
      assert(astArg);
      assert(astArg->declaredAstObjType().objTypeAsSp());
      argsObjType->push_back(astArg->declaredAstObjType().objTypeAsSp());
    }
    assert(m_ret->objTypeAsSp());
    m_objType = make_shared<const ObjTypeFun>(
      argsObjType, m_ret->objTypeAsSp());
  }
  return m_objType;
}

StorageDuration AstFunDef::storageDuration() const {
  return StorageDuration::eStatic;
}

AstObject* const AstDataDef::noInit = reinterpret_cast<AstObject*>(1);

AstDataDef::AstDataDef(const std::string& name, AstObjType* declaredAstObjType,
  StorageDuration declaredStorageDuration,  AstCtList* ctorArgs) :
  AstObjDef(name),
  m_declaredAstObjType(declaredAstObjType ?
    unique_ptr<AstObjType>(declaredAstObjType) :
    make_unique<AstObjTypeSymbol>(ObjType::eInt)),
  m_declaredStorageDuration(declaredStorageDuration),
  m_ctorArgs(mkCtorArgs(ctorArgs, m_declaredStorageDuration, m_doNotInit)) {
  assert(declaredStorageDuration != StorageDuration::eYetUndefined);
  assert(m_ctorArgs);
}

AstDataDef::AstDataDef(const std::string& name, AstObjType* declaredAstObjType,
  StorageDuration declaredStorageDuration, AstObject* initObj) :
  AstDataDef(name, declaredAstObjType, declaredStorageDuration,
    initObj ? new AstCtList(initObj) : nullptr) {
}

AstDataDef::AstDataDef(const std::string& name, AstObjType* declaredAstObjType,
  AstObject* initObj) :
  AstDataDef(name, declaredAstObjType, StorageDuration::eLocal, initObj) {
}

AstDataDef::AstDataDef(const std::string& name, ObjType::EType declaredObjType,
  AstObject* initObj) :
  AstDataDef(name, new AstObjTypeSymbol(declaredObjType), initObj) {
}

const ObjType& AstDataDef::objType() const {
  return m_declaredAstObjType->objType();
}

shared_ptr<const ObjType> AstDataDef::objTypeAsSp() const {
  return m_declaredAstObjType->objTypeAsSp();
}

unique_ptr<AstCtList> AstDataDef::mkCtorArgs(AstCtList* ctorArgs,
  StorageDuration storageDuration, bool& doNotInit) {
  if ( storageDuration==StorageDuration::eMember ) {
    doNotInit = true;
    assert(nullptr==ctorArgs || ctorArgs->childs().empty());
    if ( nullptr==ctorArgs ) {
      return make_unique<AstCtList>();
    } else {
      return unique_ptr<AstCtList>(ctorArgs);
    }
  } else {
    doNotInit = false; // assumption
    if (not ctorArgs) {
      ctorArgs = new AstCtList();
    }
    if (not ctorArgs->childs().empty()) {
      if ( ctorArgs->childs().front()==noInit ) {
        doNotInit = true;
        assert(ctorArgs->childs().size()==1);
        ctorArgs->childs().clear();
      }
    }
  }
  return unique_ptr<AstCtList>(ctorArgs);
}

AstObjType& AstDataDef::declaredAstObjType() const {
  return *m_declaredAstObjType.get();
}

StorageDuration AstDataDef::declaredStorageDuration() const {
  return m_declaredStorageDuration;
}

AstNumber::AstNumber(GeneralValue value, AstObjType* astObjType) :
  m_value(value),
  m_declaredAstObjType(
    astObjType ? astObjType : new AstObjTypeSymbol(ObjType::eInt)) {
  assert(m_declaredAstObjType);
}

AstNumber::AstNumber(GeneralValue value, ObjType::EType eType) :
  AstNumber(value, new AstObjTypeSymbol(eType)) {
}

AstObjType& AstNumber::declaredAstObjType() const {
  assert(m_declaredAstObjType);
  return *m_declaredAstObjType;
}

const ObjType& AstNumber::objType() const {
  return m_declaredAstObjType->objType();
};

std::shared_ptr<const ObjType> AstNumber::objTypeAsSp() const {
  return m_declaredAstObjType->objTypeAsSp();
};

StorageDuration AstNumber::storageDuration() const {
  return StorageDuration::eLocal;
};

void AstSymbol::setAccessFromAstParent(Access access) {
  assert(Access::eYetUndefined!=access);
  assert(Access::eYetUndefined==m_accessFromAstParent);
  m_accessFromAstParent = access;
  if ( m_referencedAstObj ) {
    m_referencedAstObj->addAccess(access);
  }
}

Access AstSymbol::accessFromAstParent() const {
  return m_accessFromAstParent;
}

Object& AstSymbol::object() {
  assert(m_referencedAstObj);
  return *m_referencedAstObj;
}

const Object& AstSymbol::object() const {
  assert(m_referencedAstObj);
  return *m_referencedAstObj;
}

const ObjType& AstSymbol::objType() const {
  assert(m_referencedAstObj);
  return m_referencedAstObj->objType();
}

std::shared_ptr<const ObjType> AstSymbol::objTypeAsSp() const {
  assert(m_referencedAstObj);
  return m_referencedAstObj->objTypeAsSp();
}

StorageDuration AstSymbol::storageDuration() const {
  assert(m_referencedAstObj);
  return m_referencedAstObj->storageDuration();
}

void AstSymbol::setreferencedAstObjAndPropagateAccess(AstObjDef& referencedAstObj) {
  assert(!m_referencedAstObj); // it doesnt make sense to set it twice
  m_referencedAstObj = &referencedAstObj;
  m_referencedAstObj->addAccess(accessFromAstParent());
}

bool AstSymbol::isInitialized() const {
  assert(m_referencedAstObj);
  return m_referencedAstObj->isInitialized();
}

const map<const string, const AstOperator::EOperation> AstOperator::m_opMap{
  {"and", eAnd},
  {"&&", eAnd},
  {"or", eOr},
  {"||", eOr},
  {"==", eEqualTo},
  {"=<", eAssign},
  {"not", eNot}};

// in case of ambiguity, prefer one letters over symbols. Also note that
// single char operators are prefered over multi chars; the single chars are
// not even in the map.
const map<const AstOperator::EOperation, const std::string> AstOperator::m_opReverseMap{
  {eAnd, "and"},
  {eOr, "or"},
  {eEqualTo, "=="},
  {eAssign, "=<"},
  {eDeref, "*"}};

AstOperator::AstOperator(char op, AstCtList* args) :
  AstOperator(static_cast<EOperation>(op), args) {};

AstOperator::AstOperator(const string& op, AstCtList* args) :
  AstOperator(toEOperation(op), args) {};

AstOperator::AstOperator(AstOperator::EOperation op, AstCtList* args) :
  m_referencedObj{},
  m_storageDuration{StorageDuration::eYetUndefined},
  m_accessFromAstParent{Access::eYetUndefined},
  m_op(op),
  m_args(args ? unique_ptr<AstCtList>(args) : make_unique<AstCtList>()) {
  if ( '-' == m_op || '+' == m_op ) {
    const auto argCnt = args->childs().size();
    assert( argCnt==1 || argCnt==2 );
  } else {
    const size_t required_arity =
      (op==eNot || op==eAddrOf || op==eDeref ) ? 1 : 2;
    assert( args->childs().size() == required_arity );
  }
}

AstOperator::AstOperator(char op, AstObject* operand1, AstObject* operand2) :
  AstOperator(static_cast<EOperation>(op), operand1, operand2) {};

AstOperator::AstOperator(const string& op, AstObject* operand1, AstObject* operand2) :
  AstOperator(toEOperation(op), operand1, operand2) {};

AstOperator::AstOperator(AstOperator::EOperation op, AstObject* operand1, AstObject* operand2) :
  AstOperator(op, new AstCtList(operand1, operand2)) {};

void AstOperator::setAccessFromAstParent(Access access) {
  assert(Access::eYetUndefined!=access);
  assert(Access::eYetUndefined==m_accessFromAstParent);
  m_accessFromAstParent = access;
  if ( m_referencedObj ) {
    m_referencedObj->addAccess(access);
  }
  addAccess(access);
}

Access AstOperator::accessFromAstParent() const {
  assert(Access::eYetUndefined!=m_accessFromAstParent);
  return m_accessFromAstParent;
}

Object& AstOperator::object() {
  return m_referencedObj ? *m_referencedObj : *this;
}

const Object& AstOperator::object() const {
  return m_referencedObj ? *m_referencedObj : static_cast<const Object&>(*this);
}

const ObjType& AstOperator::objType() const {
  assert(m_objType);
  return *m_objType;
}

std::shared_ptr<const ObjType> AstOperator::objTypeAsSp() const {
  return m_objType;
}

StorageDuration AstOperator::storageDuration() const {
  assert(m_storageDuration!=StorageDuration::eYetUndefined);
  return m_storageDuration;
}

void AstOperator::setReferencedObjAndPropagateAccess(unique_ptr<Object> object) {
  assert(!m_dummy); // it doesn't make sense to set it twice
  assert(object);
  setReferencedObjAndPropagateAccess(*object);
  m_dummy = move(object);
}

void AstOperator::setReferencedObjAndPropagateAccess(Object& object) {
  assert(!m_referencedObj); // it doesn't make sense to set it twice
  m_referencedObj = &object;
  assert(!m_objType);  // it doesn't make sense to set it twice
  m_objType = m_referencedObj->objTypeAsSp();
  assert(m_storageDuration==StorageDuration::eYetUndefined); // it doesn't make sense to set it twice
  m_storageDuration = m_referencedObj->storageDuration();
  m_referencedObj->addAccess(m_accessFromAstParent);
}

void AstOperator::setObjType(std::shared_ptr<const ObjType> objType) {
  assert(objType);
  assert(!m_objType);  // it doesn't make sense to set it twice
  m_objType = move(objType);
}

void AstOperator::setStorageDuration(StorageDuration storageDuration) {
  assert(storageDuration!=StorageDuration::eYetUndefined);
  assert(m_storageDuration==StorageDuration::eYetUndefined); // it doesn't make sense to set it twice
  m_storageDuration = storageDuration;
}

AstOperator::EClass AstOperator::class_() const {
  return classOf(m_op);
}

bool AstOperator::isBinaryLogicalShortCircuit() const {
  return m_op==eAnd || m_op==eOr;
}

AstOperator::EClass AstOperator::classOf(AstOperator::EOperation op) {
  switch (op) {
  case eVoidAssign:
  case eAssign:
    return eAssignment;

  case eAdd:
  case eSub:
  case eMul:
  case eDiv:
    return eArithmetic;

  case eNot:
  case eAnd:
  case eOr:
    return eLogical;

  case eEqualTo:
    return eComparison;

  case eAddrOf:
  case eDeref:
    return eMemberAccess;
  }
  assert(false);
  return eOther;
}

AstOperator::EOperation AstOperator::toEOperationPreferingBinary(const string& op) {
  if ( op=="*" ) {
    return eMul;
  } else {
    return toEOperation(op);
  }
}

AstOperator::EOperation AstOperator::toEOperation(const string& op) {
  if (op.size()==1) {
    assert(op[0]!='*'); // '*' is ambigous: either eMul or eDeref
    assert(op[0]!=';'); // ';' is not really an operator, use AstSeq
    return static_cast<EOperation>(op[0]);
  } else {
    auto i = m_opMap.find(op);
    assert( i != m_opMap.end() );
    return i->second;
  }
}

basic_ostream<char>& operator<<(basic_ostream<char>& os,
  AstOperator::EOperation op) {
  if (static_cast<int>(op)<128) {
    return os << static_cast<char>(op);
  } else {
    auto i = AstOperator::m_opReverseMap.find(op);
    assert( i != AstOperator::m_opReverseMap.end() );
    return os << i->second;
  }
}

AstSeq::AstSeq(std::vector<AstNode*>* operands) :
  m_accessFromAstParent{Access::eYetUndefined},
  m_operands(toUniquePtrs(operands)) {
  for ( const auto& operand : m_operands ) {
    assert(operand);
  }
  assert(!m_operands.empty());
}

AstSeq::AstSeq(AstNode* op1, AstNode* op2, AstNode* op3, AstNode* op4,
  AstNode* op5) :
  m_accessFromAstParent{Access::eYetUndefined},
  m_operands(toUniquePtrs(op1, op2, op3, op4, op5)) {
  assert(!m_operands.empty());
}

void AstSeq::setAccessFromAstParent(Access access) {
  assert(Access::eYetUndefined!=access);
  assert(Access::eYetUndefined==m_accessFromAstParent);
  m_accessFromAstParent = access;
  // 'object().addAccess(m_accessFromAstParent)' is done in
  // semanticanalizer. object() just delegates to lastoparand(), which
  // currently might not yet know its object().
}

Access AstSeq::accessFromAstParent() const {
  return m_accessFromAstParent;
}

Object& AstSeq::object() {
  return lastOperand().object();
}

const Object& AstSeq::object() const {
  return lastOperand().object();
}

bool AstSeq::lastOperandIsAnObject() const {
  return nullptr!=dynamic_cast<AstObject*>(m_operands.back().get());
}

AstObject& AstSeq::lastOperand() const {
  const auto obj = dynamic_cast<AstObject*>(m_operands.back().get());
  assert(obj);
  return *obj;
}

AstIf::AstIf(AstObject* cond, AstObject* action, AstObject* elseAction) :
  m_condition(cond),
  m_action(action),
  m_elseAction(elseAction) {
  assert(m_condition);
  assert(m_action);
}

const ObjType& AstIf::objType() const {
  assert(m_objType);
  return *m_objType;
}

std::shared_ptr<const ObjType> AstIf::objTypeAsSp() const {
  assert(m_objType);
  return m_objType;
}

StorageDuration AstIf::storageDuration() const {
  return StorageDuration::eLocal;
}

void AstIf::setObjectType(std::shared_ptr<const ObjType> objType) {
  assert(objType);
  assert(!m_objType); // doesn't make sense to set it twice
  m_objType = move(objType);
}

AstLoop::AstLoop(AstObject* cond, AstObject* body) :
  m_condition(cond),
  m_body(body) {
  assert(m_condition);
  assert(m_body);
}

const ObjType& AstLoop::objType() const {
  return *objTypeFundaVoid;
}

std::shared_ptr<const ObjType> AstLoop::objTypeAsSp() const {
  return objTypeFundaVoid;
}

StorageDuration AstLoop::storageDuration() const {
  return StorageDuration::eLocal;
}

AstReturn::AstReturn(AstObject* retVal) :
  m_retVal(retVal ? retVal : new AstNop()) {
  assert(m_retVal);
}

const ObjType& AstReturn::objType() const {
  return *objTypeFundaNoreturn;
}

std::shared_ptr<const ObjType> AstReturn::objTypeAsSp() const {
  return objTypeFundaNoreturn;
}

StorageDuration AstReturn::storageDuration() const {
  return StorageDuration::eLocal;
}

AstObject& AstReturn::retVal() const {
  assert(m_retVal);
  return *m_retVal.get();
}

AstFunCall::AstFunCall(AstObject* address, AstCtList* args) :
  m_address(address ? unique_ptr<AstObject>(address) : make_unique<AstSymbol>("")),
  m_args(args ? unique_ptr<AstCtList>(args) : make_unique<AstCtList>()) {
  assert(m_address);
  assert(m_args);
}

const ObjType& AstFunCall::objType() const {
  return *objTypeAsSp();
}

std::shared_ptr<const ObjType> AstFunCall::objTypeAsSp() const {
  const auto objTypeFun = std::dynamic_pointer_cast<const ObjTypeFun>(
    m_address->object().objTypeAsSp());
  assert(objTypeFun);
  return objTypeFun->ret().shared_from_this();
}

StorageDuration AstFunCall::storageDuration() const {
  return StorageDuration::eLocal;
}

AstObjTypeSymbol::AstObjTypeSymbol(const std::string name) :
  m_name(name) {
}

AstObjTypeSymbol::AstObjTypeSymbol(ObjType::EType fundaType) :
  m_name(toName(fundaType)) {
  assert(fundaType!=ObjType::ePointer);
}

const string& AstObjTypeSymbol::toName(ObjType::EType fundaType) {
  initMap();
  assert(fundaType!=ObjType::ePointer);
  return m_typeToName[fundaType];
}

ObjType::EType AstObjTypeSymbol::toType(const string& name) {
  initMap();
  const auto type = find(m_typeToName.begin(), m_typeToName.end(), name);
  assert(type!=m_typeToName.end());
  return static_cast<ObjType::EType>(type - m_typeToName.begin()); 
}

void AstObjTypeSymbol::initMap() {
  if ( !m_isMapInitialzied ) {
    m_isMapInitialzied = true;
    for ( int i = 0 ; i<ObjType::eTypeCnt; ++i ) {
      const auto type = static_cast<ObjType::EType>(i);
      if ( type==ObjType::ePointer ) {
        m_typeToName[i] = "<invalid(pointer)>";
      } else {
        m_typeToName[i] = ObjTypeFunda(type).toStr();
      }
    }
    for ( const auto& x : m_typeToName ) {
      assert(!x.empty());
    }
  }
}

array<string, ObjType::eTypeCnt> AstObjTypeSymbol::m_typeToName{};

bool AstObjTypeSymbol::m_isMapInitialzied = false;

void AstObjTypeSymbol::printValueTo(ostream& os, GeneralValue value) const {
  const auto type = toType(m_name);
  if ( type == ObjType::eChar) {
    os << "'" << char(value) << "'";
  } else if ( type == ObjType::eNullptr ) {
    os << "nullptr";
  } else {
    os << value;
    switch ( type ) {
    case ObjType::eBool: os << "bool"; break;
    case ObjType::eInt: break;
    case ObjType::eDouble: os << "d"; break;
    default: assert(false);
    }
  }
}

bool AstObjTypeSymbol::isValueInRange(GeneralValue value) const {
  switch (toType(m_name)) {
  case ObjType::eVoid: return false;
  case ObjType::eNoreturn: return false;
  case ObjType::eChar: return (0<=value && value<=0xFF) && (value==static_cast<int>(value));
  case ObjType::eInt: return (INT_MIN<=value && value<=INT_MAX) && (value==static_cast<int>(value));
  case ObjType::eDouble: return true;
  case ObjType::eBool: return value==0.0 || value==1.0;
  case ObjType::eNullptr: return value==0.0;
  case ObjType::ePointer: assert(false); // AstObjTypeSymbol does not handle ObjType::ePointer
  case ObjType::eTypeCnt: assert(false);
  };
  assert(false);
  return false;
}

AstObject* AstObjTypeSymbol::createDefaultAstObjectForSemanticAnalizer() {
  // What parser does
  const auto newAstNode = new AstNumber(0, new AstObjTypeSymbol(m_name));

  // What EnvInserter does: nothing

  // What SignatureAugmentor does:
  newAstNode->declaredAstObjType().createAndSetObjType();

  // What SemanticAnalizer does: to be done by caller
  return newAstNode;
}

llvm::Value* AstObjTypeSymbol::createLlvmValueFrom(GeneralValue value) const {
  switch (toType(m_name)) {
  case ObjType::eChar: // fall through
  case ObjType::eInt:  // fall through
  case ObjType::eBool:
    return llvm::ConstantInt::get( llvm::getGlobalContext(),
      llvm::APInt(objType().size(), value));
    break;
  case ObjType::eDouble:
    return llvm::ConstantFP::get( llvm::getGlobalContext(), llvm::APFloat(value));
    break;
  default:
    assert(false);
  }
  return nullptr;
}

const ObjType& AstObjTypeSymbol::objType() const {
  assert(m_objType);
  return *m_objType;
}

shared_ptr<const ObjType> AstObjTypeSymbol::objTypeAsSp() const {
  return m_objType;
}

void AstObjTypeSymbol::createAndSetObjType() {
  assert(!m_objType); // it doesn't make sense to set it twice
  m_objType = make_shared<ObjTypeFunda>(toType(m_name));
}

AstObjTypeQuali::AstObjTypeQuali(ObjType::Qualifiers qualifiers, AstObjType* targetType) :
  m_qualifiers(qualifiers),
  m_targetType(targetType) {
  assert(m_targetType);
}

void AstObjTypeQuali::printValueTo(ostream& os, GeneralValue value) const {
  m_targetType->printValueTo(os, value);
}

bool AstObjTypeQuali::isValueInRange(GeneralValue value) const {
  return m_targetType->isValueInRange(value);
}

AstObject* AstObjTypeQuali::createDefaultAstObjectForSemanticAnalizer() {
  return m_targetType->createDefaultAstObjectForSemanticAnalizer();
}

llvm::Value* AstObjTypeQuali::createLlvmValueFrom(GeneralValue value) const {
  return m_targetType->createLlvmValueFrom(value);
}

const ObjType& AstObjTypeQuali::objType() const {
  assert(m_objType);
  return *m_objType;
}

shared_ptr<const ObjType> AstObjTypeQuali::objTypeAsSp() const {
  return m_objType;
}

void AstObjTypeQuali::createAndSetObjType() {
  assert(m_targetType);
  assert(m_targetType->objTypeAsSp());
  assert(!m_objType); // it doesn't make sense to set it twice
  m_objType = make_shared<ObjTypeQuali>(m_qualifiers, m_targetType->objTypeAsSp());
}

AstObjTypePtr::AstObjTypePtr(AstObjType* pointee) :
  m_pointee(pointee) {
  assert(m_pointee);
}

void AstObjTypePtr::printValueTo(ostream& os, GeneralValue value) const {
  // not yet implemented
  assert(false);
}

bool AstObjTypePtr::isValueInRange(GeneralValue value) const {
  return (value<=UINT_MAX) && (value==static_cast<unsigned int>(value));
}

AstObject* AstObjTypePtr::createDefaultAstObjectForSemanticAnalizer() {
  // What parser does
  const auto newAstNode = new AstNumber(0, ObjType::eNullptr);

  // What EnvInserter does: nothing

  // What SignatureAugmentor does:
  newAstNode->declaredAstObjType().createAndSetObjType();

  // What SemanticAnalizer does: to be done by caller
  return newAstNode;
}

llvm::Value* AstObjTypePtr::createLlvmValueFrom(GeneralValue value) const {
  // not yet implemented
  assert(false);
}

const ObjTypePtr& AstObjTypePtr::objType() const {
  assert(m_objType);
  return *m_objType;
}

shared_ptr<const ObjType> AstObjTypePtr::objTypeAsSp() const {
  return m_objType;
}

void AstObjTypePtr::createAndSetObjType() {
  assert(m_pointee->objTypeAsSp());
  assert(!m_objType); // it doesn't make sense to set it twice
  m_objType = make_shared<ObjTypePtr>(m_pointee->objTypeAsSp());
}

/** The vectors's elements must be non-null */
AstCtList::AstCtList(vector<AstObject*>* childs) :
  m_childs(childs ? childs : new vector<AstObject*>() ) {
  assert(m_childs);
  for (auto it = m_childs->begin(); it != m_childs->end(); ++it) {
    assert(*it);
  }
}

/** When child is NULL it is ignored */
AstCtList::AstCtList(AstObject* child) :
  m_childs(new vector<AstObject*>()) {
  assert(m_childs);
  if (child) { m_childs->push_back(child); }
}

/** NULL childs are ignored.*/
AstCtList::AstCtList(AstObject* child1, AstObject* child2, AstObject* child3,
  AstObject* child4, AstObject* child5, AstObject* child6) :
  m_childs(new vector<AstObject*>()) {
  assert(m_childs);
  if (child1) { m_childs->push_back(child1); }
  if (child2) { m_childs->push_back(child2); }
  if (child3) { m_childs->push_back(child3); }
  if (child4) { m_childs->push_back(child4); }
  if (child5) { m_childs->push_back(child5); }
  if (child6) { m_childs->push_back(child6); }
}

AstCtList::~AstCtList() {
  if ( m_owner ) {
    for (auto i=m_childs->begin(); i!=m_childs->end(); ++i) {
      delete (*i);
    }
  }
  delete m_childs;
}

void AstCtList::releaseOwnership() {
  m_owner = false;
}

/** When child is NULL it is ignored */
AstCtList* AstCtList::Add(AstObject* child) {
  if (child) m_childs->push_back(child);
  return this;
}

/** NULL childs are ignored. */
AstCtList* AstCtList::Add(AstObject* child1, AstObject* child2, AstObject* child3) {
  Add(child1);
  Add(child2);
  Add(child3);
  return this;
}

void AstNop::accept(AstConstVisitor& visitor) const { visitor.visit(*this); }
void AstBlock::accept(AstConstVisitor& visitor) const { visitor.visit(*this); }
void AstCast::accept(AstConstVisitor& visitor) const { visitor.visit(*this); }
void AstFunDef::accept(AstConstVisitor& visitor) const { visitor.visit(*this); }
void AstDataDef::accept(AstConstVisitor& visitor) const { visitor.visit(*this); }
void AstNumber::accept(AstConstVisitor& visitor) const { visitor.visit(*this); }
void AstSymbol::accept(AstConstVisitor& visitor) const { visitor.visit(*this); }
void AstFunCall::accept(AstConstVisitor& visitor) const { visitor.visit(*this); }
void AstOperator::accept(AstConstVisitor& visitor) const { visitor.visit(*this); }
void AstSeq::accept(AstConstVisitor& visitor) const { visitor.visit(*this); }
void AstIf::accept(AstConstVisitor& visitor) const { visitor.visit(*this); }
void AstObjTypeSymbol::accept(AstConstVisitor& visitor) const { visitor.visit(*this); }
void AstObjTypeQuali::accept(AstConstVisitor& visitor) const { visitor.visit(*this); }
void AstObjTypePtr::accept(AstConstVisitor& visitor) const { visitor.visit(*this); }
void AstLoop::accept(AstConstVisitor& visitor) const { visitor.visit(*this); }
void AstReturn::accept(AstConstVisitor& visitor) const { visitor.visit(*this); }
void AstCtList::accept(AstConstVisitor& visitor) const { visitor.visit(*this); }

void AstNop::accept(AstVisitor& visitor) { visitor.visit(*this); }
void AstBlock::accept(AstVisitor& visitor) { visitor.visit(*this); }
void AstCast::accept(AstVisitor& visitor) { visitor.visit(*this); }
void AstFunDef::accept(AstVisitor& visitor) { visitor.visit(*this); }
void AstDataDef::accept(AstVisitor& visitor) { visitor.visit(*this); }
void AstNumber::accept(AstVisitor& visitor) { visitor.visit(*this); }
void AstSymbol::accept(AstVisitor& visitor) { visitor.visit(*this); }
void AstFunCall::accept(AstVisitor& visitor) { visitor.visit(*this); }
void AstOperator::accept(AstVisitor& visitor) { visitor.visit(*this); }
void AstSeq::accept(AstVisitor& visitor) { visitor.visit(*this); }
void AstIf::accept(AstVisitor& visitor) { visitor.visit(*this); }
void AstObjTypeSymbol::accept(AstVisitor& visitor) { visitor.visit(*this); }
void AstObjTypeQuali::accept(AstVisitor& visitor) { visitor.visit(*this); }
void AstObjTypePtr::accept(AstVisitor& visitor) { visitor.visit(*this); }
void AstLoop::accept(AstVisitor& visitor) { visitor.visit(*this); }
void AstReturn::accept(AstVisitor& visitor) { visitor.visit(*this); }
void AstCtList::accept(AstVisitor& visitor) { visitor.visit(*this); }
