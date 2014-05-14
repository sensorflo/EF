#ifndef AST_H
#define AST_H
#include <string>
#include <list>
#include <iostream>

class AstNode;
class AstNumber;
class AstSymbol;
class AstFunCall;
class AstOperator;
class AstSeq;
class AstCtList;
class AstFunDef;
class AstFunDecl;
class AstDataDecl;
class AstDataDef;
class AstIf;

class AstVisitor {
public:
  virtual ~AstVisitor() {};
  virtual void visit(const AstSeq& seq) =0;
  virtual void visit(const AstCtList& ctList) =0;
  virtual void visit(const AstOperator& op) =0;
  virtual void visit(const AstNumber& number) =0;
  virtual void visit(const AstSymbol& symbol) =0;
  virtual void visit(const AstFunCall& funCall) =0;
  virtual void visit(const AstFunDef& funDef) =0;
  virtual void visit(const AstFunDecl& funDecl) =0;
  virtual void visit(const AstDataDecl& dataDecl) =0;
  virtual void visit(const AstDataDef& dataDef) =0;
  virtual void visit(const AstIf& if_) =0;
};

class AstNode {
public:
  virtual ~AstNode() {};
  virtual void accept(AstVisitor& visitor) const =0;
  /** Similar to printTo; the resulting text is returned as string. */
  virtual std::string toStr() const;
  /** Print node recursively in its canonical form to given stream */
  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>&) const = 0;
};

class AstValue : public AstNode {
};

class AstFunDef : public AstValue {
public:
  AstFunDef(AstFunDecl* decl, AstSeq* body);
  virtual ~AstFunDef();
  virtual void accept(AstVisitor& visitor) const { visitor.visit(*this); }
  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>&) const;
  virtual const AstFunDecl& decl() const { return *m_decl; }
  virtual const AstSeq& body() const { return *m_body; }
private:
  /** We're the owner. Is garanteed to be non-null */
  const AstFunDecl* const m_decl;
  /** We're the owner. Is garanteed to be non-null */
  const AstSeq* const m_body;
};

class AstFunDecl : public AstValue {
public:
  AstFunDecl(const std::string& name, std::list<std::string>* args = NULL);
  AstFunDecl(const std::string& name, const std::string& arg1,
    const std::string& arg2 = "", const std::string& arg3 = "");
  ~AstFunDecl();
  virtual void accept(AstVisitor& visitor) const { visitor.visit(*this); };
  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>&) const;
  virtual const std::string& name() const { return m_name; }
  virtual const std::list<std::string>& args() const { return *m_args; }
private:
  static std::list<std::string>* createArgs(const std::string& arg1 = "",
    const std::string& arg2 = "", const std::string& arg3 = "");
  const std::string m_name;
  /** We're the owner. Is garanteed to be non-null */
  const std::list<std::string>* const m_args;
};

class AstDataDecl : public AstValue {
public:
  enum EStorage {
    eValue, ///< aka const
    eAlloca ///< aka mut(able)
  };
  AstDataDecl(const std::string& name, EStorage storage = eValue);
  virtual void accept(AstVisitor& visitor) const { visitor.visit(*this); };
  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>&) const;
  virtual const std::string& name() const { return m_name; }
  virtual EStorage storage() const { return m_storage; }
private:
  const std::string m_name;
  const EStorage m_storage;
};

class AstDataDef : public AstValue {
public:
  AstDataDef(AstDataDecl* decl, AstSeq* initValue = NULL);
  ~AstDataDef();
  virtual void accept(AstVisitor& visitor) const { visitor.visit(*this); };
  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>&) const;
  virtual const AstDataDecl& decl() const { return *m_decl; }
  virtual const AstSeq* initValue() const { return m_initValue; }
private:
  /** We're the owner. Is garanteed to be non-null */
  const AstDataDecl* const m_decl;
  /** We're the owner. Is _NOT_ garanteed to be non-null */
  const AstSeq* const m_initValue;
};

/** Literal number */
class AstNumber : public AstValue {
public:
  AstNumber(int value) : m_value(value) {}
  virtual void accept(AstVisitor& visitor) const { visitor.visit(*this); };
  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>& os) const; 
  int value() const { return m_value; }
private:
  const int m_value;
};

