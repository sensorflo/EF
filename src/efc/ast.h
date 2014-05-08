#ifndef AST_H
#define AST_H
#include <string>
#include <list>
#include <iostream>

class AstNode;
class AstNumber;
class AstOperator;
class AstSeq;
class AstFunDef;

class AstVisitor {
public:
  virtual ~AstVisitor() {};
  virtual void visit(const AstSeq& seq) =0;
  virtual void visit(const AstOperator& op) =0;
  virtual void visit(const AstNumber& number) =0;
  virtual void visit(const AstFunDef& funDef) =0;
};

class AstNode {
public:
  virtual ~AstNode() {};
  virtual void accept(AstVisitor& visitor) const =0;
  virtual std::string toStr();
  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>&) = 0;
};

class AstFunDef : public AstNode {
public:
  AstFunDef(const std::string& name, AstSeq* body);
  virtual ~AstFunDef();
  virtual void accept(AstVisitor& visitor) const { visitor.visit(*this); };
  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>&);
  virtual const std::string& name() const { return m_name; }
  virtual const AstSeq& body() const { return *m_body; }
private:
  std::string m_name;
  /** We're the owner. Is garanteed to be non-null */
  AstSeq* m_body;
};

class AstValue : public AstNode {
};

/** Literal number */
class AstNumber : public AstValue {
public:
  AstNumber(int value) : m_value(value) {}
  virtual void accept(AstVisitor& visitor) const { visitor.visit(*this); };
  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>& os); 
  int value() const { return m_value; }
private:
  const int m_value;
};

class AstOperator : public AstValue {
public:
  AstOperator(char op, AstValue* lhs, AstValue* rhs);
  virtual ~AstOperator();
  virtual void accept(AstVisitor& visitor) const;
  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>& os); 
  char op() const { return m_op; }
  const AstValue& lhs() const { return *m_lhs; }
  const AstValue& rhs() const { return *m_rhs; }
private:
  AstOperator(const AstOperator&);
  AstOperator& operator=(const AstOperator&);
  char m_op;
  AstValue* m_lhs;
  AstValue* m_rhs;
};

class AstSeq : public AstNode {
public:
  AstSeq(AstNode* child1 = NULL);
  AstSeq(AstNode* child1, AstNode* child2, AstNode* child3 = NULL);
  ~AstSeq();
  virtual void accept(AstVisitor& visitor) const;
  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>&);
  AstSeq* Add(AstNode* child);
  AstSeq* Add(AstNode* child1, AstNode* child2, AstNode* child3 = NULL);
  const std::list<AstNode*>& childs() { return m_childs; }
private:
  AstSeq(const AstSeq&);
  AstSeq& operator=(const AstSeq&);

  /** We're the owner of the pointees. Pointers are garanteed to be non null*/
  std::list<AstNode*> m_childs;
};

#endif
