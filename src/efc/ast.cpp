#include "ast.h"
#include "env.h"
#include "astvisitor.h"
#include "astprinter.h"
#include "errorhandler.h"
#include "object.h"
#include "fqnameprovider.h"
#include <cassert>
#include <stdexcept>
using namespace std;

namespace {

template<typename T>
vector<unique_ptr<T>> toUniquePtrs(vector<T*>* src) {
  assert(src);
  const unique_ptr<vector<T*>> dummy(src);
  vector<unique_ptr<T>> dst{};
  for ( const auto& element : *src ) {
    dst.emplace_back(element);
  }
  return dst;
}

template<typename T>
vector<unique_ptr<T>> toUniquePtrs(T* src1 = nullptr, T* src2 = nullptr,
  T* src3 = nullptr, T* src4 = nullptr, T* src5 = nullptr) {
  vector<unique_ptr<T>> dst{};
  for ( const auto& element : {src1, src2, src3, src4, src5} ) {
    if ( element ) {
      dst.emplace_back(element);
    }
  }
  return dst;
}

}

string AstNode::toStr() const {
  return AstPrinter::toStr(*this);
}

AstObject::AstObject() :
  AstObject(nullptr) {
}

AstObject::AstObject(std::shared_ptr<Object> object) :
  m_access(Access::eYetUndefined),
  m_object(move(object)) {
}

void AstObject::setAccess(Access access) {
  m_access = access;
}

const ObjType& AstObject::objType() const {
  assert(m_object);
  return m_object->objType();
}

shared_ptr<const ObjType> AstObject::objTypeAsSp() const {
  assert(m_object);
  return m_object->objTypeAsSp();
}

bool AstObject::objectIsModifiedOrRevealsAddr() const {
  assert(m_object);
  return m_object->isModifiedOrRevealsAddr();
}

void AstObject::setObject(shared_ptr<Object> object) {
  assert(object);
  assert(!m_object.get()); // it makes no sense to set it twice
  m_object = move(object);
}

AstNop::AstNop() :
  AstObject{
    make_shared<Object>(
      make_shared<ObjTypeFunda>(ObjTypeFunda::eVoid),
      StorageDuration::eLocal)} {
}

AstBlock::AstBlock(AstObject* body) :
  m_name(Env::makeUniqueInternalName("$block")),
  m_body(body) {
  assert(m_body);
}

AstCast::AstCast(AstObjType* specifiedNewAstObjType, AstObject* child) :
  m_specifiedNewAstObjType(
    specifiedNewAstObjType ?
    unique_ptr<AstObjType>(specifiedNewAstObjType) :
    make_unique<AstObjTypeSymbol>(ObjTypeFunda::eInt)),
  m_child(child ? unique_ptr<AstObject>(child) : make_unique<AstNumber>(0)) {
  assert(m_child);
  assert(m_specifiedNewAstObjType);
}

AstCast::AstCast(ObjTypeFunda::EType specifiedNewOjType, AstObject* child) :
  AstCast{new AstObjTypeSymbol(specifiedNewOjType), child} {
}

AstObjType& AstCast::specifiedNewAstObjType() const {
  return *m_specifiedNewAstObjType;
}

AstFunDef::AstFunDef(const string& name,
  std::vector<AstDataDef*>* args,
  AstObjType* ret,
  AstObject* body) :
  m_name(name),
  m_args(toUniquePtrs(args)),
  m_ret(ret),
  m_body(body),
  m_fqNameProvider{} {
  assert(m_ret);
  assert(m_body);
}

AstObjType& AstFunDef::declaredRetAstObjType() const {
  assert(m_ret);
  return *m_ret;
}

vector<AstDataDef*>* AstFunDef::createArgs(AstDataDef* arg1,
  AstDataDef* arg2, AstDataDef* arg3) {
  auto args = new vector<AstDataDef*>;
  if (arg1) { args->push_back(arg1); }
  if (arg2) { args->push_back(arg2); }
  if (arg3) { args->push_back(arg3); }
  return args;
}

const string& AstFunDef::fqName() const {
  assert(m_fqNameProvider);
  return m_fqNameProvider->fqName();;
}

