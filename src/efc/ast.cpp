#include "ast.h"

#include "astprinter.h"
#include "astvisitor.h"
#include "env.h"
#include "errorhandler.h"
#include "irgen.h"

#include <cassert>
#include <sstream>
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

thread_local shared_ptr<const ObjTypeFunda> objTypeFundaVoid =
  make_shared<ObjTypeFunda>(ObjTypeFunda::eVoid);

thread_local shared_ptr<const ObjTypeFunda> objTypeFundaNoreturn =
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

AstObject::AstObject(Location loc)
  : AstNode{move(loc)}, m_accessFromAstParent{Access::eYetUndefined} {
}

bool AstObject::isObjTypeNoReturn() const {
  return objType().isNoreturn();
}

void AstObject::setAccessFromAstParent(Access access) {
  assert(Access::eYetUndefined != access);
  assert(Access::eYetUndefined == m_accessFromAstParent);
  m_accessFromAstParent = access;
  addAccess(access);
}

Access AstObject::accessFromAstParent() const {
  assert(Access::eYetUndefined != m_accessFromAstParent);
  return m_accessFromAstParent;
}

AstObjDef::AstObjDef(Location loc) : AstObject(move(loc)) {
}

basic_ostream<char>& AstObjDef::printTo(basic_ostream<char>& os) const {
  const auto storageDuration_ = storageDuration();
  if (storageDuration_ != StorageDuration::eLocal) {
    os << storageDuration_ << "/";
  }
  os << objType();
  return os;
}

AstNop::AstNop(Location loc)
  : AstObject{std::move(loc)}
  , FullConcreteObject{objTypeFundaVoid, StorageDuration::eLocal} {
}

AstBlock::AstBlock(AstObject* body, Location loc)
  : Object{Env::makeUniqueInternalName("$block")}
  , AstObject{move(loc)}
  , m_body{body} {
  assert(m_body);
}

const ObjType& AstBlock::objType() const {
  return *objTypeAsSp();
}

std::shared_ptr<const ObjType> AstBlock::objTypeAsSp() const {
  return m_body->objType().unqualifiedObjType();
}

StorageDuration AstBlock::storageDuration() const {
  return StorageDuration::eLocal;
}

AstCast::AstCast(AstObjType* specifiedNewAstObjType, AstObject* arg)
  : AstCast{specifiedNewAstObjType,
      new AstCtList{arg != nullptr ? arg : new AstNumber{0}}} {
}

AstCast::AstCast(
  AstObjType* specifiedNewAstObjType, AstCtList* args, Location loc)
  : AstObject{move(loc)}
  , m_specifiedNewAstObjType{specifiedNewAstObjType != nullptr
        ? unique_ptr<AstObjType>{specifiedNewAstObjType}
        : make_unique<AstObjTypeSymbol>(ObjTypeFunda::eInfer)}
  , m_args{args != nullptr ? unique_ptr<AstCtList>{args}
                           : make_unique<AstCtList>()} {
}

AstCast::AstCast(ObjTypeFunda::EType specifiedNewOjType, AstObject* child)
  : AstCast{new AstObjTypeSymbol{specifiedNewOjType}, child} {
}

AstObjType& AstCast::specifiedNewAstObjType() const {
  return *m_specifiedNewAstObjType;
}

const ObjType& AstCast::objType() const {
  return m_specifiedNewAstObjType->objType();
}

shared_ptr<const ObjType> AstCast::objTypeAsSp() const {
  return m_specifiedNewAstObjType->objTypeAsSp();
}

StorageDuration AstCast::storageDuration() const {
  return StorageDuration::eLocal;
}

AstFunDef::AstFunDef(const string& name, vector<AstDataDef*>* args,
  AstObjType* ret, AstObject* body, Location loc)
  : Object{name}
  , AstObjDef{move(loc)}
  , m_args{toUniquePtrs(args)}
  , m_ret{ret}
  , m_body{body} {
  assert(m_ret);
  assert(m_body);
  for (const auto& arg : m_args) { assert(arg); }
}

std::string AstFunDef::description() const {
  return "function definition of " + fqName() + " defined here '" +
    toString(loc()) + "'";
}

