#pragma once
// If you change this list of header files, you must also modify the
// analogous, redundant list in the Makefile
#include <string>
#include <memory>
class Entity;

/** A node in the Env, see there. Currently very badly integrated, Env does
not yet use EnvNode; only AstClassDef and ObjTypeClass implement it. */
class EnvNode {
public:
  virtual void find(const std::string& name,
    std::shared_ptr<Entity>* entity,
    std::size_t* index = nullptr) const =0;
};