void AstFunDef::assignDeclaredObjTypeToAssociatedObject() {
  // create the ObjType of this function
  const auto& argsObjType = new vector<shared_ptr<const ObjType>>;
  for (const auto& astArg: m_args) {
    assert(astArg);
    assert(astArg->declaredAstObjType().objTypeAsSp());
    argsObjType->push_back(astArg->declaredAstObjType().objTypeAsSp());
  }
  assert(m_ret->objTypeAsSp());
  auto&& objTypeOfFun = make_shared<const ObjTypeFun>(argsObjType,
    m_ret->objTypeAsSp());

  // the function Object is naturaly of the ObjType just created above
  m_object->setObjType(move(objTypeOfFun));
}

AstObject* const AstDataDef::noInit = reinterpret_cast<AstObject*>(1);

AstDataDef::AstDataDef(const std::string& name, AstObjType* declaredAstObjType,
  StorageDuration declaredStorageDuration,  AstCtList* ctorArgs) :
  m_name(name),
  m_declaredAstObjType(declaredAstObjType ?
    unique_ptr<AstObjType>(declaredAstObjType) :
    make_unique<AstObjTypeSymbol>(ObjTypeFunda::eInt)),
  m_declaredStorageDuration(declaredStorageDuration),
  m_ctorArgs(mkCtorArgs(ctorArgs, m_declaredStorageDuration, m_doNotInit)),
  m_fqNameProvider{} {
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

AstDataDef::AstDataDef(const std::string& name, ObjTypeFunda::EType declaredObjType,
  AstObject* initObj) :
  AstDataDef(name, new AstObjTypeSymbol(declaredObjType), initObj) {
}

const ObjType& AstDataDef::objType() const {
  assert(m_objType);
  assert(!m_object || m_objType.get() == &m_object->objType());
  return *m_objType;
}

shared_ptr<const ObjType> AstDataDef::objTypeAsSp() const {
  assert(!m_object || m_objType.get() == &m_object->objType());
  return m_objType;
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

const string& AstDataDef::fqName() const {
  assert(m_fqNameProvider);
  return m_fqNameProvider->fqName();
}

StorageDuration AstDataDef::declaredStorageDuration() const {
  return m_declaredStorageDuration;
}

void AstDataDef::assignDeclaredObjTypeToAssociatedObject() {
  assert(m_declaredAstObjType);
  assert(m_declaredAstObjType->objTypeAsSp());
  assert(!m_objType); // it doesn't make sense to set it twice
  m_objType = m_declaredAstObjType->objTypeAsSp();
  if ( m_object ) {
    m_object->setObjType(m_declaredAstObjType->objTypeAsSp());
  }
}

AstNumber::AstNumber(GeneralValue value, AstObjType* astObjType) :
  m_value(value),
  m_declaredAstObjType(
    astObjType ? astObjType : new AstObjTypeSymbol(ObjTypeFunda::eInt)) {
  assert(m_declaredAstObjType);
}

AstNumber::AstNumber(GeneralValue value, ObjTypeFunda::EType eType) :
  AstNumber(value, new AstObjTypeSymbol(eType)) {
}

AstObjType& AstNumber::declaredAstObjType() const {
  assert(m_declaredAstObjType);
  return *m_declaredAstObjType;
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
  m_operands(toUniquePtrs(operands)) {
  for ( const auto& operand : m_operands ) {
    assert(operand);
  }
  assert(!m_operands.empty());
}

AstSeq::AstSeq(AstNode* op1, AstNode* op2, AstNode* op3, AstNode* op4,
  AstNode* op5) :
  m_operands(toUniquePtrs(op1, op2, op3, op4, op5)) {
  assert(!m_operands.empty());
}

AstObject& AstSeq::lastOperandAsAstObject(ErrorHandler& errorHandler) const {
  auto obj = dynamic_cast<AstObject*>(m_operands.back().get());
  if ( !obj ) {
    Error::throwError(errorHandler, Error::eObjectExpected);
  }
  return *obj;
}

AstIf::AstIf(AstObject* cond, AstObject* action, AstObject* elseAction) :
  m_condition(cond),
  m_action(action),
  m_elseAction(elseAction) {
  assert(m_condition);
  assert(m_action);
}

AstLoop::AstLoop(AstObject* cond, AstObject* body) :
  AstObject{
    make_shared<Object>(
      make_shared<ObjTypeFunda>(ObjTypeFunda::eVoid),
      StorageDuration::eLocal)},
  m_condition(cond),
  m_body(body) {
  assert(m_condition);
  assert(m_body);
}

AstReturn::AstReturn(AstObject* retVal) :
  AstObject{
    make_shared<Object>(
      make_shared<ObjTypeFunda>(ObjTypeFunda::eNoreturn),
      StorageDuration::eLocal)},
  m_retVal(retVal ? retVal : new AstNop()) {
  assert(m_retVal);
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

void AstFunCall::createAndSetObjectUsingRetObjType() {
  const auto& objType = m_address->objType();
  assert(typeid(objType)==typeid(ObjTypeFun));
  const auto& objTypeFun = static_cast<const ObjTypeFun&>(objType);
  setObject(make_shared<Object>(
      objTypeFun.ret().unqualifiedObjType(),
      StorageDuration::eLocal));
}

AstObjTypeSymbol::AstObjTypeSymbol(const std::string name) :
  m_name(name) {
}

AstObjTypeSymbol::AstObjTypeSymbol(ObjTypeFunda::EType fundaType) :
  m_name(toName(fundaType)) {
  assert(fundaType!=ObjTypeFunda::ePointer);
}

string AstObjTypeSymbol::toName(ObjTypeFunda::EType fundaType) {
  initMap();
  assert(fundaType!=ObjTypeFunda::ePointer);
  return m_typeToName[fundaType];
}

ObjTypeFunda::EType AstObjTypeSymbol::toType(const string& name) {
  initMap();
  const auto type = find(m_typeToName.begin(), m_typeToName.end(), name);
  assert(type!=m_typeToName.end());
  return static_cast<ObjTypeFunda::EType>(type - m_typeToName.begin()); 
}

void AstObjTypeSymbol::initMap() {
  if ( !m_isMapInitialzied ) {
    m_isMapInitialzied = true;
    for ( int i = 0 ; i<ObjTypeFunda::eTypeCnt; ++i ) {
      const auto type = static_cast<ObjTypeFunda::EType>(i);
      if ( type==ObjTypeFunda::ePointer ) {
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

array<string, ObjTypeFunda::eTypeCnt> AstObjTypeSymbol::m_typeToName{};

bool AstObjTypeSymbol::m_isMapInitialzied = false;

void AstObjTypeSymbol::printValueTo(ostream& os, GeneralValue value) const {
  const auto type = toType(m_name);
  if ( type == ObjTypeFunda::eChar) {
    os << "'" << char(value) << "'";
  } else if ( type == ObjTypeFunda::eNullptr ) {
    os << "nullptr";
  } else {
    os << value;
    switch ( type ) {
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
  case ObjTypeFunda::eChar: return (0<=value && value<=0xFF) && (value==static_cast<int>(value));
  case ObjTypeFunda::eInt: return (INT_MIN<=value && value<=INT_MAX) && (value==static_cast<int>(value));
  case ObjTypeFunda::eDouble: return true;
  case ObjTypeFunda::eBool: return value==0.0 || value==1.0;
  case ObjTypeFunda::eNullptr: return value==0.0;
  case ObjTypeFunda::ePointer: assert(false); // AstObjTypeSymbol does not handle ObjTypeFunda::ePointer
  case ObjTypeFunda::eTypeCnt: assert(false);
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
  case ObjTypeFunda::eChar: // fall through
  case ObjTypeFunda::eInt:  // fall through
  case ObjTypeFunda::eBool:
    return llvm::ConstantInt::get( llvm::getGlobalContext(),
      llvm::APInt(objType().size(), value));
    break;
  case ObjTypeFunda::eDouble:
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
  const auto newAstNode = new AstNumber(0, ObjTypeFunda::eNullptr);

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

AstClassDef::AstClassDef(const std::string& name, std::vector<AstDataDef*>* dataMembers) :
  m_name(name),
  m_dataMembers(toUniquePtrs(dataMembers)) {
  for (const auto& dataMember: m_dataMembers) {
    assert(dataMember);
  }
}

AstClassDef::AstClassDef(const std::string& name, AstDataDef* m1, AstDataDef* m2,
  AstDataDef* m3) :
  m_name(name),
  m_dataMembers(toUniquePtrs(m1, m2, m3)) {
}

void AstClassDef::printValueTo(ostream& os, GeneralValue value) const {
  // The liskov substitution principle is broken here
  assert(false);
}

bool AstClassDef::isValueInRange(GeneralValue value) const {
  // The liskov substitution principle is broken here
  assert(false);
}

AstObject* AstClassDef::createDefaultAstObjectForSemanticAnalizer() {
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
