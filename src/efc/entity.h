#pragma once
class ErrorHandler;

class Entity {
public:
  bool isDefined() const { return m_isDefined; }
  void markAsDefined(ErrorHandler& errorHandler);
private:
  /** This entity is defined, as opposed to only declared */
  bool m_isDefined;
};

typedef Entity SymbolTableEntry; // TODO: remove temporary solution

