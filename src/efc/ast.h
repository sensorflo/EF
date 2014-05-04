#ifndef AST_H
#define AST_H
#include <string>
#include <list>
#include <iostream>

class AstNode;
class AstNumber;
class AstSeq;

class AstVisitor {
public:
  virtual ~AstVisitor() {};
  virtual void visit(const AstSeq& seq) =0;
  virtual void visit(const AstNumber& number) =0;
};

class AstNode {
public:
  virtual ~AstNode() {};
  virtual void accept(AstVisitor& visitor) const =0;
  virtual std::string toStr();
  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>&) = 0;
};

/** Literal number */
class AstNumber : public AstNode {
public:
  AstNumber(int value) : m_value(value) {}
  virtual void accept(AstVisitor& visitor) const { visitor.visit(*this); };
  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>& os); 
  int value() const { return m_value; }
private:
  const int m_value;
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
private:
  AstSeq(const AstSeq&);
  AstSeq& operator=(const AstSeq&);

  /** We're the owner of the pointees. Pointers are garanteed to be non null*/
  std::list<AstNode*> m_childs;
};

#endif
