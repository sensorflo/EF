#include "ast.h"

#include "astprinter.h"
#include "astvisitor.h"
#include "env.h"
#include "errorhandler.h"
#include "irgen.h"

#include <cassert>
#include <stdexcept>
#include <utility>

using namespace std;

namespace {

template<typename T>
vector<unique_ptr<T>> toUniquePtrs(vector<T*>* src) {
  assert(src);
  const unique_ptr<vector<T*>> dummy(src);
  vector<unique_ptr<T>> dst{};
  for (const auto& element : *src) { dst.emplace_back(element); }
  return dst;
}

template<typename T>
vector<unique_ptr<T>> toUniquePtrs(T* src1 = nullptr, T* src2 = nullptr,
  T* src3 = nullptr, T* src4 = nullptr, T* src5 = nullptr) {
  vector<unique_ptr<T>> dst{};
  for (const auto& element : {src1, src2, src3, src4, src5}) {
    if (element) { dst.emplace_back(element); }
  }
  return dst;
}

thread_local std::shared_ptr<const ObjTypeFunda> objTypeFundaVoid =
  make_shared<ObjTypeFunda>(ObjTypeFunda::eVoid);

thread_local std::shared_ptr<const ObjTypeFunda> objTypeFundaNoreturn =
  make_shared<ObjTypeFunda>(ObjTypeFunda::eNoreturn);
}

bool DisableLocationRequirement::m_areLocationsRequired = true;

AstNode::AstNode(Location loc) : m_loc{move(loc)} {
  if (DisableLocationRequirement::areLocationsRequired()) {
    assert(!m_loc.isNull());
  }
}

string AstNode::toStr() const {
  return AstPrinter::toStr(*this);
}

bool AstObject::isObjTypeNoReturn() const {
  return object().objType().isNoreturn();
}

AstObjInstance::AstObjInstance(Location loc)
  : AstObject(move(loc)), m_accessFromAstParent{Access::eYetUndefined} {
}

void AstObjInstance::setAccessFromAstParent(Access access) {
  assert(Access::eYetUndefined != access);
  assert(Access::eYetUndefined == m_accessFromAstParent);
  m_accessFromAstParent = access;
  addAccess(access);
}

Access AstObjInstance::accessFromAstParent() const {
  assert(Access::eYetUndefined != m_accessFromAstParent);
  return m_accessFromAstParent;
}

AstObjDef::AstObjDef(string name, Location loc)
  : AstObjInstance(move(loc)), EnvNode(move(name)) {
}