vector<AstDataDef*>* AstFunDef::createArgs(
  AstDataDef* arg1, AstDataDef* arg2, AstDataDef* arg3) {
  auto args = new vector<AstDataDef*>{};
  if (arg1 != nullptr) { args->push_back(arg1); }
  if (arg2 != nullptr) { args->push_back(arg2); }
  if (arg3 != nullptr) { args->push_back(arg3); }
  return args;
}

const ObjType& AstFunDef::objType() const {
  if (!m_objType) {
    objTypeAsSp(); // internally sets m_objType
  }
  return *m_objType;
}

shared_ptr<const ObjType> AstFunDef::objTypeAsSp() const {
  if (!m_objType) {
    const auto& argsObjType = new vector<shared_ptr<const ObjType>>{};
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

std::vector<std::unique_ptr<AstDataDef>> const&
  AstFunDef::declaredArgs() const {
  return m_args;
}

AstObjType& AstFunDef::ret() const {
  assert(m_ret);
  return *m_ret;
}

AstObject& AstFunDef::body() const {
  assert(m_body);
  return *m_body;
}

AstObject* const AstDataDef::noInit = reinterpret_cast<AstObject*>(1);

AstDataDef::AstDataDef(const string& name, AstObjType* declaredAstObjType,
  StorageDuration declaredStorageDuration, AstCtList* ctorArgs, Location loc)
  : Object{name}
  , AstObjDef{loc}
  , m_doNotInit{false} // conceptually initialized by mkCtorArgs below
  , m_declaredAstObjType{declaredAstObjType != nullptr
        ? unique_ptr<AstObjType>{declaredAstObjType}
        : make_unique<AstObjTypeSymbol>(ObjTypeFunda::eInfer)}
  , m_declaredStorageDuration{declaredStorageDuration}
  , m_ctorArgs{
      mkCtorArgs(ctorArgs, m_declaredStorageDuration, m_doNotInit, loc)} {
  assert(declaredStorageDuration != StorageDuration::eYetUndefined);
  assert(m_ctorArgs);
}

AstDataDef::AstDataDef(const string& name, AstObjType* declaredAstObjType,
  StorageDuration declaredStorageDuration, AstObject* initObj, Location loc)
  : AstDataDef{name, declaredAstObjType, declaredStorageDuration,
      initObj != nullptr ? new AstCtList{loc, initObj} : nullptr, loc} {
}

AstDataDef::AstDataDef(const string& name, AstObjType* declaredAstObjType,
  AstObject* initObj, Location loc)
  : AstDataDef{
      name, declaredAstObjType, StorageDuration::eLocal, initObj, move(loc)} {
}

AstDataDef::AstDataDef(const string& name, ObjTypeFunda::EType declaredObjType,
  AstObject* initObj, Location loc)
  : AstDataDef{name, new AstObjTypeSymbol{declaredObjType, loc}, initObj, loc} {
}

std::string AstDataDef::description() const {
  return "data definition of " + fqName() + " defined here '" +
    toString(loc()) + "'";
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
    return unique_ptr<AstCtList>{ctorArgs};
  }

  doNotInit = false; // assumption
  if (ctorArgs == nullptr) { ctorArgs = new AstCtList{loc}; }
  if (not ctorArgs->childs().empty()) {
    if (ctorArgs->childs().front() == noInit) {
      doNotInit = true;
      assert(ctorArgs->childs().size() == 1);
      ctorArgs->childs().clear();
    }
  }

  return unique_ptr<AstCtList>{ctorArgs};
}

AstObjType& AstDataDef::declaredAstObjType() const {
  return *m_declaredAstObjType;
}

StorageDuration AstDataDef::declaredStorageDuration() const {
  return m_declaredStorageDuration;
}

AstNumber::AstNumber(GeneralValue value, AstObjType* astObjType, Location loc)
  : AstObject{loc}
  , m_value{value}
  , m_declaredAstObjType{astObjType != nullptr
        ? astObjType
        : new AstObjTypeSymbol{ObjTypeFunda::eInt, loc}} {
  assert(m_declaredAstObjType);
}

AstNumber::AstNumber(
  GeneralValue value, ObjTypeFunda::EType eType, Location loc)
  : AstNumber{value, new AstObjTypeSymbol{eType, loc}, loc} {
}

AstObjType& AstNumber::declaredAstObjType() const {
  assert(m_declaredAstObjType);
  return *m_declaredAstObjType;
}

const ObjType& AstNumber::objType() const {
  return m_declaredAstObjType->objType();
}

shared_ptr<const ObjType> AstNumber::objTypeAsSp() const {
  return m_declaredAstObjType->objTypeAsSp();
}

StorageDuration AstNumber::storageDuration() const {
  return StorageDuration::eLocal;
}

AstSymbol::AstSymbol(std::string name, Location loc)
  : AstObject{std::move(loc)}
  , m_referencedObj{nullptr}
  , m_name{std::move(name)} {
}

void AstSymbol::addAccess(Access access) {
  // do it the normal way if possible
  if (m_referencedObj != nullptr) { ObjectDelegate::addAccess(access); }
  else {
    // Not possible now since we don't know yet the referenced object. Will do
    // it later in setreferencedObjAndPropagateAccess
  }
}

Object& AstSymbol::referencedObj() const {
  assert(m_referencedObj != nullptr);
  return *m_referencedObj;
}

void AstSymbol::setreferencedObjAndPropagateAccess(AstObjDef& referencedObj) {
  assert(!m_referencedObj); // it doesnt make sense to set it twice
  m_referencedObj = &referencedObj;
  m_referencedObj->addAccess(accessFromAstParent());
}

bool AstSymbol::isInitialized() const {
  assert(m_referencedObj);
  return m_referencedObj->isInitialized();
}

const map<const string, const AstOperator::EOperation> AstOperator::m_opMap{
  {"and", eAnd}, {"&&", eAnd}, {"or", eOr}, {"||", eOr}, {"==", eEqualTo},
  {"=<", eAssign}, {"not", eNot}};

// in case of ambiguity, prefer one letters over symbols. Also note that
// single char operators are prefered over multi chars; the single chars are
// not even in the map.
const map<const AstOperator::EOperation, const string>
  AstOperator::m_opReverseMap{{eAnd, "and"}, {eOr, "or"}, {eEqualTo, "=="},
    {eAssign, "=<"}, {eDeref, "*"}};

AstOperator::AstOperator(char op, AstCtList* args, Location loc)
  : AstOperator{static_cast<EOperation>(op), args, move(loc)} {
}

AstOperator::AstOperator(const string& op, AstCtList* args, Location loc)
  : AstOperator{toEOperation(op), args, move(loc)} {
}

AstOperator::AstOperator(
  AstOperator::EOperation op, AstCtList* args, Location loc)
  : AstObject{move(loc)}
  , m_referencedObj{nullptr}
  , m_op{op}
  , m_args{args != nullptr ? unique_ptr<AstCtList>{args}
                           : make_unique<AstCtList>()} {
  if (!returnsByRef()) {
    m_obj = make_unique<FullConcreteObject>();
    m_referencedObj = m_obj.get();
    m_obj->m_storageDuration = StorageDuration::eLocal;

    if (m_args->childs().empty()) {
      // nop - it's a semantic error if there are no arguments, but it's the job
      // of the SemanticAnalizer to report an error.
    }
    else if (class_() == AstOperator::eComparison) {
      m_obj->m_objType = std::make_unique<ObjTypeFunda>(ObjTypeFunda::eBool);
    }
    else if (m_op == AstOperator::eVoidAssign) {
      m_obj->m_objType = make_shared<ObjTypeFunda>(ObjTypeFunda::eVoid);
    }
    else {
      // nop - m_obj->m_objType is dependend on the object type of childs, but
      // that might not be known yet. SemanticAnalizer will set
      // m_obj->m_objType.
    }
  }
  else {
    // nop - settig m_referencedObj to the proper object is done later by
    // SemanticAnalizer
  }
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
  : AstOperator{static_cast<EOperation>(op), operand1, operand2, move(loc)} {
}

AstOperator::AstOperator(
  const string& op, AstObject* operand1, AstObject* operand2, Location loc)
  : AstOperator{toEOperation(op), operand1, operand2, move(loc)} {
}

AstOperator::AstOperator(AstOperator::EOperation op, AstObject* operand1,
  AstObject* operand2, Location loc)
  : AstOperator{op, new AstCtList{loc, operand1, operand2}, loc} {
}

void AstOperator::addAccess(Access access) {
  // do it the normal way if possible
  if (m_referencedObj != nullptr) { ObjectDelegate::addAccess(access); }
  else {
    // Not possible now since we don't know yet the referenced object. Will do
    // it later in setreferencedObjAndPropagateAccess
  }
}

Object& AstOperator::referencedObj() const {
  assert(m_referencedObj != nullptr);
  return *m_referencedObj;
}

void AstOperator::setReferencedObjAndPropagateAccess(
  unique_ptr<Object> object) {
  assert(object != nullptr);
  setReferencedObjAndPropagateAccess(*object);
  m_foreignObj = move(object);
}

void AstOperator::setReferencedObjAndPropagateAccess(Object& object) {
  assert(returnsByRef());
  assert(m_referencedObj == nullptr); // it doesn't make sense to set it twice
  m_referencedObj = &object;
  m_referencedObj->addAccess(accessFromAstParent());
}

void AstOperator::setObjType(shared_ptr<const ObjType> objType) {
  assert(objType);
  assert(!returnsByRef()); // can't set obj type of m_referencedObj
  assert(m_obj != nullptr);
  assert(m_obj->m_objType == nullptr); // it doesn't make sense to set it twice
  m_obj->m_objType = move(objType);
}

std::string AstOperator::opAsStr() const {
  stringstream ss{};
  ss << m_op;
  return ss.str();
}

string AstOperator::funName() const {
  const auto& s = opAsStr();
  return string{"operator"} + (s[0] >= 'a' && s[0] <= 'z' ? "_" : "") + s;
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
  return toEOperation(op);
}

AstOperator::EOperation AstOperator::toEOperation(const string& op) {
  if (op.size() == 1) {
    assert(op[0] != '*'); // '*' is ambigous: either eMul or eDeref
    assert(op[0] != ';'); // ';' is not really an operator, use AstSeq
    return static_cast<EOperation>(op[0]);
  }
  auto i = m_opMap.find(op);
  assert(i != m_opMap.end());
  return i->second;
}

bool AstOperator::returnsByRef() const {
  return m_op == AstOperator::eAssign || m_op == AstOperator::eDeref;
}

basic_ostream<char>& operator<<(
  basic_ostream<char>& os, AstOperator::EOperation op) {
  if (static_cast<int>(op) < 128) { return os << static_cast<char>(op); }
  auto i = AstOperator::m_opReverseMap.find(op);
  assert(i != AstOperator::m_opReverseMap.end());
  return os << i->second;
}

namespace {
/** Returns the location of the last operand in the sequence, or s_nullLoc if
that doesn't exist. */
Location locationOf(const vector<unique_ptr<AstNode>>& operands) {
  if (operands.empty() || operands.back() == nullptr) { return s_nullLoc; }
  return operands.back()->loc();
}
}

FullConcreteObject AstSeq::m_dummyObj{};

AstSeq::AstSeq(vector<AstNode*>* operands) : AstSeq{toUniquePtrs(operands)} {
}

AstSeq::AstSeq(
  AstNode* op1, AstNode* op2, AstNode* op3, AstNode* op4, AstNode* op5)
  : AstSeq{toUniquePtrs(op1, op2, op3, op4, op5)} {
}

AstSeq::AstSeq(vector<unique_ptr<AstNode>>&& operands)
  : AstObject{locationOf(operands)}, m_operands{move(operands)} {
  for (const auto& operand : m_operands) { assert(operand); }
  assert(!m_operands.empty());
}

Object& AstSeq::referencedObj() const {
  if (lastOperandIsAnObject()) { return lastOperand(); }
  return m_dummyObj;
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
  : AstObject{move(loc)}
  , m_condition{cond}
  , m_action{action}
  , m_elseAction{elseAction} {
  assert(m_condition);
  assert(m_action);
}

const ObjType& AstIf::objType() const {
  assert(m_objType);
  return *m_objType;
}

shared_ptr<const ObjType> AstIf::objTypeAsSp() const {
  assert(m_objType);
  return m_objType;
}

StorageDuration AstIf::storageDuration() const {
  return StorageDuration::eLocal;
}

void AstIf::setObjectType(shared_ptr<const ObjType> objType) {
  assert(objType);
  assert(!m_objType); // doesn't make sense to set it twice
  m_objType = move(objType);
}

AstLoop::AstLoop(AstObject* cond, AstObject* body, Location loc)
  : AstObject{move(loc)}, m_condition(cond), m_body(body) {
  assert(m_condition);
  assert(m_body);
}

const ObjType& AstLoop::objType() const {
  return *objTypeFundaVoid;
}

shared_ptr<const ObjType> AstLoop::objTypeAsSp() const {
  return objTypeFundaVoid;
}

StorageDuration AstLoop::storageDuration() const {
  return StorageDuration::eLocal;
}

AstReturn::AstReturn(AstObject* retVal, Location loc)
  : AstReturn{
      new AstCtList{retVal != nullptr ? retVal : new AstNop{loc}}, loc} {
}

AstReturn::AstReturn(AstCtList* ctorArgs, Location loc)
  : AstObject{loc}
  , m_ctorArgs{ctorArgs != nullptr ? unique_ptr<AstCtList>{ctorArgs}
                                   : make_unique<AstCtList>(loc)} {
  if (m_ctorArgs->childs().empty()) { m_ctorArgs->Add(new AstNop{loc}); }
}

const ObjType& AstReturn::objType() const {
  return *objTypeFundaNoreturn;
}

shared_ptr<const ObjType> AstReturn::objTypeAsSp() const {
  return objTypeFundaNoreturn;
}

StorageDuration AstReturn::storageDuration() const {
  return StorageDuration::eLocal;
}

AstFunCall::AstFunCall(AstObject* address, AstCtList* args, Location loc)
  : AstObject{move(loc)}
  , m_address{address != nullptr ? unique_ptr<AstObject>{address}
                                 : make_unique<AstSymbol>("")}
  , m_args{args != nullptr ? unique_ptr<AstCtList>{args}
                           : make_unique<AstCtList>()} {
  assert(m_address);
  assert(m_args);
}

const ObjType& AstFunCall::objType() const {
  return *objTypeAsSp();
}

shared_ptr<const ObjType> AstFunCall::objTypeAsSp() const {
  const auto objTypeFun =
    dynamic_pointer_cast<const ObjTypeFun>(m_address->objTypeAsSp());
  assert(objTypeFun);
  return objTypeFun->ret().unqualifiedObjType();
}

StorageDuration AstFunCall::storageDuration() const {
  return StorageDuration::eLocal;
}

AstObjTypeSymbol::AstObjTypeSymbol(string name, Location loc)
  : AstObjType{move(loc)}, m_name{move(name)} {
}

AstObjTypeSymbol::AstObjTypeSymbol(ObjTypeFunda::EType fundaType, Location loc)
  : AstObjType{move(loc)}, m_name{toName(fundaType)} {
  assert(fundaType != ObjTypeFunda::ePointer);
}

string AstObjTypeSymbol::toName(ObjTypeFunda::EType fundaType) {
  initMap();
  assert(fundaType != ObjTypeFunda::ePointer);
  return m_typeToName.at(fundaType);
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
    auto i = 0u;
    for (auto& name : m_typeToName) {
      const auto type = static_cast<ObjTypeFunda::EType>(i);
      if (type == ObjTypeFunda::ePointer) { name = "<invalid(pointer)>"; }
      else {
        name = ObjTypeFunda(type).completeName();
      }
      ++i;
    }
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
    new AstNumber{0, new AstObjTypeSymbol{m_name, loc}, loc};

  // What EnvInserter does: nothing

  // What TemplateInstanciator does:
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

// @todo the current implementation assumes that we already know the object type
//       template named m_name, and thus can instantiate it (with zero template
//       args).
void AstObjTypeSymbol::createAndSetObjType() {
  assert(!m_objType); // it doesn't make sense to set it twice
  m_objType = make_shared<ObjTypeFunda>(toType(m_name));
}

AstObjTypeQuali::AstObjTypeQuali(
  ObjType::Qualifiers qualifiers, AstObjType* targetType, Location loc)
  : AstObjType{move(loc)}, m_qualifiers{qualifiers}, m_targetType{targetType} {
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
  : AstObjType{move(loc)}, m_pointee{pointee} {
  assert(m_pointee);
}

void AstObjTypePtr::printValueTo(
  ostream& /*os*/, GeneralValue /*value*/) const {
  // not yet implemented
  assert(false);
}

bool AstObjTypePtr::isValueInRange(GeneralValue value) const {
  return (value <= UINT_MAX) && (value == static_cast<unsigned int>(value));
}

AstObject* AstObjTypePtr::createDefaultAstObjectForSemanticAnalizer(
  Location loc) const {
  // What parser does
  const auto newAstNode = new AstNumber{0, ObjTypeFunda::eNullptr, move(loc)};

  // What EnvInserter does: nothing

  // What TemplateInstanciator does:
  newAstNode->declaredAstObjType().createAndSetObjType();

  // What SemanticAnalizer does: to be done by caller
  return newAstNode;
}

llvm::Value* AstObjTypePtr::createLlvmValueFrom(GeneralValue /*value*/) const {
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
  string name, vector<AstDataDef*>* dataMembers, Location loc)
  : AstObjType{move(loc)}
  , m_name{move(name)}
  , m_dataMembers{toUniquePtrs(dataMembers)} {
  for (const auto& dataMember : m_dataMembers) { assert(dataMember); }
}

AstClassDef::AstClassDef(
  string name, AstDataDef* m1, AstDataDef* m2, AstDataDef* m3)
  : m_name{move(name)}, m_dataMembers{toUniquePtrs(m1, m2, m3)} {
}

void AstClassDef::printValueTo(ostream& /*os*/, GeneralValue /*value*/) const {
  // The liskov substitution principle is broken here
  assert(false);
}

bool AstClassDef::isValueInRange(GeneralValue /*value*/) const {
  // The liskov substitution principle is broken here
  assert(false);
}

AstObject* AstClassDef::createDefaultAstObjectForSemanticAnalizer(
  Location) const {
  // The liskov substitution principle is broken here
  assert(false);
}

llvm::Value* AstClassDef::createLlvmValueFrom(GeneralValue /*value*/) const {
  // not yet implemented
  assert(false);
}

const ObjTypeCompound& AstClassDef::objType() const {
  assert(m_objType);
  return *m_objType;
}

shared_ptr<const ObjType> AstClassDef::objTypeAsSp() const {
  return m_objType;
}

void AstClassDef::createAndSetObjType() {
  assert(!m_objType); // it doesn't make sense to set it twice
  vector<shared_ptr<const ObjType>> dataMembersCopy{};
  for (const auto& dataMember : m_dataMembers) {
    assert(dataMember);
    assert(dataMember->objTypeAsSp());
    dataMembersCopy.emplace_back(dataMember->objTypeAsSp());
  }
  m_objType = make_shared<ObjTypeCompound>(m_name, move(dataMembersCopy));
}

/** The vectors's elements must be non-null */
AstCtList::AstCtList(vector<AstObject*>* childs, Location loc)
  : AstNode{move(loc)}
  , m_childs{childs != nullptr ? childs : new vector<AstObject*>{}} {
  assert(m_childs);
  for (const auto& child : *m_childs) { assert(child); }
}

/** nullptr childs are ignored.*/
AstCtList::AstCtList(Location loc, AstObject* child1, AstObject* child2)
  : AstCtList{nullptr, move(loc)} {
  if (child1 != nullptr) { m_childs->push_back(child1); }
  if (child2 != nullptr) { m_childs->push_back(child2); }
}

/** nullptr childs are ignored.*/
AstCtList::AstCtList(AstObject* child1, AstObject* child2, AstObject* child3,
  AstObject* child4, AstObject* child5, AstObject* child6)
  : AstCtList{Location{}, child1, child2} {
  if (child3 != nullptr) { m_childs->push_back(child3); }
  if (child4 != nullptr) { m_childs->push_back(child4); }
  if (child5 != nullptr) { m_childs->push_back(child5); }
  if (child6 != nullptr) { m_childs->push_back(child6); }
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
  if (child != nullptr) { m_childs->push_back(child); }
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
