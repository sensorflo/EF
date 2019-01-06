#pragma once

#include "envnode.h"
#include "objtype2.h"

#include <string>
#include <vector>

class Concept {};

using Concepts = std::vector<rw<Concept>>;

// Should be AST node?
class ObjTypeTemplate : public EnvNode {
public:
  /** @todo ctor parameter 'ConceptInstanceDefs params' */
  ObjTypeTemplate(std::string name);
  ~ObjTypeTemplate() override;

  // @todo: also template args and location
  std::string description() const override {
    return "object type template " + fqName();
  }

  ObjType& instanciate(ObjTypes args, ObjType::Qualifiers qualifiers);

  //probably already solved by EnvNode::insert
  //void addMemberFun(FunctionTemplate);
  // for the simple case of a FunctionTemplate not refering to concepts
  //void addMemberFun(const FunctionObj*);

  /** Analogous to instanciate. Here we conceptually instantiate the object type
  template 'function', with the template params being retObjType and flattened
  paramObjTypes.  */
  static ObjType& instanciateFunObjType(
    const ObjType& retObjType, ObjTypes paramObjTypes);

  // @todo how to properly explain / formalize. are these all template
  //       arguments? But ones that are handled specially during matching and
  //       elsewhere? e.g. canBeNull and initializerAutoDeref have influence on
  //       the semantic analizer
  static ObjType& instanciateRawPtrObjType(const ObjType& pointeeObjType,
    EPtrCanBeNull canBeNull, EPtrInit init, ObjType::Qualifiers qualifiers);

private:
  using UP_UnqualifiedObjType =
    std::unique_ptr<UnqualifiedObjType, Deleter<UnqualifiedObjType>>;
  using UP_ObjType = std::unique_ptr<ObjType, Deleter<ObjType>>;

  /** Instantiates a new type if it doesn't already exist, that is instantiate an
  object type template. Instances are never deleted. Since currently EF does not yet allow
  creating custom types, instanciate is currently only used internally for builtin
  types. Also object type templates currently don't exist, so the template
  arguments is always the empty list.
  @param name relative name (i.e. id), _not_ fully qualified name
  @return The ObjType, being an EnvNode, is not yet inserted into the Env

  @todo add Env or EnvNode parentEnvNode as parameter

  @todo something is wrong: the name must be the name of the obj type template, not
  the instantiated obj type, else we can't have multiple instantiations
  with different template args */
  static ObjType& instanciate(std::string name, size_t size,
    ObjTypes templateArgs, llvm::Type* llvmType,
    ObjType::Qualifiers qualifiers);

  static UnqualifiedObjType& instanciateUnqualifiedObjType(
    std::string name, size_t size, ObjTypes templateArgs, llvm::Type* llvmType);

  static UnqualifiedObjType& instanciateFunUnqualifiedObjType(
    const ObjType& retObjType, ObjTypes paramObjTypes);

  static UnqualifiedObjType& instanciateRawPtrUnqualifiedObjType(
    const ObjType& pointeeObjType, EPtrCanBeNull canBeNull, EPtrInit init);

  static ObjType& instanciate(
    UnqualifiedObjType& otr, ObjType::Qualifiers qualifiers);

  // @todo
  // ConceptInstanceDefs m_params
  size_t m_size{0};

  /** All instances of UnqualifiedObjType, inclusive subtypes. Note that the mapping of
  names to UnqualifiedObjType instances is done by our supertype, EnvNode. */
  static std::vector<UP_UnqualifiedObjType> m_UnqualifiedObjTypePool;

  /** Analogous to m_UnqualifiedObjTypePool */
  static std::vector<UP_ObjType> m_ObjTypePool;
};