basic_ostream<char>& AstObjDef::printTo(basic_ostream<char>& os) const {
  const auto storageDuration_ = storageDuration();
  if (storageDuration_ != StorageDuration::eLocal) {
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

AstBlock::AstBlock(AstObject* body, Location loc)
  : AstObjInstance(move(loc))
  , EnvNode(Env::makeUniqueInternalName("$block"))
  , m_body(body) {
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

AstCast::AstCast(AstObjType* specifiedNewAstObjType, AstObject* child)
  : AstCast(
      specifiedNewAstObjType, new AstCtList(child ? child : new AstNumber(0))) {
}

AstCast::AstCast(
  AstObjType* specifiedNewAstObjType, AstCtList* args, Location loc)
  : AstObjInstance(move(loc))
  , m_specifiedNewAstObjType(specifiedNewAstObjType
        ? unique_ptr<AstObjType>(specifiedNewAstObjType)
        : make_unique<AstObjTypeSymbol>(ObjTypeFunda::eInfer))
  , m_args(args ? unique_ptr<AstCtList>(args) : make_unique<AstCtList>()) {
}

AstCast::AstCast(ObjTypeFunda::EType specifiedNewOjType, AstObject* child)
  : AstCast{new AstObjTypeSymbol(specifiedNewOjType), child} {
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

AstFunDef::AstFunDef(const string& name, std::vector<AstDataDef*>* args,
  AstObjType* ret, AstObject* body, Location loc)
  : AstObjDef(name, move(loc))
  , m_args(toUniquePtrs(args))
  , m_ret(ret)
  , m_body(body) {
  assert(m_ret);
  assert(m_body);
  for (const auto& arg : m_args) { assert(arg); }
}

vector<AstDataDef*>* AstFunDef::createArgs(
  AstDataDef* arg1, AstDataDef* arg2, AstDataDef* arg3) {
  auto args = new vector<AstDataDef*>;
  if (arg1) { args->push_back(arg1); }
  if (arg2) { args->push_back(arg2); }
  if (arg3) { args->push_back(arg3); }
  return args;
}

const ObjType& AstFunDef::objType() const {
  if (!m_objType) {
    objTypeAsSp(); // internally sets m_objType
  }
  return *m_objType;
}

std::shared_ptr<const ObjType> AstFunDef::objTypeAsSp() const {
  if (!m_objType) {
    const auto& argsObjType = new vector<shared_ptr<const ObjType>>;
    for (const auto& astArg : m_args) {
      assert(astArg);
      assert(astArg->declaredAstObjType().objTypeAsSp());
      argsObjType->push_back(astArg->declaredAstObjType().objTypeAsSp());
    }
    assert(m_ret->objTypeAsSp());
    m_objType =
      make_shared<const ObjTypeFun>(argsObjType, m_ret->objTypeAsSp());
  }
  return m_objType;
}

StorageDuration AstFunDef::storageDuration() const {
  return StorageDuration::eStatic;
}

AstObject* const AstDataDef::noInit = reinterpret_cast<AstObject*>(1);

AstDataDef::AstDataDef(const std::string& name, AstObjType* declaredAstObjType,
  StorageDuration declaredStorageDuration, AstCtList* ctorArgs, Location loc)
  : AstObjDef(name, loc)
  , m_declaredAstObjType(declaredAstObjType
        ? unique_ptr<AstObjType>(declaredAstObjType)
        : make_unique<AstObjTypeSymbol>(ObjTypeFunda::eInfer))
  , m_declaredStorageDuration(declaredStorageDuration)
  , m_ctorArgs(
      mkCtorArgs(ctorArgs, m_declaredStorageDuration, m_doNotInit, loc)) {
  assert(declaredStorageDuration != StorageDuration::eYetUndefined);
  assert(m_ctorArgs);
}

AstDataDef::AstDataDef(const std::string& name, AstObjType* declaredAstObjType,
  StorageDuration declaredStorageDuration, AstObject* initObj, Location loc)
  : AstDataDef(name, declaredAstObjType, declaredStorageDuration,
      initObj ? new AstCtList(loc, initObj) : nullptr, loc) {
}

AstDataDef::AstDataDef(const std::string& name, AstObjType* declaredAstObjType,
  AstObject* initObj, Location loc)
  : AstDataDef(
      name, declaredAstObjType, StorageDuration::eLocal, initObj, move(loc)) {
}

AstDataDef::AstDataDef(const std::string& name,
  ObjTypeFunda::EType declaredObjType, AstObject* initObj, Location loc)
  : AstDataDef(name, new AstObjTypeSymbol(declaredObjType, loc), initObj, loc) {
}

const ObjType& AstDataDef::objType() const {
  return m_declaredAstObjType->objType();
}

shared_ptr<const ObjType> AstDataDef::objTypeAsSp() const {
  return m_declaredAstObjType->objTypeAsSp();
}

unique_ptr<AstCtList> AstDataDef::mkCtorArgs(AstCtList* ctorArgs,
  StorageDuration storageDuration, bool& doNotInit, const Location& loc) {
  if (storageDuration == StorageDuration::eMember) {
    doNotInit = true;
    assert(nullptr == ctorArgs || ctorArgs->childs().empty());
    if (nullptr == ctorArgs) { return make_unique<AstCtList>(loc); }
    else {
      return unique_ptr<AstCtList>(ctorArgs);
    }
  }
  else {
    doNotInit = false; // assumption
    if (not ctorArgs) { ctorArgs = new AstCtList(loc); }
    if (not ctorArgs->childs().empty()) {
      if (ctorArgs->childs().front() == noInit) {
        doNotInit = true;
        assert(ctorArgs->childs().size() == 1);
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

AstNumber::AstNumber(GeneralValue value, AstObjType* astObjType, Location loc)
  : AstObjInstance(loc)
  , m_value(value)
  , m_declaredAstObjType(
      astObjType ? astObjType : new AstObjTypeSymbol(ObjTypeFunda::eInt, loc)) {
  assert(m_declaredAstObjType);
}

AstNumber::AstNumber(
  GeneralValue value, ObjTypeFunda::EType eType, Location loc)
  : AstNumber(value, new AstObjTypeSymbol(eType, loc), loc) {
}

AstObjType& AstNumber::declaredAstObjType() const {
  assert(m_declaredAstObjType);
  return *m_declaredAstObjType;
}

const ObjType& AstNumber::objType() const {
  return m_declaredAstObjType->objType();
}

std::shared_ptr<const ObjType> AstNumber::objTypeAsSp() const {
  return m_declaredAstObjType->objTypeAsSp();
}

StorageDuration AstNumber::storageDuration() const {
  return StorageDuration::eLocal;
}

void AstSymbol::setAccessFromAstParent(Access access) {
  assert(Access::eYetUndefined != access);
  assert(Access::eYetUndefined == m_accessFromAstParent);
  m_accessFromAstParent = access;
  if (m_referencedAstObj) { m_referencedAstObj->addAccess(access); }
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

void AstSymbol::setreferencedAstObjAndPropagateAccess(
  AstObjDef& referencedAstObj) {
  assert(!m_referencedAstObj); // it doesnt make sense to set it twice
  m_referencedAstObj = &referencedAstObj;
  m_referencedAstObj->addAccess(accessFromAstParent());
}

bool AstSymbol::isInitialized() const {
  assert(m_referencedAstObj);
  return m_referencedAstObj->isInitialized();
}

const map<const string, const AstOperator::EOperation> AstOperator::m_opMap{
  {"and", eAnd}, {"&&", eAnd}, {"or", eOr}, {"||", eOr}, {"==", eEqualTo},
  {"=<", eAssign}, {"not", eNot}};

// in case of ambiguity, prefer one letters over symbols. Also note that
// single char operators are prefered over multi chars; the single chars are
// not even in the map.
const map<const AstOperator::EOperation, const std::string>
  AstOperator::m_opReverseMap{{eAnd, "and"}, {eOr, "or"}, {eEqualTo, "=="},
    {eAssign, "=<"}, {eDeref, "*"}};

AstOperator::AstOperator(char op, AstCtList* args, Location loc)
  : AstOperator(static_cast<EOperation>(op), args, move(loc)) {
}

AstOperator::AstOperator(const string& op, AstCtList* args, Location loc)
  : AstOperator(toEOperation(op), args, move(loc)) {
}

AstOperator::AstOperator(
  AstOperator::EOperation op, AstCtList* args, Location loc)
  : AstObject{move(loc)}
  , m_referencedObj{}
  , m_accessFromAstParent{Access::eYetUndefined}
  , m_storageDuration{StorageDuration::eYetUndefined}
  , m_op(op)
  , m_args(args ? unique_ptr<AstCtList>(args) : make_unique<AstCtList>()) {
  if ('-' == m_op || '+' == m_op) {
    const auto argCnt = m_args->childs().size();
    assert(argCnt == 1 || argCnt == 2);
  }
  else {
    const size_t required_arity =
      (op == eNot || op == eAddrOf || op == eDeref) ? 1 : 2;
    assert(m_args->childs().size() == required_arity);
  }
}

AstOperator::AstOperator(
  char op, AstObject* operand1, AstObject* operand2, Location loc)
  : AstOperator(static_cast<EOperation>(op), operand1, operand2, move(loc)) {
}

AstOperator::AstOperator(
  const string& op, AstObject* operand1, AstObject* operand2, Location loc)
  : AstOperator(toEOperation(op), operand1, operand2, move(loc)) {
}

AstOperator::AstOperator(AstOperator::EOperation op, AstObject* operand1,
  AstObject* operand2, Location loc)
  : AstOperator(op, new AstCtList(loc, operand1, operand2), loc) {
}

void AstOperator::setAccessFromAstParent(Access access) {
  assert(Access::eYetUndefined != access);
  assert(Access::eYetUndefined == m_accessFromAstParent);
  m_accessFromAstParent = access;
  if (m_referencedObj) { m_referencedObj->addAccess(access); }
  addAccess(access);
}

Access AstOperator::accessFromAstParent() const {
  assert(Access::eYetUndefined != m_accessFromAstParent);
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
  assert(m_storageDuration != StorageDuration::eYetUndefined);
  return m_storageDuration;
}

void AstOperator::setReferencedObjAndPropagateAccess(
  unique_ptr<Object> object) {
  assert(!m_dummy); // it doesn't make sense to set it twice
  assert(object);
  setReferencedObjAndPropagateAccess(*object);
  m_dummy = move(object);
}

void AstOperator::setReferencedObjAndPropagateAccess(Object& object) {
  assert(!m_referencedObj); // it doesn't make sense to set it twice
  m_referencedObj = &object;
  assert(!m_objType); // it doesn't make sense to set it twice
  m_objType = m_referencedObj->objTypeAsSp();
  assert(m_storageDuration ==
    StorageDuration::eYetUndefined); // it doesn't make sense to set it twice
  m_storageDuration = m_referencedObj->storageDuration();
  m_referencedObj->addAccess(m_accessFromAstParent);
}

void AstOperator::setObjType(std::shared_ptr<const ObjType> objType) {
  assert(objType);
  assert(!m_objType); // it doesn't make sense to set it twice
  m_objType = move(objType);
}

void AstOperator::setStorageDuration(StorageDuration storageDuration) {
  assert(storageDuration != StorageDuration::eYetUndefined);
  assert(m_storageDuration ==
    StorageDuration::eYetUndefined); // it doesn't make sense to set it twice
  m_storageDuration = storageDuration;
}

AstOperator::EClass AstOperator::class_() const {
  return classOf(m_op);
}

bool AstOperator::isBinaryLogicalShortCircuit() const {
  return m_op == eAnd || m_op == eOr;
}

AstOperator::EClass AstOperator::classOf(AstOperator::EOperation op) {
  switch (op) {
  case eVoidAssign:
  case eAssign: return eAssignment;

  case eAdd:
  case eSub:
  case eMul:
  case eDiv: return eArithmetic;

  case eNot:
  case eAnd:
  case eOr: return eLogical;

  case eEqualTo: return eComparison;

  case eAddrOf:
  case eDeref: return eMemberAccess;
  }
  assert(false);
  return eOther;
}

AstOperator::EOperation AstOperator::toEOperationPreferingBinary(
  const string& op) {
  if (op == "*") { return eMul; }
  else {
    return toEOperation(op);
  }
}

AstOperator::EOperation AstOperator::toEOperation(const string& op) {
  if (op.size() == 1) {
    assert(op[0] != '*'); // '*' is ambigous: either eMul or eDeref
    assert(op[0] != ';'); // ';' is not really an operator, use AstSeq
    return static_cast<EOperation>(op[0]);
  }
  else {
    auto i = m_opMap.find(op);
    assert(i != m_opMap.end());
    return i->second;
  }
}

basic_ostream<char>& operator<<(
  basic_ostream<char>& os, AstOperator::EOperation op) {
  if (static_cast<int>(op) < 128) { return os << static_cast<char>(op); }
  else {
    auto i = AstOperator::m_opReverseMap.find(op);
    assert(i != AstOperator::m_opReverseMap.end());
    return os << i->second;
  }
}

namespace {
/** Returns the location of the last operand in the sequence, or s_nullLoc if
that doesn't exist. */
Location locationOf(std::vector<AstNode*>* operands) {
  if (operands == nullptr || operands->empty() || operands->back() == nullptr) {
    return s_nullLoc;
  }
  else {
    return operands->back()->loc();
  }
}
}

AstSeq::AstSeq(std::vector<AstNode*>* operands)
  : AstObject{locationOf(operands)}
  , m_accessFromAstParent{Access::eYetUndefined}
  , m_operands(toUniquePtrs(operands)) {
  for (const auto& operand : m_operands) { assert(operand); }
  assert(!m_operands.empty());
}

AstSeq::AstSeq(
  AstNode* op1, AstNode* op2, AstNode* op3, AstNode* op4, AstNode* op5)
  : m_accessFromAstParent{Access::eYetUndefined}
  , m_operands(toUniquePtrs(op1, op2, op3, op4, op5)) {
  assert(!m_operands.empty());
}

void AstSeq::setAccessFromAstParent(Access access) {
  assert(Access::eYetUndefined != access);
  assert(Access::eYetUndefined == m_accessFromAstParent);
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
  return nullptr != dynamic_cast<AstObject*>(m_operands.back().get());
}

AstObject& AstSeq::lastOperand() const {
  const auto obj = dynamic_cast<AstObject*>(m_operands.back().get());
  assert(obj);
  return *obj;
}

AstIf::AstIf(
  AstObject* cond, AstObject* action, AstObject* elseAction, Location loc)
  : AstObjInstance{move(loc)}
  , m_condition(cond)
  , m_action(action)
  , m_elseAction(elseAction) {
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

AstLoop::AstLoop(AstObject* cond, AstObject* body, Location loc)
  : AstObjInstance{move(loc)}, m_condition(cond), m_body(body) {
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

AstReturn::AstReturn(AstObject* retVal, Location loc)
  : AstReturn(new AstCtList(retVal ? retVal : new AstNop(loc)), loc) {
}

AstReturn::AstReturn(AstCtList* args, Location loc)
  : AstObjInstance{loc}
  , m_ctorArgs(
      args ? unique_ptr<AstCtList>(args) : make_unique<AstCtList>(loc)) {
  if (m_ctorArgs->childs().empty()) { m_ctorArgs->Add(new AstNop(loc)); }
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

AstFunCall::AstFunCall(AstObject* address, AstCtList* args, Location loc)
  : AstObjInstance{move(loc)}
  , m_address(
      address ? unique_ptr<AstObject>(address) : make_unique<AstSymbol>(""))
  , m_args(args ? unique_ptr<AstCtList>(args) : make_unique<AstCtList>()) {
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
  return objTypeFun->ret().unqualifiedObjType();
}

StorageDuration AstFunCall::storageDuration() const {
  return StorageDuration::eLocal;
}

AstObjTypeSymbol::AstObjTypeSymbol(const std::string name, Location loc)
  : AstObjType{move(loc)}, m_name(name) {
}

AstObjTypeSymbol::AstObjTypeSymbol(ObjTypeFunda::EType fundaType, Location loc)
  : AstObjType{move(loc)}, m_name(toName(fundaType)) {
  assert(fundaType != ObjTypeFunda::ePointer);
}

string AstObjTypeSymbol::toName(ObjTypeFunda::EType fundaType) {
  initMap();
  assert(fundaType != ObjTypeFunda::ePointer);
  return m_typeToName[fundaType];
}

ObjTypeFunda::EType AstObjTypeSymbol::toType(const string& name) {
  initMap();
  const auto type = find(m_typeToName.begin(), m_typeToName.end(), name);
  assert(type != m_typeToName.end());
  return static_cast<ObjTypeFunda::EType>(type - m_typeToName.begin());
}

void AstObjTypeSymbol::initMap() {
  if (!m_isMapInitialzied) {
    m_isMapInitialzied = true;
    for (int i = 0; i < ObjTypeFunda::eTypeCnt; ++i) {
      const auto type = static_cast<ObjTypeFunda::EType>(i);
      if (type == ObjTypeFunda::ePointer) {
        m_typeToName[i] = "<invalid(pointer)>";
      }
      else {
        m_typeToName[i] = ObjTypeFunda(type).toStr();
      }
    }
    for (const auto& x : m_typeToName) { assert(!x.empty()); }
  }
}

array<string, ObjTypeFunda::eTypeCnt> AstObjTypeSymbol::m_typeToName{};

bool AstObjTypeSymbol::m_isMapInitialzied = false;

void AstObjTypeSymbol::printValueTo(ostream& os, GeneralValue value) const {
  const auto type = toType(m_name);
  if (type == ObjTypeFunda::eChar) { os << "'" << char(value) << "'"; }
  else if (type == ObjTypeFunda::eNullptr) {
    os << "nullptr";
  }
  else {
    os << value;
    switch (type) {
    case ObjTypeFunda::eBool: os << "bool"; break;
    case ObjTypeFunda::eInt: break;
    case ObjTypeFunda::eDouble: os << "d"; break;
    default: assert(false);
    }
  }
}

bool AstObjTypeSymbol::isValueInRange(GeneralValue value) const {
  switch (toType(m_name)) {
  case ObjTypeFunda::eVoid: return false;
  case ObjTypeFunda::eNoreturn: return false;
  case ObjTypeFunda::eInfer: return false;
  case ObjTypeFunda::eChar:
    return (0 <= value && value <= 0xFF) && (value == static_cast<int>(value));
  case ObjTypeFunda::eInt:
    return (INT_MIN <= value && value <= INT_MAX) &&
      (value == static_cast<int>(value));
  case ObjTypeFunda::eDouble: return true;
  case ObjTypeFunda::eBool: return value == 0.0 || value == 1.0;
  case ObjTypeFunda::eNullptr: return value == 0.0;
  case ObjTypeFunda::ePointer:
    assert(false); // AstObjTypeSymbol does not handle ObjTypeFunda::ePointer
  case ObjTypeFunda::eTypeCnt: assert(false);
  };
  assert(false);
  return false;
}

AstObject* AstObjTypeSymbol::createDefaultAstObjectForSemanticAnalizer(
  Location loc) const {
  // What parser does
  const auto newAstNode =
    new AstNumber(0, new AstObjTypeSymbol(m_name, loc), loc);

  // What EnvInserter does: nothing

  // What SignatureAugmentor does:
  newAstNode->declaredAstObjType().createAndSetObjType();

  // What SemanticAnalizer does: to be done by caller
  return newAstNode;
}

llvm::Value* AstObjTypeSymbol::createLlvmValueFrom(GeneralValue value) const {
  switch (toType(m_name)) {
  case ObjTypeFunda::eChar: // fall through
  case ObjTypeFunda::eInt: // fall through
  case ObjTypeFunda::eBool:
    return llvm::ConstantInt::get(
      llvmContext, llvm::APInt(objType().size(), value));
    break;
  case ObjTypeFunda::eDouble:
    return llvm::ConstantFP::get(llvmContext, llvm::APFloat(value));
    break;
  default: assert(false);
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

AstObjTypeQuali::AstObjTypeQuali(
  ObjType::Qualifiers qualifiers, AstObjType* targetType, Location loc)
  : AstObjType{move(loc)}, m_qualifiers(qualifiers), m_targetType(targetType) {
  assert(m_targetType);
}

void AstObjTypeQuali::printValueTo(ostream& os, GeneralValue value) const {
  m_targetType->printValueTo(os, value);
}

bool AstObjTypeQuali::isValueInRange(GeneralValue value) const {
  return m_targetType->isValueInRange(value);
}

AstObject* AstObjTypeQuali::createDefaultAstObjectForSemanticAnalizer(
  Location loc) const {
  return m_targetType->createDefaultAstObjectForSemanticAnalizer(move(loc));
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
  m_objType =
    make_shared<ObjTypeQuali>(m_qualifiers, m_targetType->objTypeAsSp());
}

AstObjTypePtr::AstObjTypePtr(AstObjType* pointee, Location loc)
  : AstObjType{move(loc)}, m_pointee(pointee) {
  assert(m_pointee);
}

void AstObjTypePtr::printValueTo(ostream& os, GeneralValue value) const {
  // not yet implemented
  assert(false);
}

bool AstObjTypePtr::isValueInRange(GeneralValue value) const {
  return (value <= UINT_MAX) && (value == static_cast<unsigned int>(value));
}

AstObject* AstObjTypePtr::createDefaultAstObjectForSemanticAnalizer(
  Location loc) const {
  // What parser does
  const auto newAstNode = new AstNumber(0, ObjTypeFunda::eNullptr, move(loc));

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

AstClassDef::AstClassDef(
  std::string name, std::vector<AstDataDef*>* dataMembers, Location loc)
  : AstObjType{move(loc)}
  , m_name(std::move(name))
  , m_dataMembers(toUniquePtrs(dataMembers)) {
  for (const auto& dataMember : m_dataMembers) { assert(dataMember); }
}

AstClassDef::AstClassDef(
  std::string name, AstDataDef* m1, AstDataDef* m2, AstDataDef* m3)
  : m_name(std::move(name)), m_dataMembers(toUniquePtrs(m1, m2, m3)) {
}

void AstClassDef::printValueTo(ostream& os, GeneralValue value) const {
  // The liskov substitution principle is broken here
  assert(false);
}

bool AstClassDef::isValueInRange(GeneralValue value) const {
  // The liskov substitution principle is broken here
  assert(false);
}

AstObject* AstClassDef::createDefaultAstObjectForSemanticAnalizer(
  Location) const {
  // The liskov substitution principle is broken here
  assert(false);
}

llvm::Value* AstClassDef::createLlvmValueFrom(GeneralValue value) const {
  // not yet implemented
  assert(false);
}

const ObjTypeClass& AstClassDef::objType() const {
  assert(m_objType);
  return *m_objType;
}

shared_ptr<const ObjType> AstClassDef::objTypeAsSp() const {
  return m_objType;
}

void AstClassDef::createAndSetObjType() {
  assert(!m_objType); // it doesn't make sense to set it twice
  std::vector<std::shared_ptr<const ObjType>> dataMembersCopy;
  for (const auto& dataMember : m_dataMembers) {
    assert(dataMember);
    assert(dataMember->objTypeAsSp());
    dataMembersCopy.emplace_back(dataMember->objTypeAsSp());
  }
  m_objType = make_shared<ObjTypeClass>(m_name, move(dataMembersCopy));
}

/** The vectors's elements must be non-null */
AstCtList::AstCtList(vector<AstObject*>* childs, Location loc)
  : AstNode{move(loc)}, m_childs(childs ? childs : new vector<AstObject*>()) {
  assert(m_childs);
  for (const auto& child : *m_childs) { assert(child); }
}

/** nullptr childs are ignored.*/
AstCtList::AstCtList(Location loc, AstObject* child1, AstObject* child2)
  : AstCtList(nullptr, move(loc)) {
  if (child1) { m_childs->push_back(child1); }
  if (child2) { m_childs->push_back(child2); }
}

/** nullptr childs are ignored.*/
AstCtList::AstCtList(AstObject* child1, AstObject* child2, AstObject* child3,
  AstObject* child4, AstObject* child5, AstObject* child6)
  : AstCtList(Location{}, child1, child2) {
  if (child3) { m_childs->push_back(child3); }
  if (child4) { m_childs->push_back(child4); }
  if (child5) { m_childs->push_back(child5); }
  if (child6) { m_childs->push_back(child6); }
}

AstCtList::~AstCtList() {
  if (m_owner) {
    for (const auto& child : *m_childs) { delete (child); }
  }
  delete m_childs;
}

void AstCtList::releaseOwnership() {
  m_owner = false;
}

/** When child is nullptr it is ignored */
AstCtList* AstCtList::Add(AstObject* child) {
  if (child) m_childs->push_back(child);
  return this;
}

/** nullptr childs are ignored. */
AstCtList* AstCtList::Add(
  AstObject* child1, AstObject* child2, AstObject* child3) {
  Add(child1);
  Add(child2);
  Add(child3);
  return this;
}

// clang-format off

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
void AstClassDef::accept(AstConstVisitor& visitor) const { visitor.visit(*this); }
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
void AstClassDef::accept(AstVisitor& visitor) { visitor.visit(*this); }
void AstLoop::accept(AstVisitor& visitor) { visitor.visit(*this); }
void AstReturn::accept(AstVisitor& visitor) { visitor.visit(*this); }
void AstCtList::accept(AstVisitor& visitor) { visitor.visit(*this); }

// clang-format on
