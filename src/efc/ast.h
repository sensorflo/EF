#ifndef AST_H
#define AST_H
#include <string>
#include <iostream>

class AstNode {
public:
  virtual ~AstNode() {};
  virtual std::string toStr();
  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>&) = 0;
};

class AstNumber : public AstNode {
public:
  AstNumber(int number) : m_number(number) {}
  virtual std::basic_ostream<char>& printTo(std::basic_ostream<char>& os); 
private:
  const int m_number;
};

#endif