class AstSymbol : public AstValue {
public:
  enum ValueCategory { eRValue, eLValue };
  AstSymbol(const std::string* name, ValueCategory valueCategory = eRValue);
  virtual ~AstSymbol();
  virtual void accept(AstVisitor& visitor) const { visitor.visit(*this); };
  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>& os) const; 
  const std::string& name() const { return *m_name; }
  const ValueCategory valueCategory() const { return m_valueCategory; }
private:
  /** We're the owner. Is garanteed to be non-null */
  const std::string* const m_name;
  const ValueCategory m_valueCategory;
};

class AstFunCall : public AstValue {
public:
  AstFunCall(const std::string& name, AstCtList* args = NULL);
  virtual void accept(AstVisitor& visitor) const { visitor.visit(*this); };
  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>& os) const; 
  virtual const std::string& name () const { return m_name; }
  virtual const AstCtList& args () const { return *m_args; }
private:
  const std::string m_name;
  /** We're the owner. Is garanteed to be non-null */
  const AstCtList* const m_args;
};

class AstOperator : public AstValue {
public:
  AstOperator(char op, AstValue* lhs, AstValue* rhs);
  virtual ~AstOperator();
  virtual void accept(AstVisitor& visitor) const { visitor.visit(*this); };
  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>& os) const; 
  char op() const { return m_op; }
  const AstValue& lhs() const { return *m_lhs; }
  const AstValue& rhs() const { return *m_rhs; }
private:
  AstOperator(const AstOperator&);
  AstOperator& operator=(const AstOperator&);
  const char m_op;
  const AstValue* const m_lhs;
  const AstValue* const m_rhs;
};

/* If flow control expression */
class AstIf : public AstValue {
public:
  struct ConditionActionPair {
    ConditionActionPair(AstSeq* condition, AstSeq* action) :
      m_condition(condition), m_action(action) {}
    /** We're the owner. Is garanteed to be non-null */
    AstSeq* m_condition;
    /** We're the owner. Is garanteed to be non-null */
    AstSeq* m_action;
  };
  AstIf(std::list<ConditionActionPair>* conditionActionPairs, AstSeq* elseAction = NULL);
  AstIf(AstSeq* cond, AstSeq* action, AstSeq* elseAction = NULL);
  virtual ~AstIf();
  virtual void accept(AstVisitor& visitor) const { visitor.visit(*this); };
  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>& os) const; 
  const std::list<ConditionActionPair>& conditionActionPairs() const { return *m_conditionActionPairs; }
  const AstSeq* elseAction() const { return m_elseAction; }
private:
  static std::list<ConditionActionPair>* makeDefaultConditionActionPairs();
  static std::list<ConditionActionPair>* makeConditionActionPairs(
    AstSeq* cond, AstSeq* action);
  /** We're the owner. Is garanteed to be non-null and size>=1*/
  const std::list<ConditionActionPair>* const m_conditionActionPairs;
  /** We're the owner. Is NOT garanteed to be non-null */
  const AstSeq* const m_elseAction;
};

class AstSeq : public AstValue {
public:
  AstSeq(AstNode* child1 = NULL);
  AstSeq(AstNode* child1, AstNode* child2, AstNode* child3 = NULL);
  ~AstSeq();
  virtual void accept(AstVisitor& visitor) const { visitor.visit(*this); }
  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>&) const;
  AstSeq* Add(AstNode* child);
  AstSeq* Add(AstNode* child1, AstNode* child2, AstNode* child3 = NULL);
  const std::list<AstNode*>& childs() const { return m_childs; }
private:
  AstSeq(const AstSeq&);
  AstSeq& operator=(const AstSeq&);

  /** We're the owner of the pointees. Pointers are garanteed to be non null*/
  std::list<AstNode*> m_childs;
};

class AstCtList : public AstNode {
public:
  AstCtList(AstNode* child1 = NULL);
  AstCtList(AstNode* child1, AstNode* child2, AstNode* child3 = NULL);
  ~AstCtList();
  virtual void accept(AstVisitor& visitor) const { visitor.visit(*this); };
  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>&) const;
  AstCtList* Add(AstNode* child);
  AstCtList* Add(AstNode* child1, AstNode* child2, AstNode* child3 = NULL);
  const std::list<AstNode*>& childs() const { return m_childs; }
private:
  AstCtList(const AstCtList&);
  AstCtList& operator=(const AstCtList&);

  /** We're the owner of the pointees. Pointers are garanteed to be non null*/
  std::list<AstNode*> m_childs;
};

#endif
